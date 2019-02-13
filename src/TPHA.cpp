#include <iostream>

#include "MyFunctions.hpp"
#include "TPHA.hpp"

using std::cerr;
using std::cout;
using std::endl;

TPHA::TPHA()
    : fDebugMode(false),
      fHandler(-1),
      fpParameters(nullptr),
      fpReadoutBuffer(nullptr),
      fppPHAEvents(nullptr),
      fpPHAWaveform(nullptr),
      fDataArray(nullptr)
{
}

TPHA::TPHA(std::string id, CAEN_DGTZ_ConnectionType type, int link, int node,
           uint32_t VMEadd)
    : TPHA()
{
  // TPHA();
  fDeviceID = id;

  Open(type, link, node, VMEadd);
  GetBoardInfo();
  fpParameters = new TDigiPar(id, fDeviceInfo);  // How to check fDeviceInfo
  fpParameters->GenParameter();

  fNChs = fpParameters->GetNCh();
  fTimeSample = fpParameters->GetTimeSample();

  fTime.resize(fNChs);
  fTimeOffset.resize(fNChs);
  fPreviousTime.resize(fNChs);
  fDataArray = new unsigned char[4096 * ONE_HIT_SIZE * 16];
}

TPHA::~TPHA()
{
  FreeMemory();
  // Now I assume the handler is positive number or 0.
  if (fHandler > -1) Close();
  DelPointer(fpParameters);
  delete[] fDataArray;
  fDataArray = nullptr;
}

void TPHA::ConfigDevice()
{
  // Writing the procedures in this function means I can not understand why
  // need.  Those are copied from digiTES

  ResetDevice();

  SetChMask();

  SetFanSpeed();

  // Channel Control Reg (indiv trg, seq readout) ??
  if (fpParameters->GetDigiModel() == 5000) {
    auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8000, 0);
    PrintError(err, "UNKNOWN1");
  } else if ((fpParameters->GetDPPType() == DPPType::DPP_PHA_724) ||
             (fpParameters->GetDPPType() == DPPType::DPP_nPHA_724)) {
    auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8000, 0x01000114);
    PrintError(err, "UNKNOWN2");
  }

  // Enable new format
  if (fpParameters->GetDigiModel() == 751) {
    auto err = RegisterSetBits(0x8000, 31, 31, 1);
    PrintError(err, "UNKNOWN3");
  }

  if (fpParameters->GetDPPType() != DPPType::STD_730) {  // only for DPP
    // Acquisition mode (MIXED)
    SetACQMode();

    // Enable Extra Word
    if ((fpParameters->GetDigiModel() == 730) ||
        (fpParameters->GetDigiModel() == 725) ||
        (fpParameters->GetDigiModel() == 751)) {
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1 << 17);
      PrintError(err, "UNKNOWN4");
    }
    // some specific settings in the global CTRL register
    if (true) {  // set trgout bidir
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, (1 << 2));
      PrintError(err, "UNKNOWN5");
    }

    if (fpParameters->GetDPPType() ==
        DPPType::DPP_PSD_751) {  // Set new probe mode in x751 models
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1 << 26);
      PrintError(err, "UNKNOWN6");
    }

    if ((fpParameters->GetDPPType() == DPPType::DPP_PHA_730) ||
        (fpParameters->GetDPPType() == DPPType::DPP_PSD_730) ||
        (fpParameters->GetDPPType() ==
         DPPType::DPP_nPHA_724)) {  // Enable auto - flush
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1);
      PrintError(err, "UNKNOWN7");
    }

    if (fpParameters->GetDPPType() == DPPType::DPP_PSD_720) {
      // Enable baseline or Extended Time stamp
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1 << 17);
      PrintError(err, "UNKNOWN8");
      // Enable ext time stamp
      err = RegisterSetBits(0x1080, 7, 7, 1);
      PrintError(err, "UNKNOWN9");
    }
  }

  SetRecordLength(fpParameters->GetRecordLength());

  SetIOLevel(fpParameters->GetIOLevel());

  // In digiTES, here are some SYNC mode settings.
  // I set Acquisition mode as independent mode.
  auto err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  SetDeviceBuffer();

  SetChParameter();

  BoardCalibration();
}

void TPHA::ResetDevice()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
}

void TPHA::SetChMask()
{
  auto chFlag = fpParameters->GetChFlag();
  uint32_t chMask = 0;
  for (uint i = 0; i < chFlag.size(); i++) chMask |= (chFlag[i] << i);
  // cout << "Channel mask: " << chMask << endl;
  auto err = CAEN_DGTZ_SetChannelEnableMask(fHandler, chMask);
  PrintError(err, "SetChannelEnableMask");
}

void TPHA::SetFanSpeed()
{
  if (fpParameters->GetDigiModel() != 5000) {
    uint32_t fanSpeed = 0b110000;  // Low or auto speed mode
    // uint32_t fanSpeed = 0b111000;  // High speed mode
    auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8168, fanSpeed);
    PrintError(err, "SetFanSpeed");
  }
}

void TPHA::SetACQMode()
{  // We do not use other modes.
  auto err = CAEN_DGTZ_SetDPPAcquisitionMode(
      fHandler, CAEN_DGTZ_DPP_AcqMode_t::CAEN_DGTZ_DPP_ACQ_MODE_Mixed,
      CAEN_DGTZ_DPP_SaveParam_t::CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
  PrintError(err, "SetDPPAcquisitionMode");
}

void TPHA::SetRecordLength(uint32_t size)
{
  // What does channel = -1 mean?  Copied from digiTES
  auto err = CAEN_DGTZ_SetRecordLength(fHandler, size, -1);
  PrintError(err, "SetRecordLength");
}

void TPHA::SetIOLevel(CAEN_DGTZ_IOLevel_t level)
{
  auto err = CAEN_DGTZ_SetIOLevel(fHandler, level);
  PrintError(err, "SetIOLevel");
}

void TPHA::SetDeviceBuffer()
{
  auto type = fpParameters->GetDPPType();
  if (type != DPPType::STD_730) {  // for DPP
    // 0 means automatic mode
    auto err = CAEN_DGTZ_SetDPPEventAggregation(fHandler, 0, 0);
    PrintError(err, "SetDPPEventAggregation");
    // I have no idea about 1023.  4096 (4095) is also work?
    err = CAEN_DGTZ_SetMaxNumAggregatesBLT(fHandler, 1023);
    PrintError(err, "SetMaxNumAggregatesBLT");
  } else {
    auto err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, 1023);
    PrintError(err, "SetMaxNumEventsBLT");
  }
}

void TPHA::SetChParameter()
{
  if (fpParameters->GetDPPType() == DPPType::DPP_PHA_730) {
    auto offset = fpParameters->GetDCOffset();
    auto preTrigger = fpParameters->GetPreTrigger();

    auto TTFSmoothing = fpParameters->GetTTFSmoothing();
    auto TTFDelay = fpParameters->GetTTFDelay();
    auto trapRiseTime = fpParameters->GetTrapRiseTime();
    auto trapFlatTop = fpParameters->GetTrapFlatTop();
    auto peakingTime = fpParameters->GetPeakingTime();
    auto trapPoleZero = fpParameters->GetTrapPoleZero();
    auto threshold = fpParameters->GetThreshold();
    auto trigHoldOff = fpParameters->GetTrgHoldOff();
    auto SHF = fpParameters->GetSHF();
    auto NSPeak = fpParameters->GetNSPeak();
    auto polarity = fpParameters->GetPolarity();
    auto trapNSBaseline = fpParameters->GetTrapNSBaseline();
    auto decimation = fpParameters->GetDecimation();
    auto fakeEventFlag = fpParameters->GetFakeEventFlag();
    auto trapCFDFraction = fpParameters->GetTrapCFDFraction();
    auto dynamicRange = fpParameters->GetDynamicRange();

    auto trapNorm = fpParameters->GetTrapNorm();
    // auto fineGain = fpParameters->GetEneFineGain();

    for (uint iCh = 0; iCh < fNChs; iCh++) {
      auto err = CAEN_DGTZ_SetChannelDCOffset(fHandler, iCh, offset[iCh]);
      cout << offset[iCh] << endl;
      PrintError(err, "SetChannelDCOffset");

      err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, iCh, preTrigger[iCh]);
      PrintError(err, "SetDPPPreTriggerSize");

      SetTrapNorm(iCh, trapNorm[iCh]);
      // auto gain = uint(fineGain[iCh] * 1000);
      // SetEneFineGain(iCh, gain);
      // SetPreTrigger(iCh, preTrigger[iCh]);
      SetTTFSmoothing(iCh, TTFSmoothing[iCh]);
      SetTTFDelay(iCh, TTFDelay[iCh]);
      SetTrapRiseTime(iCh, trapRiseTime[iCh]);
      SetTrapFlatTop(iCh, trapFlatTop[iCh]);
      SetPeakingTime(iCh, peakingTime[iCh]);
      SetTrapPoleZero(iCh, trapPoleZero[iCh]);
      SetThreshold(iCh, threshold[iCh]);
      SetTimeValWin(iCh, 0);
      SetTrgHoldOff(iCh, trigHoldOff[iCh]);
      SetPeakHoldOff(iCh, 8);
      SetBaselineHoldOff(iCh, 4);
      SetSHF(iCh, SHF[iCh]);
      SetNSPeak(iCh, NSPeak[iCh]);
      SetPolarity(iCh, polarity[iCh]);
      SetTrapNSBaseline(iCh, trapNSBaseline[iCh]);
      SetDecimation(iCh, decimation[iCh]);
      SetFakeEventFlag(iCh, fakeEventFlag[iCh]);
      SetDiscr(iCh, trapCFDFraction[iCh]);
      SetDynamicRange(iCh, dynamicRange[iCh]);
      SetExtraWord(iCh, 2);
    }
  } else {
    cout << "Now only support x730 series with PHA FW." << endl;
    exit(0);
  }

  for (uint iCh = 0; iCh < fNChs; iCh++) {
    CAEN_DGTZ_PulsePolarity_t pol;
    auto err = CAEN_DGTZ_GetChannelPulsePolarity(fHandler, iCh, &pol);
    cout << err << "\t" << pol << endl;
  }
}

void TPHA::SetTrapNorm(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x10C4 + (ch << 8), val);
  PrintError(err, "SetTrapNorm");
}
void TPHA::SetEneFineGain(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x10C4 + (ch << 8), val);
  PrintError(err, "SetTrapNorm");
}
void TPHA::SetPreTrigger(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1038 + (ch << 8), val);
  PrintError(err, "SetPreTrigger");
}
void TPHA::SetTTFSmoothing(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1054 + (ch << 8), val);
  PrintError(err, "SetTTFSmoothing");
}
void TPHA::SetTTFDelay(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1058 + (ch << 8), val);
  PrintError(err, "SetTTFDelay");
}
void TPHA::SetTrapRiseTime(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x105C + (ch << 8), val);
  PrintError(err, "SetTrapRiseTime");
}
void TPHA::SetTrapFlatTop(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1060 + (ch << 8), val);
  PrintError(err, "SetTrapFlatTop");
}
void TPHA::SetPeakingTime(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1064 + (ch << 8), val);
  PrintError(err, "SetPeakingTime");
}
void TPHA::SetTrapPoleZero(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1068 + (ch << 8), val);
  PrintError(err, "SetTrapPoleZero");
}
void TPHA::SetThreshold(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x106C + (ch << 8), val);
  PrintError(err, "SetThreshold");
}
void TPHA::SetTimeValWin(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1070 + (ch << 8), val);
  PrintError(err, "SetTimeValWin");
}
void TPHA::SetTrgHoldOff(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1074 + (ch << 8), val);
  PrintError(err, "SetTrgHoldOff");
}
void TPHA::SetPeakHoldOff(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1078 + (ch << 8), val);
  PrintError(err, "SetPeakHoldOff");
}
void TPHA::SetBaselineHoldOff(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x107C + (ch << 8), val);
  PrintError(err, "SetBaselineHoldOff");
}
void TPHA::SetSHF(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 0, 5, val);
  PrintError(err, "SetSHF");
}
void TPHA::SetNSPeak(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 12, 13, val);
  PrintError(err, "SetNSPeak");
}
void TPHA::SetPolarity(uint ch, uint val)
{
  // auto err = RegisterSetBits(0x1080 + (ch << 8), 16, 16, val);
  CAEN_DGTZ_PulsePolarity_t pol = CAEN_DGTZ_PulsePolarity_t(val);
  auto err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, ch, pol);
  PrintError(err, "SetPolarity");
}
void TPHA::SetTrapNSBaseline(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 20, 22, val);
  PrintError(err, "SetTrapNSBaseline");
}
void TPHA::SetDecimation(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 8, 9, val);
  PrintError(err, "SetDecimation");
}
void TPHA::SetFakeEventFlag(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 26, 26, val);
  PrintError(err, "SetFakeEventFlag");
}
void TPHA::SetDiscr(uint ch, uint val)
{
  // ZeroVoltLevel should be ignored by DPP_PHA_730
  // But used in digiTES.  Why?
  // CAEN_DGTZ_WriteRegister(fHandler, 0x10D0 + (ch << 8), ZeroVoltLevel);
  auto err = RegisterSetBits(0x1080 + (ch << 8), 6, 6, 0);
  PrintError(err, "SetDiscr");
  err = RegisterSetBits(0x1080 + (ch << 8), 17, 17, 1);
  PrintError(err, "SetDiscr");
  err = RegisterSetBits(0x10A0 + (ch << 8), 12, 13, val);
  PrintError(err, "SetDiscr");
}
void TPHA::SetDynamicRange(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1028 + (ch << 8), val);
  PrintError(err, "SetDynamicRange");
}
void TPHA::SetExtraWord(uint ch, uint val)
{
  auto err = RegisterSetBits(0x10A0 + (ch << 8), 8, 10, val);
  PrintError(err, "SetExtraWord");
}

void TPHA::AllocateMemory()
{
  // FreeMemory();
  uint32_t size;

  auto err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer, &size);
  PrintError(err, "MallocReadoutBuffer");

  fppPHAEvents = new CAEN_DGTZ_DPP_PHA_Event_t *[fNChs];
  err = CAEN_DGTZ_MallocDPPEvents(fHandler, (void **)fppPHAEvents, &size);
  PrintError(err, "MallocDPPEvents");

  err = CAEN_DGTZ_MallocDPPWaveforms(fHandler, (void **)&fpPHAWaveform, &size);
  PrintError(err, "MallocDPPWaveforms");
}

void TPHA::FreeMemory()
{
  // I can not know
  // Think deleting pointer timing
  if (fpReadoutBuffer != nullptr) {
    auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
    PrintError(err, "FreeReadoutBuffer");
    fpReadoutBuffer = nullptr;
  }
  if (fppPHAEvents != nullptr) {
    auto err = CAEN_DGTZ_FreeDPPEvents(fHandler, (void **)fppPHAEvents);
    PrintError(err, "FreeDPPEvents");
    fppPHAEvents = nullptr;
  }
  if (fpPHAWaveform != nullptr) {
    auto err = CAEN_DGTZ_FreeDPPWaveforms(fHandler, fpPHAWaveform);
    PrintError(err, "FreeDPPWaveforms");
    fpPHAWaveform = nullptr;
  }
}

CAEN_DGTZ_ErrorCode TPHA::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");
  AllocateMemory();
  for (auto &t : fTime) t = 0;
  for (auto &t : fTimeOffset) t = 0;
  for (auto &t : fPreviousTime) t = 0;

  return err;
}

void TPHA::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");
  FreeMemory();
}

void TPHA::ReadEvents()
{
  fNEvents = 0;  // Event counter

  CAEN_DGTZ_ErrorCode err;

  uint32_t bufferSize;
  err = CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                           fpReadoutBuffer, &bufferSize);
  PrintError(err, "ReadData");
  if (bufferSize == 0) return;  // in the case of 0, GetDPPEvents makes crush

  uint32_t nEvents[fNChs];
  err = CAEN_DGTZ_GetDPPEvents(fHandler, fpReadoutBuffer, bufferSize,
                               (void **)fppPHAEvents, nEvents);
  PrintError(err, "GetDPPEvents");

  // for (int i = 0; i < nEvents[0]; i++)
  //   std::cout << (fppPHAEvents[0][i]).ChargeLong << std::endl;

  for (uint iCh = 0; iCh < fNChs; iCh++) {
    for (uint iEve = 0; iEve < nEvents[iCh]; iEve++) {
      err = CAEN_DGTZ_DecodeDPPWaveforms(fHandler, &fppPHAEvents[iCh][iEve],
                                         fpPHAWaveform);
      PrintError(err, "DecodeDPPWaveforms");

      auto index = fNEvents * ONE_HIT_SIZE;
      fDataArray[index++] = 0;    // fModNumber is needed.
      fDataArray[index++] = iCh;  // int to char.  Dangerous

      constexpr uint64_t timeMask = 0x7FFFFFFF;
      auto tdc =
          (fppPHAEvents[iCh][iEve].TimeTag & timeMask) + fTimeOffset[iCh];
      if (tdc < fPreviousTime[iCh]) {
        tdc += (timeMask + 1);
        fTimeOffset[iCh] += (timeMask + 1);
      }
      fPreviousTime[iCh] = tdc;

      // if (fTSample > 0) tdc *= fTSample;
      fTime[iCh] = tdc * fTimeSample;

      constexpr auto timeSize = sizeof(fTime[0]);
      memcpy(&fDataArray[index], &fTime[iCh], timeSize);
      index += timeSize;

      constexpr auto adcSize = sizeof(fppPHAEvents[0][0].Energy);
      // auto adc = sumCharge;
      memcpy(&fDataArray[index], &fppPHAEvents[iCh][iEve].Energy, adcSize);
      index += adcSize;

      // std::cout << fppPHAEvents[iCh][iEve].ChargeLong << std::endl;

      constexpr auto waveSize = sizeof(fpPHAWaveform->Trace1[0]) * kNSamples;
      memcpy(&fDataArray[index], fpPHAWaveform->Trace1, waveSize);

      fNEvents++;
    }
  }
}

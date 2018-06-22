#include <iostream>

#include "MyFunctions.hpp"
#include "TDigitizer.hpp"

using std::cerr;
using std::cout;
using std::endl;

TDigitizer::TDigitizer()
    : fDebugMode(false),
      fHandler(-1),
      fpParameters(nullptr),
      fpReadoutBuffer(nullptr),
      fppPHAEvents(nullptr),
      fpPHAWaveform(nullptr),
      fDataArray(nullptr)
{
}

TDigitizer::TDigitizer(std::string id, CAEN_DGTZ_ConnectionType type, int link,
                       int node, uint32_t VMEadd)
    : TDigitizer()
{
  // TDigitizer();
  fDeviceID = id;

  OpenDevice(type, link, node, VMEadd);
  GetDeviceInfo();
  fpParameters = new TDigiPar(id, fDeviceInfo);  // How to check fDeviceInfo
  fpParameters->GenParameter();

  fNChs = fpParameters->GetNCh();
  fTimeSample = fpParameters->GetTimeSample();

  fTime.resize(fNChs);
  fTimeOffset.resize(fNChs);
  fPreviousTime.resize(fNChs);
  fDataArray = new unsigned char[4096 * ONE_HIT_SIZE * 16];
}

TDigitizer::~TDigitizer()
{
  FreeMemory();
  // Now I assume the handler is positive number or 0.
  if (fHandler > -1) CloseDevice();
  DelPointer(fpParameters);
  delete[] fDataArray;
  fDataArray = nullptr;
}

void TDigitizer::CloseDevice()
{
  auto err = CAEN_DGTZ_CloseDigitizer(fHandler);
  if (err == CAEN_DGTZ_Success)
    fHandler = -1;
  else
    PrintError(err, "CloseDigitizer");
}

void TDigitizer::OpenDevice(CAEN_DGTZ_ConnectionType type, int link, int node,
                            uint32_t VMEadd)
{
  auto err = CAEN_DGTZ_OpenDigitizer(type, link, node, VMEadd, &fHandler);
  if (err != CAEN_DGTZ_Success) {
    cout << "Can not open the device!  Check the connection" << endl;
    exit(0);
  }
}

void TDigitizer::GetDeviceInfo()
{
  auto err = CAEN_DGTZ_GetInfo(fHandler, &fDeviceInfo);
  PrintError(err, "GetInfo");

  cout << "Model name:\t" << fDeviceInfo.ModelName << "\n"
       << "Model number:\t" << fDeviceInfo.Model << "\n"
       << "No. channels:\t" << fDeviceInfo.Channels << "\n"
       << "Format factor:\t" << fDeviceInfo.FormFactor << "\n"
       << "Family code:\t" << fDeviceInfo.FamilyCode << "\n"
       << "Firmware revision of the FPGA on the mother board (ROC):\t"
       << fDeviceInfo.ROC_FirmwareRel << "\n"
       << "Firmware revision of the FPGA on the daughter board (AMC):\t"
       << fDeviceInfo.AMC_FirmwareRel << "\n"
       << "Serial number:\t" << fDeviceInfo.SerialNumber << "\n"
       << "PCB revision:\t" << fDeviceInfo.PCB_Revision << "\n"
       << "No. bits of the ADC:\t" << fDeviceInfo.ADC_NBits << "\n"
       << "Device handler of CAENComm:\t" << fDeviceInfo.CommHandle << "\n"
       << "Device handler of CAENVME:\t" << fDeviceInfo.VMEHandle << "\n"
       << "License number:\t" << fDeviceInfo.License << endl;
}

void TDigitizer::PrintError(const CAEN_DGTZ_ErrorCode &err,
                            const std::string &funcName)
{
  if (fDebugMode) std::cout << funcName << std::endl;
  if (err < 0) {  // 0 is success
    std::cerr << "In " << funcName << ", error code = " << err << endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

void TDigitizer::CalibrationADC()
{
  if (((fpParameters->GetDigiModel() == 730) ||
       (fpParameters->GetDigiModel() == 725) ||
       (fpParameters->GetDigiModel() == 751))) {
    auto err = CAEN_DGTZ_Calibrate(fHandler);
    if ((fpParameters->GetDigiModel() == 730) ||
        (fpParameters->GetDigiModel() == 725)) {
      for (auto i = 0; i < fNChs; i++) LockTempCalibration_x730(i);
      for (auto i = 0; i < fNChs; i++) {
        uint32_t d32;
        CAEN_DGTZ_ReadRegister(fHandler, 0x1088 + (i << 8), &d32);
        if (!(d32 & 0x8))
          cout << "\nWARNING: calibration not done on ch. " << i << endl;
      }
    }
    usleep(10000);
  }
}

void TDigitizer::LockTempCalibration_x730(uint ch)
{  // I can not understand what actually do
  uint32_t lock, ctrl;

  // enter engineering functions
  int err = WriteSPIRegister(ch, 0x7A, 0x59);
  err += WriteSPIRegister(ch, 0x7A, 0x1A);
  err += WriteSPIRegister(ch, 0x7A, 0x11);
  err += WriteSPIRegister(ch, 0x7A, 0xAC);

  // read lock value
  err += ReadSPIRegister(ch, 0xA7, lock);
  // write lock value
  err += WriteSPIRegister(ch, 0xA5, lock);

  // enable lock
  err += ReadSPIRegister(ch, 0xA4, ctrl);
  ctrl |= 0x4;  // set bit 2
  err += WriteSPIRegister(ch, 0xA4, ctrl);

  err += ReadSPIRegister(ch, 0xA4, ctrl);
  if (err != 0) {
    cerr << "LockTempCalibration has problem" << endl;
  }
}

void TDigitizer::ConfigDevice()
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

  CalibrationADC();
}

void TDigitizer::ResetDevice()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
}

void TDigitizer::SetChMask()
{
  auto chFlag = fpParameters->GetChFlag();
  uint32_t chMask = 0;
  for (auto i = 0; i < chFlag.size(); i++) chMask |= (chFlag[i] << i);
  // cout << "Channel mask: " << chMask << endl;
  auto err = CAEN_DGTZ_SetChannelEnableMask(fHandler, chMask);
  PrintError(err, "SetChannelEnableMask");
}

void TDigitizer::SetFanSpeed()
{
  if (fpParameters->GetDigiModel() != 5000) {
    uint32_t fanSpeed = 0b110000;  // Low or auto speed mode
    // uint32_t fanSpeed = 0b111000;  // High speed mode
    auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8168, fanSpeed);
    PrintError(err, "SetFanSpeed");
  }
}

void TDigitizer::SetACQMode()
{  // We do not use other modes.
  auto err = CAEN_DGTZ_SetDPPAcquisitionMode(
      fHandler, CAEN_DGTZ_DPP_AcqMode_t::CAEN_DGTZ_DPP_ACQ_MODE_Mixed,
      CAEN_DGTZ_DPP_SaveParam_t::CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
  PrintError(err, "SetDPPAcquisitionMode");
}

void TDigitizer::SetRecordLength(uint32_t size)
{
  // What does channel = -1 mean?  Copied from digiTES
  auto err = CAEN_DGTZ_SetRecordLength(fHandler, size, -1);
  PrintError(err, "SetRecordLength");
}

void TDigitizer::SetIOLevel(CAEN_DGTZ_IOLevel_t level)
{
  auto err = CAEN_DGTZ_SetIOLevel(fHandler, level);
  PrintError(err, "SetIOLevel");
}

void TDigitizer::SetDeviceBuffer()
{
  auto DPPType = fpParameters->GetDPPType();
  if (DPPType != DPPType::STD_730) {  // for DPP
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

void TDigitizer::SetChParameter()
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
    for (auto iCh = 0; iCh < fNChs; iCh++) {
      auto err = CAEN_DGTZ_SetChannelDCOffset(fHandler, iCh, offset[iCh]);
      PrintError(err, "SetChannelDCOffset");

      err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, iCh, preTrigger[iCh]);
      PrintError(err, "SetDPPPreTriggerSize");

      SetTrapNorm(iCh, trapNorm[iCh]);
      SetPreTrigger(iCh, preTrigger[iCh] / 4);  // Really needed?
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
}

void TDigitizer::SetTrapNorm(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x10C4 + (ch << 8), val);
  PrintError(err, "SetTrapNorm");
}
void TDigitizer::SetPreTrigger(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1038 + (ch << 8), val);
  PrintError(err, "SetPreTrigger");
}
void TDigitizer::SetTTFSmoothing(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1054 + (ch << 8), val);
  PrintError(err, "SetTTFSmoothing");
}
void TDigitizer::SetTTFDelay(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1058 + (ch << 8), val);
  PrintError(err, "SetTTFDelay");
}
void TDigitizer::SetTrapRiseTime(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x105C + (ch << 8), val);
  PrintError(err, "SetTrapRiseTime");
}
void TDigitizer::SetTrapFlatTop(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1060 + (ch << 8), val);
  PrintError(err, "SetTrapFlatTop");
}
void TDigitizer::SetPeakingTime(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1064 + (ch << 8), val);
  PrintError(err, "SetPeakingTime");
}
void TDigitizer::SetTrapPoleZero(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1068 + (ch << 8), val);
  PrintError(err, "SetTrapPoleZero");
}
void TDigitizer::SetThreshold(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x106C + (ch << 8), val);
  PrintError(err, "SetThreshold");
}
void TDigitizer::SetTimeValWin(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1070 + (ch << 8), val);
  PrintError(err, "SetTimeValWin");
}
void TDigitizer::SetTrgHoldOff(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1074 + (ch << 8), val);
  PrintError(err, "SetTrgHoldOff");
}
void TDigitizer::SetPeakHoldOff(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1078 + (ch << 8), val);
  PrintError(err, "SetPeakHoldOff");
}
void TDigitizer::SetBaselineHoldOff(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x107C + (ch << 8), val);
  PrintError(err, "SetBaselineHoldOff");
}
void TDigitizer::SetSHF(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 0, 5, val);
  PrintError(err, "SetSHF");
}
void TDigitizer::SetNSPeak(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 12, 13, val);
  PrintError(err, "SetNSPeak");
}
void TDigitizer::SetPolarity(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 16, 16, val);
  PrintError(err, "SetPolarity");
}
void TDigitizer::SetTrapNSBaseline(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 20, 22, val);
  PrintError(err, "SetTrapNSBaseline");
}
void TDigitizer::SetDecimation(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 8, 9, val);
  PrintError(err, "SetDecimation");
}
void TDigitizer::SetFakeEventFlag(uint ch, uint val)
{
  auto err = RegisterSetBits(0x1080 + (ch << 8), 26, 26, val);
  PrintError(err, "SetFakeEventFlag");
}
void TDigitizer::SetDiscr(uint ch, uint val)
{
  // ZeroVoltLevel should be ignored by DPP_PHA_730
  // But used in digiTES.  Why?
  // CAEN_DGTZ_WriteRegister(fHandler, 0x10D0 + (ch << 8), ZeroVoltLevel);
  auto err = RegisterSetBits(0x1080 + (ch << 8), 6, 6, 0);
  PrintError(err, "SetDicr");
  err = RegisterSetBits(0x1080 + (ch << 8), 17, 17, 1);
  PrintError(err, "SetDicr");
  err = RegisterSetBits(0x10A0 + (ch << 8), 12, 13, val);
  PrintError(err, "SetDicr");
}
void TDigitizer::SetDynamicRange(uint ch, uint val)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x1028 + (ch << 8), val);
  PrintError(err, "SetDynamicRange");
}
void TDigitizer::SetExtraWord(uint ch, uint val)
{
  auto err = RegisterSetBits(0x10A0 + (ch << 8), 8, 10, val);
  PrintError(err, "SetExtraWord");
}

// Acquisition controll
void TDigitizer::SendSWTrigger()
{
  auto err = CAEN_DGTZ_SendSWtrigger(fHandler);
  PrintError(err, "SendSWTrigger");
}

void TDigitizer::AllocateMemory()
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

void TDigitizer::FreeMemory()
{
  // I can not know
  if (fpReadoutBuffer != nullptr) {
    auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
    PrintError(err, "FreeReadoutBuffer");
    fpReadoutBuffer = nullptr;
  }
  if (fpReadoutBuffer != nullptr) {
    auto err = CAEN_DGTZ_FreeDPPEvents(fHandler, (void **)fppPHAEvents);
    PrintError(err, "FreeDPPEvents");
    fppPHAEvents = nullptr;
  }
  if (fpReadoutBuffer != nullptr) {
    auto err = CAEN_DGTZ_FreeDPPWaveforms(fHandler, fpPHAWaveform);
    PrintError(err, "FreeDPPWaveforms");
    fpPHAWaveform = nullptr;
  }
}

void TDigitizer::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");
  AllocateMemory();
  for (auto &t : fTime) t = 0;
  for (auto &t : fTimeOffset) t = 0;
  for (auto &t : fPreviousTime) t = 0;
}

void TDigitizer::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");
  FreeMemory();
}

void TDigitizer::ReadEvent()
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

  for (int iCh = 0; iCh < fNChs; iCh++) {
    for (int iEve = 0; iEve < nEvents[iCh]; iEve++) {
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

CAEN_DGTZ_ErrorCode TDigitizer::WriteSPIRegister(uint32_t ch, uint32_t address,
                                                 uint32_t value)
{
  uint32_t SPIBusy = 1;
  int32_t ret = CAEN_DGTZ_Success;

  uint32_t SPIBusyAddr = 0x1088 + (ch << 8);
  uint32_t addressingRegAddr = 0x80B4;
  uint32_t valueRegAddr = 0x10B8 + (ch << 8);

  while (SPIBusy) {
    if ((ret = CAEN_DGTZ_ReadRegister(fHandler, SPIBusyAddr, &SPIBusy)) !=
        CAEN_DGTZ_Success)
      return CAEN_DGTZ_CommError;
    SPIBusy = (SPIBusy >> 2) & 0x1;
    if (!SPIBusy) {
      if ((ret = CAEN_DGTZ_WriteRegister(fHandler, addressingRegAddr,
                                         address)) != CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
      if ((ret = CAEN_DGTZ_WriteRegister(fHandler, valueRegAddr, value)) !=
          CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
    }
    usleep(1000);
  }
  return CAEN_DGTZ_Success;
}

CAEN_DGTZ_ErrorCode TDigitizer::ReadSPIRegister(uint32_t ch, uint32_t address,
                                                uint32_t &value)
{  // Copy from digiTES
  uint32_t SPIBusy = 1;
  int32_t ret = CAEN_DGTZ_Success;
  uint32_t SPIBusyAddr = 0x1088 + (ch << 8);
  uint32_t addressingRegAddr = 0x80B4;
  uint32_t valueRegAddr = 0x10B8 + (ch << 8);

  while (SPIBusy) {
    if ((ret = CAEN_DGTZ_ReadRegister(fHandler, SPIBusyAddr, &SPIBusy)) !=
        CAEN_DGTZ_Success)
      return CAEN_DGTZ_CommError;
    SPIBusy = (SPIBusy >> 2) & 0x1;
    if (!SPIBusy) {
      if ((ret = CAEN_DGTZ_WriteRegister(fHandler, addressingRegAddr,
                                         address)) != CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
      if ((ret = CAEN_DGTZ_ReadRegister(fHandler, valueRegAddr, &value)) !=
          CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
    }
    usleep(1000);
  }
  return CAEN_DGTZ_Success;
}

CAEN_DGTZ_ErrorCode TDigitizer::RegisterSetBits(uint16_t addr, int start_bit,
                                                int end_bit, int val)
{  // copy from digiTes
  uint32_t mask = 0, reg;
  CAEN_DGTZ_ErrorCode ret;
  int i;

  if (((addr & 0xFF00) == 0x8000) && (addr != 0x8000) && (addr != 0x8004) &&
      (addr != 0x8008)) {  // broadcast access to channel individual registers
    // (loop over channels)
    uint16_t ch;
    for (ch = 0; ch < fpParameters->GetNCh(); ch++) {
      ret = CAEN_DGTZ_ReadRegister(fHandler, 0x1000 | (addr & 0xFF) | (ch << 8),
                                   &reg);
      if (ret != 0) return ret;
      for (i = start_bit; i <= end_bit; i++) mask |= 1 << i;
      reg = reg & ~mask | ((val << start_bit) & mask);
      ret = CAEN_DGTZ_WriteRegister(fHandler,
                                    0x1000 | (addr & 0xFF) | (ch << 8), reg);
      if (ret != 0) return ret;
    }
  } else {  // access to channel individual register or mother board register
    ret = CAEN_DGTZ_ReadRegister(fHandler, addr, &reg);
    if (ret != 0) return ret;
    for (i = start_bit; i <= end_bit; i++) mask |= 1 << i;
    reg = reg & ~mask | ((val << start_bit) & mask);
    ret = CAEN_DGTZ_WriteRegister(fHandler, addr, reg);
    if (ret != 0) return ret;
  }
  return ret;
}

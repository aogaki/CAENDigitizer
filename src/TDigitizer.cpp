#include <iostream>

#include "MyFunctions.hpp"
#include "TDigitizer.hpp"

using std::cerr;
using std::cout;
using std::endl;

TDigitizer::TDigitizer() : fDebugMode(true), fHandler(-1), fParameters(nullptr)
{
}

TDigitizer::TDigitizer(std::string id, CAEN_DGTZ_ConnectionType type, int link,
                       int node, uint32_t VMEadd)
{
  TDigitizer();
  fDeviceID = id;

  OpenDevice(type, link, node, VMEadd);
  GetDeviceInfo();
  fParameters = new TDigiPar(id, fDeviceInfo);  // How to check fDeviceInfo
  fParameters->GenParameter();
}

TDigitizer::~TDigitizer()
{
  DelPointer(fParameters);

  // Now I assume the handler is positive number or 0.
  if (fHandler > -1) CloseDevice();
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

void TDigitizer::ConfigDevice()
{
  // Writing the procedures in this function means I can not understand why
  // need.  Those are copied from digiTES

  ResetDevice();

  SetChMask();

  SetFanSpeed();

  // Channel Control Reg (indiv trg, seq readout) ??
  if (fParameters->GetDigiModel() == 5000) {
    auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8000, 0);
    PrintError(err, "UNKNOWN1");
  } else if ((fParameters->GetDPPType() == DPPType::DPP_PHA_724) ||
             (fParameters->GetDPPType() == DPPType::DPP_nPHA_724)) {
    auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8000, 0x01000114);
    PrintError(err, "UNKNOWN2");
  }

  // Enable new format
  if (fParameters->GetDigiModel() == 751) {
    auto err = RegisterSetBits(0x8000, 31, 31, 1);
    PrintError(err, "UNKNOWN3");
  }

  if (fParameters->GetDPPType() != DPPType::STD_730) {  // only for DPP
    // Acquisition mode (MIXED)
    SetACQMode();

    // Enable Extra Word
    if ((fParameters->GetDigiModel() == 730) ||
        (fParameters->GetDigiModel() == 725) ||
        (fParameters->GetDigiModel() == 751)) {
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1 << 17);
      PrintError(err, "UNKNOWN4");
    }
    // some specific settings in the global CTRL register
    if (true) {  // set trgout bidir
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, (1 << 2));
      PrintError(err, "UNKNOWN5");
    }

    if (fParameters->GetDPPType() ==
        DPPType::DPP_PSD_751) {  // Set new probe mode in x751 models
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1 << 26);
      PrintError(err, "UNKNOWN6");
    }

    if ((fParameters->GetDPPType() == DPPType::DPP_PHA_730) ||
        (fParameters->GetDPPType() == DPPType::DPP_PSD_730) ||
        (fParameters->GetDPPType() ==
         DPPType::DPP_nPHA_724)) {  // Enable auto - flush
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1);
      PrintError(err, "UNKNOWN7");
    }

    if (fParameters->GetDPPType() == DPPType::DPP_PSD_720) {
      // Enable baseline or Extended Time stamp
      auto err = CAEN_DGTZ_WriteRegister(fHandler, 0x8004, 1 << 17);
      PrintError(err, "UNKNOWN8");
      // Enable ext time stamp
      err = RegisterSetBits(0x1080, 7, 7, 1);
      PrintError(err, "UNKNOWN9");
    }
  }

  SetRecordLength(fParameters->GetRecordLength());

  SetIOLevel(fParameters->GetIOLevel());

  // In digiTES, here are some SYNC mode settings.
  // I set Acquisition mode as independent mode.
  auto err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  SetDeviceBuffer();

  SetChParameter();
}

void TDigitizer::ResetDevice()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
}

void TDigitizer::SetChMask()
{
  auto chFlag = fParameters->GetChFlag();
  uint32_t chMask = 0;
  for (auto i = 0; i < chFlag.size(); i++) chMask |= (chFlag[i] << i);
  // cout << "Channel mask: " << chMask << endl;
  auto err = CAEN_DGTZ_SetChannelEnableMask(fHandler, chMask);
  PrintError(err, "SetChannelEnableMask");
}

void TDigitizer::SetFanSpeed()
{
  if (fParameters->GetDigiModel() != 5000) {
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
  auto DPPType = fParameters->GetDPPType();
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
  if (fParameters->GetDPPType() == DPPType::DPP_PHA_730) {
    const auto nCh = fParameters->GetNCh();
    auto offset = fParameters->GetDCOffset();
    auto preTrigger = fParameters->GetPreTrigger();

    auto TTFSmoothing = fParameters->GetTTFSmoothing();
    auto TTFDelay = fParameters->GetTTFDelay();
    auto trapRiseTime = fParameters->GetTrapRiseTime();
    auto trapFlatTop = fParameters->GetTrapFlatTop();
    auto peakingTime = fParameters->GetPeakingTime();
    auto trapPoleZero = fParameters->GetTrapPoleZero();
    auto threshold = fParameters->GetThreshold();
    auto trigHoldOff = fParameters->GetTrgHoldOff();
    auto SHF = fParameters->GetSHF();
    auto NSPeak = fParameters->GetNSPeak();
    auto polarity = fParameters->GetPolarity();
    auto trapNSBaseline = fParameters->GetTrapNSBaseline();
    auto decimation = fParameters->GetDecimation();
    auto fakeEventFlag = fParameters->GetFakeEventFlag();
    auto trapCFDFraction = fParameters->GetTrapCFDFraction();
    auto dynamicRange = fParameters->GetDynamicRange();

    auto trapNorm = fParameters->GetTrapNorm();
    for (auto iCh = 0; iCh < nCh; iCh++) {
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
      // Need WDcfg.ZeroVoltLevel[brd][i]?
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
    for (ch = 0; ch < fParameters->GetNCh(); ch++) {
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

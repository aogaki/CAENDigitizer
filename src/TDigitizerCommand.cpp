#include <iostream>

#include "TDigitizerCommand.hpp"

TDigitizerCommand::TDigitizerCommand()
    : fHandler(-1),
      fpReadoutBuffer(nullptr),
      fpEventPtr(nullptr),
      fpEventStd(nullptr),
      fMaxBufferSize(0),
      fBufferSize(0),
      fNEvents(0),
      fReadSize(0),
      fDigitizerModel(0),
      fNChs(0),
      fTSample(0),
      fNBits(0)
{
  fConverter = new TEveConverter();
}

TDigitizerCommand::TDigitizerCommand(CAEN_DGTZ_ConnectionType type, int link,
                                     int node, uint32_t VMEadd)
    : TDigitizerCommand()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetInfo();
}

TDigitizerCommand::~TDigitizerCommand()
{
  delete fConverter;
  // ClearData();
  Reset();
  Close();
}

void TDigitizerCommand::BoardCalibration()
{
  if ((fDigitizerModel == 730) || (fDigitizerModel == 725)) {
    // Copy from digiTes
    // Honestly, I do not know what is this.
    uint32_t lock, ctrl;
    int ret = 0;
    for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
      // enter engineering functions
      ret |= WriteSPIRegister(iCh, 0x7A, 0x59);
      ret |= WriteSPIRegister(iCh, 0x7A, 0x1A);
      ret |= WriteSPIRegister(iCh, 0x7A, 0x11);
      ret |= WriteSPIRegister(iCh, 0x7A, 0xAC);

      // read lock value
      ret |= ReadSPIRegister(iCh, 0xA7, lock);
      // write lock value
      ret |= WriteSPIRegister(iCh, 0xA5, lock);

      // enable lock
      ret |= ReadSPIRegister(iCh, 0xA4, ctrl);
      ctrl |= 0x4;  // set bit 2
      ret |= WriteSPIRegister(iCh, 0xA4, ctrl);

      ret |= ReadSPIRegister(iCh, 0xA4, ctrl);
      // return ret;
    }
  } else if (fDigitizerModel == 751) {
    Calibrate();
  }
}

void TDigitizerCommand::Open(CAEN_DGTZ_ConnectionType type, int link, int node,
                             uint32_t VMEadd)
{
  auto err = CAEN_DGTZ_OpenDigitizer(type, link, node, VMEadd, &fHandler);
  PrintError(err, "OpenDigitizer");
  HandlerCheck();
}

void TDigitizerCommand::Close()
{
  auto err = CAEN_DGTZ_CloseDigitizer(fHandler);
  PrintError(err, "CloseDigitizer");
}

void TDigitizerCommand::WriteRegister(uint32_t address, uint32_t data)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, address, data);
  PrintError(err, "WriteRegister");
}

CAEN_DGTZ_ErrorCode TDigitizerCommand::WriteSPIRegister(uint32_t ch,
                                                        uint32_t address,
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

uint32_t TDigitizerCommand::ReadRegister(uint32_t address)
{
  uint32_t data;
  auto err = CAEN_DGTZ_ReadRegister(fHandler, address, &data);
  PrintError(err, "ReadRegister");
  return data;
}

CAEN_DGTZ_ErrorCode TDigitizerCommand::ReadSPIRegister(uint32_t ch,
                                                       uint32_t address,
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

void TDigitizerCommand::Reset()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
}

void TDigitizerCommand::GetInfo()
{
  CAEN_DGTZ_BoardInfo_t info;
  auto err = CAEN_DGTZ_GetInfo(fHandler, &info);
  PrintError(err, "GetInfo");

  fNChs = info.Channels;

  std::cout << "Model name:\t" << info.ModelName << "\n"
            << "Model number:\t" << info.Model << "\n"
            << "No. channels:\t" << info.Channels << "\n"
            << "Format factor:\t" << info.FormFactor << "\n"
            << "Family code:\t" << info.FamilyCode << "\n"
            << "Firmware revision of the FPGA on the mother board (ROC):\t"
            << info.ROC_FirmwareRel << "\n"
            << "Firmware revision of the FPGA on the daughter board (AMC):\t"
            << info.AMC_FirmwareRel << "\n"
            << "Serial number:\t" << info.SerialNumber << "\n"
            << "PCB revision:\t" << info.PCB_Revision << "\n"
            << "No. bits of the ADC:\t" << info.ADC_NBits << "\n"
            << "Device handler of CAENComm:\t" << info.CommHandle << "\n"
            << "Device handler of CAENVME:\t" << info.VMEHandle << "\n"
            << "License number:\t" << info.License << std::endl;

  // Copy from digites
  if (info.FamilyCode == 5) {
    fDigitizerModel = 751;
    fTSample = 1;
    fNBits = 10;
  } else if (info.FamilyCode == 7) {
    fDigitizerModel = 780;
    fTSample = 10;
    fNBits = 14;
  } else if (info.FamilyCode == 13) {
    fDigitizerModel = 781;
    fTSample = 10;
    fNBits = 14;
  } else if (info.FamilyCode == 0) {
    fDigitizerModel = 724;
    fTSample = 10;
    fNBits = 14;
  } else if (info.FamilyCode == 11) {
    fDigitizerModel = 730;
    fTSample = 2;
    fNBits = 14;
  } else if (info.FamilyCode == 14) {
    fDigitizerModel = 725;
    fTSample = 4;
    fNBits = 14;
  } else if (info.FamilyCode == 3) {
    fDigitizerModel = 720;
    fTSample = 4;
    fNBits = 12;
  } else if (info.FamilyCode == 999) {  // temporary code for Hexagon
    fDigitizerModel = 5000;
    fTSample = 10;
    fNBits = 14;
  } else {
    PrintError(err, "Check Family code @ GetInfo");
  }

  std::cout << "Digitizer model:\t" << fDigitizerModel << "\n"
            << "Time sample length:\t" << fTSample << "\n"
            << "Wave form resolution:\t" << fNBits << std::endl;
}

void TDigitizerCommand::ClearData()
{
  auto err = CAEN_DGTZ_ClearData(fHandler);
  PrintError(err, "ClearData");
}

void TDigitizerCommand::DisEveAlignedRead()
{
  auto err = CAEN_DGTZ_DisableEventAlignedReadout(fHandler);
  PrintError(err, "DisEveAlignedRead");
}

void TDigitizerCommand::SetMaxNEventsBLT(uint32_t nEve)
{
  auto err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, nEve);
  PrintError(err, "SetMaxNEventsBLT");
}

uint32_t TDigitizerCommand::GetMaxNEventsBLT()
{
  uint32_t nEve;
  auto err = CAEN_DGTZ_GetMaxNumEventsBLT(fHandler, &nEve);
  PrintError(err, "GetMaxNEventsBLT");
  return nEve;
}

void TDigitizerCommand::MallocReadoutBuffer()
{
  auto err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
                                           &fMaxBufferSize);
  PrintError(err, "MallocReadoutBuffer");
}

void TDigitizerCommand::FreeReadoutBuffer()
{
  if (fpReadoutBuffer) {
    auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
    PrintError(err, "FreeReadoutBuffer");
  }
}

void TDigitizerCommand::ReadData()
{
  auto err =
      CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                         fpReadoutBuffer, &fBufferSize);
  PrintError(err, "ReadData");
}

void TDigitizerCommand::GetNumEvents()
{
  // Check size == 0 or not?
  auto err =
      CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, fBufferSize, &fNEvents);
  PrintError(err, "GetNumEvents");
  std::cout << fNEvents << " Events" << std::endl;
}

void TDigitizerCommand::GetEventInfo(int32_t nEve)
{
  auto err = CAEN_DGTZ_GetEventInfo(fHandler, fpReadoutBuffer, fBufferSize,
                                    nEve, &fEventInfo, &fpEventPtr);
  PrintError(err, "GetEventInfo");
  // Something about printing the fpEventInfo NYI
  // std::cout << "Event number:\t" << nEve << '\n'
  //           << "Event size:\t" << fEventInfo.EventSize << '\n'
  //           << "Board ID:\t" << fEventInfo.BoardId << '\n'
  //           << "Pattern:\t" << fEventInfo.Pattern << '\n'
  //           << "Ch mask:\t" << fEventInfo.ChannelMask << '\n'
  //           << "Event counter:\t" << fEventInfo.EventCounter << '\n'
  //           << "Trigger time tag:\t" << fEventInfo.TriggerTimeTag <<
  //           std::endl;
}

void TDigitizerCommand::DecodeEvent()
{
  auto err = CAEN_DGTZ_DecodeEvent(fHandler, fpEventPtr, (void **)&fpEventStd);
  PrintError(err, "DecodeEvent");
  // std::cout << fpEventStd->ChSize[0] << std::endl;
  // for (uint32_t i = 0; i < fpEventStd->ChSize[0]; i++)
  //   std::cout << (fpEventStd->DataChannel[0])[i] << " ";
  //
  // std::cout << std::endl;
}

void TDigitizerCommand::AllocateEvent()
{
  auto err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "AllocateEvent");
}

void TDigitizerCommand::FreeEvent()
{
  auto err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "FreeEvent");
}

void TDigitizerCommand::Calibrate()
{
  auto err = CAEN_DGTZ_Calibrate(fHandler);
  PrintError(err, "Calibrate");
}

uint32_t TDigitizerCommand::ReadTemperature(int32_t ch)
{
  uint32_t temp;
  auto err = CAEN_DGTZ_ReadTemperature(fHandler, ch, &temp);
  PrintError(err, "ReadTemperature");

  return temp;
}

void TDigitizerCommand::SendSWTrigger()
{
  auto err = CAEN_DGTZ_SendSWtrigger(fHandler);
  PrintError(err, "SendSWTrigger");
}

void TDigitizerCommand::SetSWTriggerMode(CAEN_DGTZ_TriggerMode_t mode)
{
  auto err = CAEN_DGTZ_SetSWTriggerMode(fHandler, mode);
  PrintError(err, "SetSWTriggerMode");
}

CAEN_DGTZ_TriggerMode_t TDigitizerCommand::GetSWTriggerMode()
{
  CAEN_DGTZ_TriggerMode_t mode;
  auto err = CAEN_DGTZ_GetSWTriggerMode(fHandler, &mode);
  PrintError(err, "GetSWTriggerMode");

  return mode;
}

void TDigitizerCommand::SetExtTriggerInputMode(CAEN_DGTZ_TriggerMode_t mode)
{
  auto err = CAEN_DGTZ_SetExtTriggerInputMode(fHandler, mode);
  PrintError(err, "SetExtTriggerMode");
}

CAEN_DGTZ_TriggerMode_t TDigitizerCommand::GetExtTriggerInputMode()
{
  CAEN_DGTZ_TriggerMode_t mode;
  auto err = CAEN_DGTZ_GetExtTriggerInputMode(fHandler, &mode);
  PrintError(err, "GetExtTriggerMode");

  return mode;
}

void TDigitizerCommand::SetChannelSelfTrigger(CAEN_DGTZ_TriggerMode_t mode,
                                              uint32_t chMask)
{
  auto err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, mode, chMask);
  PrintError(err, "SetChannelSelfTrigger");
}

CAEN_DGTZ_TriggerMode_t TDigitizerCommand::GetChannelSelfTrigger(uint32_t ch)
{
  CAEN_DGTZ_TriggerMode_t mode;
  auto err = CAEN_DGTZ_GetChannelSelfTrigger(fHandler, ch, &mode);
  PrintError(err, "GetChannelSelfTrigger");

  return mode;
}

void TDigitizerCommand::SetChannelTriggerThreshold(uint32_t ch, uint32_t th)
{
  auto err = CAEN_DGTZ_SetChannelTriggerThreshold(fHandler, ch, th);
  PrintError(err, "SetChannelTriggerThreshold");
}

uint32_t TDigitizerCommand::GetChannelTriggerThreshold(uint32_t ch)
{
  uint32_t th;
  auto err = CAEN_DGTZ_GetChannelTriggerThreshold(fHandler, ch, &th);
  PrintError(err, "GetChannelTriggerThreshold");

  return th;
}

void TDigitizerCommand::SetRunSynchronizationMode(CAEN_DGTZ_RunSyncMode_t mode)
{
  auto err = CAEN_DGTZ_SetRunSynchronizationMode(fHandler, mode);
  PrintError(err, "SetRunSynchronizationMode");
}

CAEN_DGTZ_RunSyncMode_t TDigitizerCommand::GetRunSynchronizationMode()
{
  CAEN_DGTZ_RunSyncMode_t mode;
  auto err = CAEN_DGTZ_GetRunSynchronizationMode(fHandler, &mode);
  PrintError(err, "GetRunSynchronizationMode");

  return mode;
}

void TDigitizerCommand::SetIOLevel(CAEN_DGTZ_IOLevel_t level)
{
  auto err = CAEN_DGTZ_SetIOLevel(fHandler, level);
  PrintError(err, "SetIOLevel");
}

CAEN_DGTZ_IOLevel_t TDigitizerCommand::GetIOLevel()
{
  CAEN_DGTZ_IOLevel_t level;
  auto err = CAEN_DGTZ_GetIOLevel(fHandler, &level);
  PrintError(err, "GetIOLevel");

  return level;
}

void TDigitizerCommand::SetTriggerPolarity(CAEN_DGTZ_TriggerPolarity_t pol,
                                           uint32_t ch)
{
  auto err = CAEN_DGTZ_SetTriggerPolarity(fHandler, ch, pol);
  PrintError(err, "SetTriggerPolarity");
}

CAEN_DGTZ_TriggerPolarity_t TDigitizerCommand::GetTriggerPolarity(uint32_t ch)
{
  CAEN_DGTZ_TriggerPolarity_t pol;
  auto err = CAEN_DGTZ_GetTriggerPolarity(fHandler, ch, &pol);
  PrintError(err, "GetTriggerPolarity");

  return pol;
}

void TDigitizerCommand::SetChannelEnableMask(uint32_t mask)
{
  auto err = CAEN_DGTZ_SetChannelEnableMask(fHandler, mask);
  PrintError(err, "SetChannelEnableMask");
}

uint32_t TDigitizerCommand::GetChannelEnableMask()
{
  uint32_t mask;
  auto err = CAEN_DGTZ_GetChannelEnableMask(fHandler, &mask);
  PrintError(err, "GetChannelEnableMask");

  return mask;
}

void TDigitizerCommand::SWStartAcquisition()
{
  auto err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "SWStartAcquisition");
}

void TDigitizerCommand::SWStopAcquisition()
{
  auto err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "SWStopAcquisition");
}

void TDigitizerCommand::SetRecordLength(uint32_t size)
{
  auto err = CAEN_DGTZ_SetRecordLength(fHandler, size);
  PrintError(err, "SetRecordLength");
}

uint32_t TDigitizerCommand::GetRecordLength()
{
  uint32_t size;
  auto err = CAEN_DGTZ_GetRecordLength(fHandler, &size);
  PrintError(err, "GetRecordLength");

  return size;
}

void TDigitizerCommand::SetPostTriggerSize(uint32_t percent)
{
  auto err = CAEN_DGTZ_SetPostTriggerSize(fHandler, percent);
  PrintError(err, "SetPostTriggerSize");
}

uint32_t TDigitizerCommand::GetPostTriggerSize()
{
  uint32_t percent;
  auto err = CAEN_DGTZ_GetPostTriggerSize(fHandler, &percent);
  PrintError(err, "GetPostTriggerSize");

  return percent;
}

void TDigitizerCommand::SetAcquisitionMode(CAEN_DGTZ_AcqMode_t mode)
{
  auto err = CAEN_DGTZ_SetAcquisitionMode(fHandler, mode);
  PrintError(err, "SetAcquisitionMode");
}

CAEN_DGTZ_AcqMode_t TDigitizerCommand::GetAcquisitionMode()
{
  CAEN_DGTZ_AcqMode_t mode;
  auto err = CAEN_DGTZ_GetAcquisitionMode(fHandler, &mode);
  PrintError(err, "GetAcquisitionMode");

  return mode;
}

void TDigitizerCommand::SetChannelDCOffset(uint32_t ch, uint32_t offset)
{
  auto err = CAEN_DGTZ_SetChannelDCOffset(fHandler, ch, offset);
  PrintError(err, "SetChannelDCOffset");
}

uint32_t TDigitizerCommand::GetChannelDCOffset(uint32_t ch)
{
  uint32_t offset;
  auto err = CAEN_DGTZ_GetChannelDCOffset(fHandler, ch, &offset);
  PrintError(err, "GetChannelDCOffset");

  return offset;
}

void TDigitizerCommand::SetZeroSuppressionMode(CAEN_DGTZ_ZS_Mode_t mode)
{
  auto err = CAEN_DGTZ_SetZeroSuppressionMode(fHandler, mode);
  PrintError(err, "SetZeroSuppressionMode");
}

CAEN_DGTZ_ZS_Mode_t TDigitizerCommand::GetZeroSuppressionMode()
{
  CAEN_DGTZ_ZS_Mode_t mode;
  auto err = CAEN_DGTZ_GetZeroSuppressionMode(fHandler, &mode);
  PrintError(err, "GetZeroSuppressionMode");

  return mode;
}

void TDigitizerCommand::SetChannelZSParams(uint32_t ch,
                                           CAEN_DGTZ_ThresholdWeight_t weight,
                                           int32_t threshold, int32_t nSamples)
{
  auto err =
      CAEN_DGTZ_SetChannelZSParams(fHandler, ch, weight, threshold, nSamples);
  PrintError(err, "SetChannelZSParams");
}
void TDigitizerCommand::GetChannelZSParams(uint32_t ch,
                                           CAEN_DGTZ_ThresholdWeight_t &weight,
                                           int32_t &threshold,
                                           int32_t &nSamples)
{
  auto err = CAEN_DGTZ_GetChannelZSParams(fHandler, ch, &weight, &threshold,
                                          &nSamples);
  PrintError(err, "GetChannelZSParams");
}

void TDigitizerCommand::SetAnalogMonOutput(
    CAEN_DGTZ_AnalogMonitorOutputMode_t mode)
{
  auto err = CAEN_DGTZ_SetAnalogMonOutput(fHandler, mode);
  PrintError(err, "SetAnalogMonOutput");
}

CAEN_DGTZ_AnalogMonitorOutputMode_t TDigitizerCommand::GetAnalogMonOutput()
{
  CAEN_DGTZ_AnalogMonitorOutputMode_t mode;
  auto err = CAEN_DGTZ_GetAnalogMonOutput(fHandler, &mode);
  PrintError(err, "GetAnalogMonOutput");

  return mode;
}

void TDigitizerCommand::PrintError(const CAEN_DGTZ_ErrorCode &err,
                                   const std::string &funcName)
{
  if (err < 0) {  // 0 is success
    std::cout << "in " << funcName << ", error code = " << err << std::endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

void TDigitizerCommand::HandlerCheck()
{
  // NYI
  std::cout << "Handler: " << fHandler << std::endl;
}

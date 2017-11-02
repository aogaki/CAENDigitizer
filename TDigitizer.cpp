#include <iostream>

#include "TDigitizer.hpp"

TDigitizer::TDigitizer()
    : fHandler(-1),
      fpReadoutBuffer(nullptr),
      fpEventPtr(nullptr),
      fpEventStd(nullptr),
      fMaxBufferSize(0),
      fBufferSize(0),
      fNEvents(0),
      fReadSize(0)
{
}

TDigitizer::~TDigitizer() { Close(); }

TDigitizer::TDigitizer(CAEN_DGTZ_ConnectionType type, int link, int node,
                       uint32_t VMEadd)
    : TDigitizer()
{
  Open(type, link, node, VMEadd);
}
void TDigitizer::Open(CAEN_DGTZ_ConnectionType type, int link, int node,
                      uint32_t VMEadd)
{
  auto err = CAEN_DGTZ_OpenDigitizer(type, link, node, VMEadd, &fHandler);
  PrintError(err, "OpenDigitizer");
  HandlerCheck();
}

void TDigitizer::Close()
{
  auto err = CAEN_DGTZ_CloseDigitizer(fHandler);
  PrintError(err, "CloseDigitizer");
}

void TDigitizer::WriteRegister(uint32_t address, uint32_t data)
{
  auto err = CAEN_DGTZ_WriteRegister(fHandler, address, data);
  PrintError(err, "WriteRegister");
}

uint32_t TDigitizer::ReadRegister(uint32_t address)
{
  uint32_t data;
  auto err = CAEN_DGTZ_ReadRegister(fHandler, address, &data);
  PrintError(err, "ReadRegister");
  return data;
}

void TDigitizer::Reset()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
}

void TDigitizer::GetInfo()
{
  CAEN_DGTZ_BoardInfo_t info;
  auto err = CAEN_DGTZ_GetInfo(fHandler, &info);
  PrintError(err, "GetInfo");

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
}

void TDigitizer::ClearData()
{
  auto err = CAEN_DGTZ_ClearData(fHandler);
  PrintError(err, "ClearData");
}

void TDigitizer::DisEveAlignedRead()
{
  auto err = CAEN_DGTZ_DisableEventAlignedReadout(fHandler);
  PrintError(err, "DisEveAlignedRead");
}

void TDigitizer::SetMaxNEventsBLT(uint32_t nEve)
{
  auto err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, nEve);
  PrintError(err, "SetMaxNEventsBLT");
}

uint32_t TDigitizer::GetMaxNEventsBLT()
{
  uint32_t nEve;
  auto err = CAEN_DGTZ_GetMaxNumEventsBLT(fHandler, &nEve);
  PrintError(err, "GetMaxNEventsBLT");
  return nEve;
}

void TDigitizer::MallocReadoutBuffer()
{
  auto err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
                                           &fMaxBufferSize);
  PrintError(err, "MallocReadoutBuffer");
}

void TDigitizer::FreeReadoutBuffer()
{
  if (fpReadoutBuffer) {
    auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
    PrintError(err, "FreeReadoutBuffer");
  }
}

void TDigitizer::ReadData()
{
  auto err =
      CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                         fpReadoutBuffer, &fBufferSize);
  PrintError(err, "ReadData");
}

void TDigitizer::GetNEvents()
{
  // Check size == 0 or not?
  auto err =
      CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, fBufferSize, &fNEvents);
  PrintError(err, "GetNEvents");
}

void TDigitizer::GetEventInfo()
{
  CAEN_DGTZ_EventInfo_t eveInfo;
  auto err = CAEN_DGTZ_GetEventInfo(fHandler, fpReadoutBuffer, fBufferSize,
                                    fNEvents, &eveInfo, &fpEventPtr);
  PrintError(err, "GetEventInfo");
  // Something about printing the eveInfo NYI
}

void TDigitizer::DecodeEvent()
{
  auto err = CAEN_DGTZ_DecodeEvent(fHandler, fpEventPtr, (void **)&fpEventStd);
  PrintError(err, "DecodeEvent");
}

void TDigitizer::AllocateEvent()
{
  auto err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "AllocateEvent");
}

void TDigitizer::FreeEvent()
{
  auto err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "FreeEvent");
}

void TDigitizer::Calibrate()
{
  auto err = CAEN_DGTZ_Calibrate(fHandler);
  PrintError(err, "Calibrate");
}

uint32_t TDigitizer::ReadTemperature(int32_t ch)
{
  uint32_t temp;
  auto err = CAEN_DGTZ_ReadTemperature(fHandler, ch, &temp);
  PrintError(err, "ReadTemperature");

  return temp;
}

void TDigitizer::SendSWTrigger()
{
  auto err = CAEN_DGTZ_SendSWtrigger(fHandler);
  PrintError(err, "SendSWTrigger");
}

void TDigitizer::SetSWTriggerMode(CAEN_DGTZ_TriggerMode_t mode)
{
  auto err = CAEN_DGTZ_SetSWTriggerMode(fHandler, mode);
  PrintError(err, "SetSWTriggerMode");
}

CAEN_DGTZ_TriggerMode_t TDigitizer::GetSWTriggerMode()
{
  CAEN_DGTZ_TriggerMode_t mode;
  auto err = CAEN_DGTZ_GetSWTriggerMode(fHandler, &mode);
  PrintError(err, "GetSWTriggerMode");

  return mode;
}

void TDigitizer::SetExtTriggerInputMode(CAEN_DGTZ_TriggerMode_t mode)
{
  auto err = CAEN_DGTZ_SetExtTriggerInputMode(fHandler, mode);
  PrintError(err, "SetExtTriggerMode");
}

CAEN_DGTZ_TriggerMode_t TDigitizer::GetExtTriggerInputMode()
{
  CAEN_DGTZ_TriggerMode_t mode;
  auto err = CAEN_DGTZ_GetExtTriggerInputMode(fHandler, &mode);
  PrintError(err, "GetExtTriggerMode");

  return mode;
}

void TDigitizer::SetChannelSelfTrigger(CAEN_DGTZ_TriggerMode_t mode,
                                       uint32_t chMask)
{
  auto err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, mode, chMask);
  PrintError(err, "SetChannelSelfTrigger");
}

CAEN_DGTZ_TriggerMode_t TDigitizer::GetChannelSelfTrigger(uint32_t ch)
{
  CAEN_DGTZ_TriggerMode_t mode;
  auto err = CAEN_DGTZ_GetChannelSelfTrigger(fHandler, ch, &mode);
  PrintError(err, "GetChannelSelfTrigger");

  return mode;
}

void TDigitizer::SetChannelTriggerThreshold(uint32_t ch, uint32_t th)
{
  auto err = CAEN_DGTZ_SetChannelTriggerThreshold(fHandler, ch, th);
  PrintError(err, "SetChannelTriggerThreshold");
}

uint32_t TDigitizer::GetChannelTriggerThreshold(uint32_t ch)
{
  uint32_t th;
  auto err = CAEN_DGTZ_GetChannelTriggerThreshold(fHandler, ch, &th);
  PrintError(err, "GetChannelTriggerThreshold");

  return th;
}

void TDigitizer::SetRunSynchronizationMode(CAEN_DGTZ_RunSyncMode_t mode)
{
  auto err = CAEN_DGTZ_SetRunSynchronizationMode(fHandler, mode);
  PrintError(err, "SetRunSynchronizationMode");
}

CAEN_DGTZ_RunSyncMode_t TDigitizer::GetRunSynchronizationMode()
{
  CAEN_DGTZ_RunSyncMode_t mode;
  auto err = CAEN_DGTZ_GetRunSynchronizationMode(fHandler, &mode);
  PrintError(err, "GetRunSynchronizationMode");

  return mode;
}

void TDigitizer::SetIOLevel(CAEN_DGTZ_IOLevel_t level)
{
  auto err = CAEN_DGTZ_SetIOLevel(fHandler, level);
  PrintError(err, "SetIOLevel");
}

CAEN_DGTZ_IOLevel_t TDigitizer::GetIOLevel()
{
  CAEN_DGTZ_IOLevel_t level;
  auto err = CAEN_DGTZ_GetIOLevel(fHandler, &level);
  PrintError(err, "GetIOLevel");

  return level;
}

void TDigitizer::SetTriggerPolarity(CAEN_DGTZ_TriggerPolarity_t pol,
                                    uint32_t ch)
{
  auto err = CAEN_DGTZ_SetTriggerPolarity(fHandler, ch, pol);
  PrintError(err, "SetTriggerPolarity");
}

CAEN_DGTZ_TriggerPolarity_t TDigitizer::GetTriggerPolarity(uint32_t ch)
{
  CAEN_DGTZ_TriggerPolarity_t pol;
  auto err = CAEN_DGTZ_GetTriggerPolarity(fHandler, ch, &pol);
  PrintError(err, "GetTriggerPolarity");

  return pol;
}

void TDigitizer::SetChannelEnableMask(uint32_t mask)
{
  auto err = CAEN_DGTZ_SetChannelEnableMask(fHandler, mask);
  PrintError(err, "SetChannelEnableMask");
}

uint32_t TDigitizer::GetChannelEnableMask()
{
  uint32_t mask;
  auto err = CAEN_DGTZ_GetChannelEnableMask(fHandler, &mask);
  PrintError(err, "GetChannelEnableMask");

  return mask;
}

void TDigitizer::SWStartAcquisition()
{
  auto err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "SWStartAcquisition");
}

void TDigitizer::SWStopAcquisition()
{
  auto err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "SWStopAcquisition");
}

void TDigitizer::SetRecordLength(uint32_t size)
{
  auto err = CAEN_DGTZ_SetRecordLength(fHandler, size);
  PrintError(err, "SetRecordLength");
}

uint32_t TDigitizer::GetRecordLength()
{
  uint32_t size;
  auto err = CAEN_DGTZ_GetRecordLength(fHandler, &size);
  PrintError(err, "GetRecordLength");

  return size;
}

void TDigitizer::SetPostTriggerSize(uint32_t percent)
{
  auto err = CAEN_DGTZ_SetPostTriggerSize(fHandler, percent);
  PrintError(err, "SetPostTriggerSize");
}

uint32_t TDigitizer::GetPostTriggerSize()
{
  uint32_t percent;
  auto err = CAEN_DGTZ_GetPostTriggerSize(fHandler, &percent);
  PrintError(err, "GetPostTriggerSize");

  return percent;
}

void TDigitizer::SetAcquisitionMode(CAEN_DGTZ_AcqMode_t mode)
{
  auto err = CAEN_DGTZ_SetAcquisitionMode(fHandler, mode);
  PrintError(err, "SetAcquisitionMode");
}

CAEN_DGTZ_AcqMode_t TDigitizer::GetAcquisitionMode()
{
  CAEN_DGTZ_AcqMode_t mode;
  auto err = CAEN_DGTZ_GetAcquisitionMode(fHandler, &mode);
  PrintError(err, "GetAcquisitionMode");

  return mode;
}

void TDigitizer::SetChannelDCOffset(uint32_t ch, uint32_t offset)
{
  auto err = CAEN_DGTZ_SetChannelDCOffset(fHandler, ch, offset);
  PrintError(err, "SetChannelDCOffset");
}

uint32_t TDigitizer::GetChannelDCOffset(uint32_t ch)
{
  uint32_t offset;
  auto err = CAEN_DGTZ_GetChannelDCOffset(fHandler, ch, &offset);
  PrintError(err, "GetChannelDCOffset");

  return offset;
}

void TDigitizer::SetZeroSuppressionMode(CAEN_DGTZ_ZS_Mode_t mode)
{
  auto err = CAEN_DGTZ_SetZeroSuppressionMode(fHandler, mode);
  PrintError(err, "SetZeroSuppressionMode");
}

CAEN_DGTZ_ZS_Mode_t TDigitizer::GetZeroSuppressionMode()
{
  CAEN_DGTZ_ZS_Mode_t mode;
  auto err = CAEN_DGTZ_GetZeroSuppressionMode(fHandler, &mode);
  PrintError(err, "GetZeroSuppressionMode");

  return mode;
}

void TDigitizer::SetChannelZSParams(uint32_t ch,
                                    CAEN_DGTZ_ThresholdWeight_t weight,
                                    int32_t threshold, int32_t nSamples)
{
  auto err =
      CAEN_DGTZ_SetChannelZSParams(fHandler, ch, weight, threshold, nSamples);
  PrintError(err, "SetChannelZSParams");
}
void TDigitizer::GetChannelZSParams(uint32_t ch,
                                    CAEN_DGTZ_ThresholdWeight_t &weight,
                                    int32_t &threshold, int32_t &nSamples)
{
  auto err = CAEN_DGTZ_GetChannelZSParams(fHandler, ch, &weight, &threshold,
                                          &nSamples);
  PrintError(err, "GetChannelZSParams");
}

void TDigitizer::SetAnalogMonOutput(CAEN_DGTZ_AnalogMonitorOutputMode_t mode)
{
  auto err = CAEN_DGTZ_SetAnalogMonOutput(fHandler, mode);
  PrintError(err, "SetAnalogMonOutput");
}

CAEN_DGTZ_AnalogMonitorOutputMode_t TDigitizer::GetAnalogMonOutput()
{
  CAEN_DGTZ_AnalogMonitorOutputMode_t mode;
  auto err = CAEN_DGTZ_GetAnalogMonOutput(fHandler, &mode);
  PrintError(err, "GetAnalogMonOutput");

  return mode;
}

void TDigitizer::PrintError(const CAEN_DGTZ_ErrorCode &err,
                            const std::string &funcName)
{
  if (err < 0) {  // 0 is success
    std::cout << "in " << funcName << ", error code = " << err << std::endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

void TDigitizer::HandlerCheck()
{
  // NYI
  std::cout << "Handler: " << fHandler << std::endl;
}

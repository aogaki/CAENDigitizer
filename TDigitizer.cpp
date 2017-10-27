#include <iostream>

#include "TDigitizer.hpp"

TDigitizer::TDigitizer() {}

TDigitizer::~TDigitizer()
{
  FreeReadoutBuffer();
  Close();
}

void TDigitizer::Open(CAEN_DGTZ_ConnectionType type, int link, int node,
                      uint32_t VMEadd)
{
  auto err = CAEN_DGTZ_OpenDigitizer(type, link, node, VMEadd, &fHandler);
  PrintError(err, "OpenDigitizer");
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

void TDigitizer::PrintError(CAEN_DGTZ_ErrorCode err, std::string funcName)
{
  if (err < 0) {  // 0 is success
    std::cout << "in " << funcName << ", error code = " << err << std::endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

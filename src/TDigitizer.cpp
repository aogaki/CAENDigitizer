#include <iostream>

#include "MyFunctions.hpp"
#include "TDigitizer.hpp"

TDigitizer::TDigitizer() : fHandler(-1), fParameters(nullptr) {}

TDigitizer::TDigitizer(std::string id, CAEN_DGTZ_ConnectionType type, int link,
                       int node, uint32_t VMEadd)
{
  TDigitizer();
  fDeviceID = id;

  // Get digitizer info -> Get parameters -> Check consistency
  OpenDevice(type, link, node, VMEadd);
  GetDeviceInfo();
  fParameters = new TDigiPar(id, fDeviceInfo);  // How to check fDeviceInfo
  fParameters->test();
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
    std::cout << "Can not open the device!  Check the connection" << std::endl;
    exit(0);
  }
}

void TDigitizer::GetDeviceInfo()
{
  auto err = CAEN_DGTZ_GetInfo(fHandler, &fDeviceInfo);
  PrintError(err, "GetInfo");

  std::cout << "Model name:\t" << fDeviceInfo.ModelName << "\n"
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
            << "License number:\t" << fDeviceInfo.License << std::endl;
}

void TDigitizer::PrintError(const CAEN_DGTZ_ErrorCode &err,
                            const std::string &funcName)
{
  if (err < 0) {  // 0 is success
    std::cout << "In " << funcName << ", error code = " << err << std::endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

void TDigitizer::ConfigDevice() { ResetDevice(); }

void TDigitizer::ResetDevice()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
}

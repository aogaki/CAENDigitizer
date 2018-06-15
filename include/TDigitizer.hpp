#ifndef TDIGITIZER_HPP
#define TDIGITIZER_HPP 1

// CAEN digitizer handling superclass.  Only for DPP (PHA or PSD) firmware.
// The difference between PHA and PSD is solved by subclasses

#include <string>

// CAEN libraries
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#include "TDigiPar.hpp"

class TDigitizer
{
 public:
  TDigitizer();
  TDigitizer(std::string id, CAEN_DGTZ_ConnectionType type, int link,
             int node = 0, uint32_t VMEadd = 0);
  virtual ~TDigitizer();

  void ConfigDevice();

 private:
  std::string fDeviceID;
  int fHandler;
  CAEN_DGTZ_BoardInfo_t fDeviceInfo;

  TDigiPar *fParameters;

  void ResetDevice();
  void SetChMask();

  // Called from constructor
  void OpenDevice(CAEN_DGTZ_ConnectionType type, int link, int node,
                  uint32_t VMEadd);
  void GetDeviceInfo();

  // Called from destructor
  void CloseDevice();

  void PrintError(const CAEN_DGTZ_ErrorCode &err, const std::string &funcName);
};

#endif

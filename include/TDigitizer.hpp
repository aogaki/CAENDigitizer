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
  bool fDebugMode;
  std::string fDeviceID;
  int fHandler;
  CAEN_DGTZ_BoardInfo_t fDeviceInfo;

  TDigiPar *fParameters;

  // Functions for devicie controll
  void ResetDevice();
  void SetChMask();
  void SetFanSpeed();
  void SetACQMode();
  void SetRecordLength(uint32_t size);
  void SetIOLevel(CAEN_DGTZ_IOLevel_t level);
  void SetDeviceBuffer();
  void SetChParameter();

  // Some functions for channel parameter setting
  // Only for readable
  void SetTrapNorm(uint ch, uint val);
  void SetPreTrigger(uint ch, uint val);
  void SetTTFSmoothing(uint ch, uint val);
  void SetTTFDelay(uint ch, uint val);
  void SetTrapRiseTime(uint ch, uint val);
  void SetTrapFlatTop(uint ch, uint val);
  void SetPeakingTime(uint ch, uint val);
  void SetTrapPoleZero(uint ch, uint val);
  void SetThreshold(uint ch, uint val);
  void SetTimeValWin(uint ch, uint val = 0);
  void SetTrgHoldOff(uint ch, uint val);
  void SetPeakHoldOff(uint ch, uint val = 8);
  void SetBaselineHoldOff(uint ch, uint val = 4);
  void SetSHF(uint ch, uint val);
  void SetNSPeak(uint ch, uint val);
  void SetPolarity(uint ch, uint val);
  void SetTrapNSBaseline(uint ch, uint val);
  void SetDecimation(uint ch, uint val);
  void SetFakeEventFlag(uint ch, uint val);
  void SetDiscr(uint ch, uint val);
  void SetDynamicRange(uint ch, uint val);
  void SetExtraWord(uint ch, uint val = 2);
  // void Set(uint ch, uint val);
  // void Set(uint ch, uint val);

  // Called from constructor
  void OpenDevice(CAEN_DGTZ_ConnectionType type, int link, int node,
                  uint32_t VMEadd);
  void GetDeviceInfo();

  // Called from destructor
  void CloseDevice();

  CAEN_DGTZ_ErrorCode RegisterSetBits(uint16_t addr, int start_bit, int end_bit,
                                      int val);

  void PrintError(const CAEN_DGTZ_ErrorCode &err, const std::string &funcName);
};

#endif

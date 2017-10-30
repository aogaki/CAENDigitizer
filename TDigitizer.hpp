// Test (interface) class of CAEN Digitizer libraries
// One member function is assigned one library function

#ifndef TDigitizer_hpp
#define TDigitizer_hpp 1

#include <string>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

// class TZSParams
// {
//  public:
//   TZSParams() : fWeight(CAEN_DGTZ_ZS_FINE), fThreshold(0), fNSamples(0){};
//   ~TZSParams(){};
//
//   CAEN_DGTZ_ThresholdWeight_t fWeight;
//   int32_t fThreshold;
//   int32_t fNSamples;
// };

class TDigitizer
{
 public:
  TDigitizer();
  virtual ~TDigitizer();

  // This class inculdes all CAENDigitizer Library functions.
  // If functions are only for the one type or family,
  // Those functions are implemented in sub classes.
  // Its should be protected and used from UI class's public functions.
  // Now, in the early studying stage, its are public.

  // Communication functions
  void Open(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd);
  void Close();

  uint32_t ReadRegister(uint32_t address);
  void WriteRegister(uint32_t address, uint32_t data);

  void Reset();

  void GetInfo();

  void ClearData();

  void DisEveAlignedRead();

  void SetMaxNEventsBLT(uint32_t nEve);
  uint32_t GetMaxNEventsBLT();

  void MallocReadoutBuffer();
  void FreeReadoutBuffer();

  void ReadData();
  void GetNEvents();
  void GetEventInfo();
  void DecodeEvent();
  void AllocateEvent();
  void FreeEvent();

  void Calibrate();

  uint32_t ReadTemperature(int32_t ch);

  // Trigger functions
  void SendSWTrigger();

  void SetSWTriggerMode(CAEN_DGTZ_TriggerMode_t mode);
  CAEN_DGTZ_TriggerMode_t GetSWTriggerMode();

  void SetExtTriggerInputMode(CAEN_DGTZ_TriggerMode_t mode);
  CAEN_DGTZ_TriggerMode_t GetExtTriggerInputMode();

  void SetChannelSelfTrigger(CAEN_DGTZ_TriggerMode_t mode, uint32_t chMask);
  CAEN_DGTZ_TriggerMode_t GetChannelSelfTrigger(uint32_t ch);

  void SetChannelTriggerThreshold(uint32_t ch, uint32_t th);
  uint32_t GetChannelTriggerThreshold(uint32_t ch);

  void SetRunSynchronizationMode(CAEN_DGTZ_RunSyncMode_t mode);
  CAEN_DGTZ_RunSyncMode_t GetRunSynchronizationMode();

  void SetIOLevel(CAEN_DGTZ_IOLevel_t level);
  CAEN_DGTZ_IOLevel_t GetIOLevel();

  void SetTriggerPolarity(CAEN_DGTZ_TriggerPolarity_t pol, uint32_t ch);
  CAEN_DGTZ_TriggerPolarity_t GetTriggerPolarity(uint32_t ch);

  // Acquisition functions
  void SetChannelEnableMask(uint32_t mask);
  uint32_t GetChannelEnableMask();

  void SWStartAcquisition();
  void SWStopAcquisition();

  void SetRecordLength(uint32_t size);
  uint32_t GetRecordLength();

  void SetPostTriggerSize(uint32_t percent);
  uint32_t GetPostTriggerSize();

  void SetAcquisitionMode(CAEN_DGTZ_AcqMode_t mode);
  CAEN_DGTZ_AcqMode_t GetAcquisitionMode();

  void SetChannelDCOffset(uint32_t ch, uint32_t offset);
  uint32_t GetChannelDCOffset(uint32_t ch);

  void SetZeroSuppressionMode(CAEN_DGTZ_ZS_Mode_t mode);
  CAEN_DGTZ_ZS_Mode_t GetZeroSuppressionMode();

  void SetChannelZSParams(uint32_t ch, CAEN_DGTZ_ThresholdWeight_t weight,
                          int32_t threshold, int32_t nSamples);
  void GetChannelZSParams(uint32_t ch, CAEN_DGTZ_ThresholdWeight_t &weight,
                          int32_t &threshold, int32_t &nSamples);

  void SetAnalogMonOutput(CAEN_DGTZ_AnalogMonitorOutputMode_t mode);
  CAEN_DGTZ_AnalogMonitorOutputMode_t GetAnalogMonOutput();

 private:
  int fHandler{-1};  // What is the possible region of handler?
  char *fpReadoutBuffer{nullptr};
  char *fpEventPtr{nullptr};
  CAEN_DGTZ_UINT16_EVENT_t *fpEventStd{nullptr};  // for void **Eve
  uint32_t fMaxBufferSize{0};
  uint32_t fBufferSize{0};
  uint32_t fNEvents{0};
  uint32_t fReadSize{0};

  void PrintError(const CAEN_DGTZ_ErrorCode &err, const std::string &funcName);
  void HandlerCheck();
};

#endif

#ifndef TWaveRecord_hpp
#define TWaveRecord_hpp 1

// For the standard fiemware digitizer
// This will be super class of other firmware and model family

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

#include <string>
#include <vector>

#include "TDigitizer.hpp"
#include "TStdData.hpp"
#include "TWaveRecordData.hpp"
#include "TWaveRecordPar.hpp"

class TWaveRecord : public TDigitizer
{
 public:
  TWaveRecord();
  TWaveRecord(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
              uint32_t VMEadd = 0);
  virtual ~TWaveRecord();

  void Initialize();

  void ReadEvents();

  CAEN_DGTZ_ErrorCode StartAcquisition();
  void StopAcquisition();

  // const uint32_t GetNEvents() { return fEveCounter; };

  void SetParameter(TWaveRecordPar par);

  std::vector<WaveFormData_t> *GetData() { return fData; };

 protected:
  // For event readout
  char *fpReadoutBuffer;
  char *fpEventPtr;
  CAEN_DGTZ_EventInfo_t fEventInfo;
  CAEN_DGTZ_UINT16_EVENT_t *fpEventStd;  // for void **Eve
  uint32_t fMaxBufferSize;
  uint32_t fBufferSize;

  // Parameters
  TWaveRecordPar fParameters;

  // Data
  std::vector<WaveFormData_t> *fData;

  uint64_t fTimeOffset;
  uint64_t fPreviousTime;

  void InitParameters();

  void AcquisitionConfig();
  void TriggerConfig();
};

#endif

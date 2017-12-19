#ifndef TDPP_hpp
#define TDPP_hpp 1

#include "TDigitizer.hpp"

class TDPP : public TDigitizer
{
 public:
  TDPP();
  TDPP(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
       uint32_t VMEadd = 0);
  ~TDPP();

  void Initialize();

  void ReadEvents();

  CAEN_DGTZ_ErrorCode StartAcquisition();
  void StopAcquisition();

 private:
  virtual void SetParameters();

  virtual void AcquisitionConfig();
  virtual void TriggerConfig();

  void SetPHAPar();
  void SetPSDPar();
  CAEN_DGTZ_DPP_PHA_Params_t fParPHA;
  CAEN_DGTZ_DPP_PSD_Params_t fParPSD;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;
  uint32_t fPostTriggerSize;
  uint32_t fRecordLength;

  double fVpp;
  double fVth;

  // Memory
  void AllocateMemory();
  void FreeMemory();
  char *fpReadoutBuffer;                         // readout buffer
  CAEN_DGTZ_DPP_PSD_Event_t **fppPSDEvents;      // events buffer
  CAEN_DGTZ_DPP_PSD_Waveforms_t *fpPSDWaveform;  // waveforms buffer

  // Data
  std::vector<uint64_t> fTimeOffset;
  std::vector<uint64_t> fPreviousTime;
  std::vector<uint64_t> fTime;
};

#endif

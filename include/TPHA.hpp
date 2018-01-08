#ifndef TPHA_hpp
#define TPHA_hpp 1

#include "TDigitizer.hpp"

class TPHA : public TDigitizer
{
 public:
  TPHA();
  TPHA(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
       uint32_t VMEadd = 0);
  ~TPHA();

  void Initialize();

  void ReadEvents();

  CAEN_DGTZ_ErrorCode StartAcquisition();
  void StopAcquisition();

  uint32_t GetNEvents() { return fNEvents; };

  void SetMaster();
  void SetSlave();
  void StartSyncMode(uint32_t nMods);

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
  uint32_t fBLTEvents;

  // Memory
  void AllocateMemory();
  void FreeMemory();
  char *fpReadoutBuffer;                         // readout buffer
  CAEN_DGTZ_DPP_PHA_Event_t **fppPHAEvents;      // events buffer
  CAEN_DGTZ_DPP_PHA_Waveforms_t *fpPHAWaveform;  // waveforms buffer

  // Data
  std::vector<uint64_t> fTimeOffset;
  std::vector<uint64_t> fPreviousTime;
  std::vector<uint64_t> fTime;

  uint32_t fNEvents;
};

#endif

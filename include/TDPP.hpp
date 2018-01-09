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

  uint32_t GetNEvents() { return fNEvents; };

  void SetMaster();
  void SetSlave();
  void StartSyncMode(uint32_t nMods);

 protected:
  virtual void SetParameters();

  virtual void AcquisitionConfig();
  virtual void TriggerConfig();

  uint32_t fBLTEvents;

  // Memory
  void AllocateMemory();
  void FreeMemory();
  char *fpReadoutBuffer;  // readout buffer

  // Data

  uint32_t fNEvents;
};

#endif

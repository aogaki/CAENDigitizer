#ifndef TPSD_hpp
#define TPSD_hpp 1

#include <vector>

#include "TDPP.hpp"
#include "TPSDData.hpp"
#include "TPSDPar.hpp"

class TPSD : public TDPP
{
 public:
  TPSD();
  TPSD(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
       uint32_t VMEadd = 0);
  ~TPSD();

  void Initialize();

  void ReadEvents();

 private:
  void InitParameters();

  void AcquisitionConfig();
  void TriggerConfig();

  // Parameters
  TPSDPar fParameters;
  void SetPSDPar();
  CAEN_DGTZ_DPP_PSD_Params_t fParPSD;

  // Memory
  void AllocateMemory();
  void FreeMemory();
  char *fpReadoutBuffer;                         // readout buffer
  CAEN_DGTZ_DPP_PSD_Event_t **fppPSDEvents;      // events buffer
  CAEN_DGTZ_DPP_PSD_Waveforms_t *fpPSDWaveform;  // waveforms buffer
};

#endif

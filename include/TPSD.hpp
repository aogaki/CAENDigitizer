#ifndef TPSD_hpp
#define TPSD_hpp 1

#include <vector>

#include "TDPP.hpp"

class TPSD : public TDPP
{
 public:
  TPSD();
  TPSD(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
       uint32_t VMEadd = 0);
  ~TPSD();

  void Initialize();

  void ReadEvents();
  void ReadADC(std::vector<uint> &adc);

  void SetChMask(uint mask) { fChMask = mask; };

  void LoadPSDPar(CAEN_DGTZ_DPP_PSD_Params_t par) { fParPSD = par; };

 private:
  virtual void SetParameters();

  void AcquisitionConfig();
  void TriggerConfig();

  void SetPSDPar();
  CAEN_DGTZ_DPP_PSD_Params_t fParPSD;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;
  uint32_t fPostTriggerSize;
  uint32_t fRecordLength;

  // double fVpp;
  uint32_t fBLTEvents;

  // Memory
  void AllocateMemory();
  void FreeMemory();
  char *fpReadoutBuffer;                         // readout buffer
  CAEN_DGTZ_DPP_PSD_Event_t **fppPSDEvents;      // events buffer
  CAEN_DGTZ_DPP_PSD_Waveforms_t *fpPSDWaveform;  // waveforms buffer

  // Data
  void ReadPar();
  double fDCOffset;
  int fVth;
  uint fChMask;
  CAEN_DGTZ_PulsePolarity_t fPolarity;
};

#endif

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

  void StartAcquisition();
  void StopAcquisition();

  const std::vector<int32_t> *GetCharge() { return fCharge; };
  const std::vector<uint64_t> *GetTime() { return fTime; };

 private:
  std::vector<int32_t> *fCharge;
  std::vector<uint64_t> *fTime;

  virtual void SetParameters();

  virtual void AcquisitionConfig();
  virtual void TriggerConfig();

  CAEN_DGTZ_DPP_PHA_Params_t fParPHA;
  CAEN_DGTZ_DPP_PSD_Params_t fParPSD;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;
  uint32_t fPostTriggerSize;
  uint32_t fRecordLength;

  double fVpp;
  double fVth;
};

#endif

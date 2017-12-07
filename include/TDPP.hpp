#ifndef TDPP_hpp
#define TDPP_hpp 1

#include "TDigitizer.hpp"

class TDPP : public TDigitizer
{
 public:
  TDPP();
  TDPP(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
       uint32_t VMEadd = 0, bool plotWaveform = false);
  ~TDPP();

  void Initialize();

  void ReadEvents();

  void BoardCalibration();

  void StartAcquisition();
  void StopAcquisition();

  const std::vector<int32_t> *GetCharge() { return fCharge; };
  const std::vector<uint64_t> *GetTime() { return fTime; };

 private:
  std::vector<int32_t> *fCharge;
  std::vector<uint64_t> *fTime;

  void SetParameters();

  void Reset();
};

#endif

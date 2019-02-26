#ifndef TDPP_HPP
#define TDPP_HPP 1

#include "TDigitizer.hpp"

class TDPP : public TDigitizer
{
 public:
  TDPP();
  virtual ~TDPP(){};

  uint32_t GetNEvents() { return fNEvents; };

  CAEN_DGTZ_ErrorCode StartAcquisition();
  void StopAcquisition();

 protected:
  std::vector<uint64_t> fTimeOffset;
  std::vector<uint64_t> fPreviousTime;
  std::vector<uint64_t> fTime;

 private:
  virtual void AcquisitionConfig() = 0;
  virtual void TriggerConfig() = 0;

  virtual void AllocateMemory() = 0;
  virtual void FreeMemory() = 0;
};

#endif

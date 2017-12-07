#ifndef TDPP_hpp
#define TDPP_hpp 1

#include "TDigitizerCommand.hpp"

class TDPP : public TDigitizerCommand
{
 public:
  TDPP();
  TDPP(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
       uint32_t VMEadd = 0);
  ~TDPP();

  // NYI
  void Initialize(){};

  // DPP functions
  void SetDPPPreTriggerSize(int ch, uint32_t samples);
  uint32_t GetDPPPreTriggerSize(int ch);

  void SetChannelPulsePolarity(int ch, CAEN_DGTZ_PulsePolarity_t pol);
  CAEN_DGTZ_PulsePolarity_t GetChannelPulsePolarity(int ch);

 private:
};

#endif

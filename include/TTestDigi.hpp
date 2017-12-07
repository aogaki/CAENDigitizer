#ifndef TTestDigi_hpp
#define TTestDigi_hpp 1

#include "TDigitizerCommand.hpp"

class TTestDigi : public TDigitizerCommand
{
 public:
  TTestDigi();
  TTestDigi(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
            uint32_t VMEadd = 0);
  ~TTestDigi();

  // User interfaces
  void Initialize();
  void ReadEvents();

  // DPP functions
  void SeTTestDigiPreTriggerSize(int ch, uint32_t samples);
  uint32_t GeTTestDigiPreTriggerSize(int ch);

  void SetChannelPulsePolarity(int ch, CAEN_DGTZ_PulsePolarity_t pol);
  CAEN_DGTZ_PulsePolarity_t GetChannelPulsePolarity(int ch);

 private:
  void TriggerAcqConfig();
};

#endif

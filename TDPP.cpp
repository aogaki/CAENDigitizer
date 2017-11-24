#include "TDPP.hpp"

TDPP::TDPP() : TDigitizerCommand() {}

TDPP::TDPP(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd)
    : TDigitizerCommand(type, link, node, VMEadd)
{
}

TDPP::~TDPP() {}

void TDPP::SetDPPPreTriggerSize(int ch, uint32_t samples)
{
  auto err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, ch, samples);
  PrintError(err, "SetDPPPreTriggerSize");
}

uint32_t TDPP::GetDPPPreTriggerSize(int ch)
{
  uint32_t samples;
  auto err = CAEN_DGTZ_GetDPPPreTriggerSize(fHandler, ch, &samples);
  PrintError(err, "GetDPPPreTriggerSize");

  return samples;
}

void TDPP::SetChannelPulsePolarity(int ch, CAEN_DGTZ_PulsePolarity_t pol)
{
  auto err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, ch, pol);
  PrintError(err, "SetChannelPulsePolarity");
}

CAEN_DGTZ_PulsePolarity_t TDPP::GetChannelPulsePolarity(int ch)
{
  CAEN_DGTZ_PulsePolarity_t pol;
  auto err = CAEN_DGTZ_GetChannelPulsePolarity(fHandler, ch, &pol);
  PrintError(err, "GetChannelPulsePolarity");

  return pol;
}

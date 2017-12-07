#include <iostream>

#include "TDPP.hpp"

TDPP::TDPP() : TDigitizer() { SetParameters(); }

TDPP::TDPP(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd,
           bool plotWaveform)
    : TDPP()
{
}

TDPP::~TDPP() {}

void TDPP::SetParameters() {}

void TDPP::Reset() {}

void TDPP::Initialize() {}

void TDPP::ReadEvents() {}

void TDPP::BoardCalibration() {}

void TDPP::StartAcquisition() {}
void TDPP::StopAcquisition() {}

#include <iostream>

#include "TUSBDigitizer.hpp"

// TUSBDigitizer::TUSBDigitizer() : TDigitizer()
// {
//   Open(CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB, 0, 0, 0);
// }

TUSBDigitizer::TUSBDigitizer(int link) : TDigitizer()
{
  Open(CAEN_DGTZ_ConnectionType::CAEN_DGTZ_USB, link, 0, 0);
}

TUSBDigitizer::~TUSBDigitizer() {}

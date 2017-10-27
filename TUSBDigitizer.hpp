// Test class of CAEN USB Digitizer

#ifndef TUSBDigitizer_hpp
#define TUSBDigitizer_hpp 1

#include <string>

#include <CAENDigitizer.h>

#include "TDigitizer.hpp"

class TUSBDigitizer : public TDigitizer
{
 public:
  TUSBDigitizer(int link);
  ~TUSBDigitizer();

 private:
};

#endif

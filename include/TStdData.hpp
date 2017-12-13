#ifndef TStdData_hpp
#define TStdData_hpp 1

#include <CAENDigitizerType.h>

constexpr uint32_t kNSamples = 4096;

class TStdData
{
 public:
  TStdData()
  {
    ModNumber = 0;
    ChNumber = 0;
    TimeStamp = 0;
    ADC = 0;
    for (uint32_t i = 0; i < kNSamples; i++) Waveform[i] = 0;
  };

  ~TStdData(){};

  uint8_t ModNumber;
  uint8_t ChNumber;
  uint32_t TimeStamp;
  int32_t ADC;
  uint16_t Waveform[kNSamples];
};

#endif

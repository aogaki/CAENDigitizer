#ifndef TStdData_hpp
#define TStdData_hpp 1

#include <vector>

#include <CAENDigitizerType.h>

class TStdData
{
 public:
  TStdData()
  {
    ModNumber = 0;
    ChNumber = 0;
    TimeStamp = 0;
    ADC = 0;
  };
  explicit TStdData(int sampleSize) : TStdData()
  {
    Waveform.resize(sampleSize);
  };
  ~TStdData(){};

  uint8_t ModNumber;
  uint8_t ChNumber;
  uint64_t TimeStamp;
  uint32_t ADC;
  std::vector<uint16_t> Waveform;
};

#endif

#ifndef SampleData_h
#define SampleData_h 1

#include "TStdData.hpp"

// constexpr int ONE_HIT_SIZE = 8202;
//constexpr int ONE_HIT_SIZE = sizeof(unsigned char) + sizeof(unsigned char) +
//sizeof(unsigned int) + sizeof(int) +
//(sizeof(unsigned short) * kNSamples);
constexpr int ONE_HIT_SIZE = sizeof(unsigned char) + sizeof(unsigned char) +
  sizeof(unsigned int) + sizeof(int);

struct SampleData {
  unsigned char ModNumber;
  unsigned char ChNumber;
  unsigned int TimeStamp;
  int ADC;
  //unsigned short Waveform[kNSamples];
};

#endif

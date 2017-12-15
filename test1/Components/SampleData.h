#ifndef SampleData_h
#define SampleData_h 1

#include "TStdData.hpp"

constexpr int ONE_HIT_SIZE = sizeof(unsigned char) + sizeof(unsigned char) +
                             sizeof(unsigned long) + sizeof(int) +
                             (sizeof(unsigned short) * kNSamples);

// For data transfer buffer
constexpr int BUFFER_SIZE = 1024 * 1024 * 2;

struct SampleData {
  unsigned char ModNumber;
  unsigned char ChNumber;
  unsigned long TimeStamp;
  int ADC;
  unsigned short Waveform[kNSamples];
};

#endif

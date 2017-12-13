#ifndef SampleData_h
#define SampleData_h 1

const int ONE_HIT_SIZE = 8202;
constexpr int kNSamples = 4096;

struct SampleData {
  unsigned char ModNumber;
  unsigned char ChNumber;
  unsigned int TimeStamp;
  int ADC;
  unsigned short Waveform[kNSamples];
};

#endif

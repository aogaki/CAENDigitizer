#ifndef MYFUNCTIONS_HPP
#define MYFUNCTIONS_HPP 1

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

constexpr unsigned int kNSamples = 1024;
struct SampleData {
  unsigned char ModNumber;
  unsigned char ChNumber;
  unsigned long TimeStamp;
  short ADC;
  unsigned short Waveform[kNSamples];
};

constexpr int ONE_HIT_SIZE = sizeof(unsigned char) + sizeof(unsigned char) +
                             sizeof(unsigned long) + sizeof(short) +
                             (sizeof(unsigned short) * kNSamples);

constexpr int kMaxPacketSize = 2000000 - ONE_HIT_SIZE;

// For data transfer buffer
constexpr int BUFFER_SIZE = 1024 * 1024 * 2 * 16;

#endif

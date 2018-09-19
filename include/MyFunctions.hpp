#ifndef MYFUNCTIONS_HPP
#define MYFUNCTIONS_HPP 1

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

// constexpr unsigned int kNSample = 4096;  // It should be set by parameter
// table
constexpr unsigned int kNSample = 8192;  // It should be set by parameter table
struct SampleData {
  unsigned char ModNumber;
  unsigned char ChNumber;
  unsigned long TimeStamp;
  unsigned short ADC;
  unsigned short Waveform[kNSample];
};

constexpr int ONE_HIT_SIZE = sizeof(unsigned char) + sizeof(unsigned char) +
                             sizeof(unsigned long) + sizeof(short) +
                             (sizeof(unsigned short) * kNSample);

constexpr int kMaxPacketSize = 2000000 - ONE_HIT_SIZE;

// For data transfer buffer
constexpr int BUFFER_SIZE = kNSample * 1024 * 2 * 16;

#endif

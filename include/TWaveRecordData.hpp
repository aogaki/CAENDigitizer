#ifndef TWaveRecordData_hpp
#define TWaveRecordData_hpp 1

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

// time step is also needed???????
typedef struct {
  unsigned char ModNumber;
  unsigned char ChNumber;
  uint64_t TimeStamp;
  uint32_t RecordLength;
  uint16_t *WaveForm;
} WaveFormData_t;

#endif

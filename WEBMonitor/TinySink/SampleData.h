#ifndef SampleData_h
#define SampleData_h 1

constexpr int ONE_EVENT_SIZE = 8;

struct sampleData{
   unsigned char magic;
   unsigned char format_ver;
   unsigned char module_num;
   unsigned char reserved;
   unsigned int data;
};

#endif

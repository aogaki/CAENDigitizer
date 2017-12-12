#ifndef SampleData_h
#define SampleData_h 1

const int ONE_EVENT_SIZE = 8;

struct SampleData {
  unsigned char magic;
  unsigned char format_ver;
  unsigned char module_num;
  unsigned char reserved;
  int data;
};

#endif

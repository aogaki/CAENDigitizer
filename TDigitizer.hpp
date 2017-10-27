// Test (interface) class of CAEN Digitizer libraries
// One member function is assigned one library function

#ifndef TDigitizer_hpp
#define TDigitizer_hpp 1

#include <string>

#include <CAENDigitizer.h>

class TDigitizer
{
 public:
  TDigitizer();
  virtual ~TDigitizer();

  // Communication functions
  void Open(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd);
  void Close();

  uint32_t ReadRegister(uint32_t address);
  void WriteRegister(uint32_t address, uint32_t data);

  void Reset();

  void GetInfo();

  void ClearData();

  void DisEveAlignedRead();

  void SetMaxNEventsBLT(uint32_t nEve);
  uint32_t GetMaxNEventsBLT();

  void MallocReadoutBuffer();
  void FreeReadoutBuffer();

  void ReadData();
  void GetNEvents();  // Number of events in the readout data

 private:
  int fHandler;
  char *fpReadoutBuffer{nullptr};
  uint32_t fMaxBufferSize{0};
  uint32_t fBufferSize{0};
  uint32_t fNEvents{0};
  uint32_t fReadSize{0};

  void PrintError(CAEN_DGTZ_ErrorCode err, std::string funcName);
};

#endif

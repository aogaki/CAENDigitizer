#ifndef TWaveRecordPar_hpp
#define TWaveRecordPar_hpp 1

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

class TWaveRecordPar
{
 public:
  TWaveRecordPar()
  {
    fRecordLength = 256;
    fBLTEvents = 512;
    fVpp = 2.;
    fVth = -0.5;
    fPolarity = CAEN_DGTZ_TriggerOnFallingEdge;
    fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
    fPostTriggerSize = 80;
    fChMask = 0b11111111;
  };
  ~TWaveRecordPar(){};

  void SetRecordLength(uint32_t l) { fRecordLength = l; }
  uint32_t GetRecordLength() { return fRecordLength; }

  void SetBLTEvents(uint32_t val) { fBLTEvents = val; }
  uint32_t GetBLTEvents() { return fBLTEvents; }

  void SetVpp(double val) { fVpp = val; }
  double GetVpp() { return fVpp; }

  void SetVth(double val) { fVth = val; }
  double GetVth() { return fVth; }

  void SetDCOffset(double val) { fDCOffset = val; }
  double GetDCOffset() { return fDCOffset; }

  void SetPolarity(CAEN_DGTZ_TriggerPolarity_t val) { fPolarity = val; }
  CAEN_DGTZ_TriggerPolarity_t GetPolarity() { return fPolarity; }

  void SetTriggerMode(CAEN_DGTZ_TriggerMode_t val) { fTriggerMode = val; }
  CAEN_DGTZ_TriggerMode_t GetTriggerMode() { return fTriggerMode; }

  void SetPostTriggerSize(uint32_t val) { fPostTriggerSize = val; }
  uint32_t GetPostTriggerSize() { return fPostTriggerSize; }

  void SetChMask(uint16_t val) { fChMask = val; }
  uint16_t GetChMask() { return fChMask; }

 private:
  uint32_t fRecordLength;
  uint32_t fBLTEvents;
  double fVpp;
  double fVth;
  double fDCOffset;
  CAEN_DGTZ_TriggerPolarity_t fPolarity;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;
  uint32_t fPostTriggerSize;
  uint16_t fChMask;
};

#endif

#ifndef TDigitizer_hpp
#define TDigitizer_hpp 1

// For the standard fiemware digitizer
// This will be super class of other firmware and model family

#include <string>
#include <vector>

#include <TCanvas.h>
#include <TGraph.h>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

class TDigitizer
{
 public:
  TDigitizer();
  TDigitizer(CAEN_DGTZ_ConnectionType type, int link, int node = 0,
             uint32_t VMEadd = 0);
  virtual ~TDigitizer();

  void Initialize();

  void ReadEvents();

  void BoardCalibration();

  void StartAcquisition();
  void StopAcquisition();

  void GetBaseLine();

  const std::vector<uint32_t> *GetCharge() { return fCharge; };
  const std::vector<uint64_t> *GetTime() { return fTime; };

 protected:
  int fHandler{-1};  // What is the possible region of handler?

  // For board information
  int fDigitizerModel;
  uint32_t fNChs;
  int fTSample;  // This means time step length in ns
  int fNBits;    // ADC, Waveform resolution

  // For event readout
  char *fpReadoutBuffer;
  char *fpEventPtr;
  CAEN_DGTZ_EventInfo_t fEventInfo;
  CAEN_DGTZ_UINT16_EVENT_t *fpEventStd;  // for void **Eve
  uint32_t fMaxBufferSize;
  uint32_t fBufferSize;
  uint32_t fNEvents;
  uint32_t fReadSize;
  uint32_t fBLTEvents;
  uint32_t fRecordLength;
  uint32_t fBaseLine;

  // For trigger setting
  double fVpp;
  double fVth;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;
  CAEN_DGTZ_TriggerPolarity_t fPolarity;
  uint32_t fPostTriggerSize;

  // Charge and time
  std::vector<uint32_t> *fCharge;
  std::vector<uint64_t> *fTime;
  uint64_t fTimeOffset;
  uint64_t fPreviousTime;

  // Plotting wave form
  TCanvas *fCanvas;
  TGraph *fGraph;

  virtual void SetParameters();

  virtual void Open(CAEN_DGTZ_ConnectionType type, int link, int node,
                    uint32_t VMEadd);
  virtual void Close();

  virtual void Reset();

  virtual void GetBoardInfo();

  virtual void AcquisitionConfig();
  virtual void TriggerConfig();

  void PrintError(const CAEN_DGTZ_ErrorCode &err, const std::string &funcName);

  // Those SPI register functions are copy from digiTES
  CAEN_DGTZ_ErrorCode ReadSPIRegister(uint32_t ch, uint32_t address,
                                      uint32_t &data);

  CAEN_DGTZ_ErrorCode WriteSPIRegister(uint32_t ch, uint32_t address,
                                       uint32_t data);
};

#endif

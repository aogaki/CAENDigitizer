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
  TDigitizer(){};
  virtual ~TDigitizer(){};

  virtual void Initialize() = 0;

  virtual void ReadEvents() = 0;

  virtual void BoardCalibration() = 0;

  virtual void StartAcquisition() = 0;
  virtual void StopAcquisition() = 0;

  virtual const std::vector<int32_t> *GetCharge() = 0;
  virtual const std::vector<uint64_t> *GetTime() = 0;

 private:
};

#endif

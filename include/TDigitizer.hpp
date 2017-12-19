#ifndef TDigitizer_hpp
#define TDigitizer_hpp 1

// For the standard fiemware digitizer
// This will be super class of other firmware and model family

#include <string>
#include <vector>

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

enum class FirmWareCode {
  // I need only theese
  // Its are not same as CAENDigitizerType.h
  DPP_PSD,
  DPP_PHA,
  DPP_CI,
  STD,
};

class TDigitizer
{
 public:
  TDigitizer();
  virtual ~TDigitizer();

  void SendSWTrigger();
  void BoardCalibration();

  virtual void Initialize() = 0;

  virtual void ReadEvents() = 0;

  virtual void StartAcquisition() = 0;
  virtual void StopAcquisition() = 0;

 protected:
  int fHandler;
  void Open(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd);
  void Close();
  void Reset();

  // For board information
  void GetBoardInfo();
  int fDigitizerModel;
  FirmWareCode fFirmware;
  uint32_t fNChs;
  int fTSample;  // This means time step length in ns
  int fNBits;    // ADC, Waveform resolution

  void PrintError(const CAEN_DGTZ_ErrorCode &err, const std::string &funcName);

 private:
  // Those SPI register functions are copy from digiTES
  CAEN_DGTZ_ErrorCode ReadSPIRegister(uint32_t ch, uint32_t address,
                                      uint32_t &data);

  CAEN_DGTZ_ErrorCode WriteSPIRegister(uint32_t ch, uint32_t address,
                                       uint32_t data);
};

#endif

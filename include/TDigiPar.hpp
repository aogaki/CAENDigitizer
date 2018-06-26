#ifndef TDIGIPAR_HPP
#define TDIGIPAR_HPP 1

// Parameter class of CAEN digitizer.
// Generating parameters from BoardInfo as same as digiTES
// Reading some parameters from XML file or SQL server

#include <string>

// boost
#include <boost/property_tree/xml_parser.hpp>

// CAEN libraries
#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

// #define MAX_NCH 32  // max. number of channels (physical+virtual)
// #define MAX_GW 20   // max. number of generic write commads

// Constants
static constexpr uint kgMaxCh = 16;

enum class DPPType {  // copy from digiTES.h
  DPP_CI = 0x0000,
  DPP_PSD_720 = 0x0001,
  DPP_PSD_751 = 0x0002,
  DPP_PSD_730 = 0x0003,
  DPP_PHA_724 = 0x0004,
  DPP_PHA_730 = 0x0005,
  DPP_nPHA_724 = 0x0006,
  STD_730 = 0x1000,
};
// For supporting SL7
// gcc4.8 don't support DPPType::DPP_CI as above
// namespace DPPType
// {  // copy from digiTES.h
// constexpr uint DPP_CI = 0x0000;
// constexpr uint DPP_PSD_720 = 0x0001;
// constexpr uint DPP_PSD_751 = 0x0002;
// constexpr uint DPP_PSD_730 = 0x0003;
// constexpr uint DPP_PHA_724 = 0x0004;
// constexpr uint DPP_PHA_730 = 0x0005;
// constexpr uint DPP_nPHA_724 = 0x0006;
// constexpr uint STD_730 = 0x1000;
// };  // namespace DPPType

class TDigiPar
{
 public:
  TDigiPar();
  explicit TDigiPar(const std::string &id, const CAEN_DGTZ_BoardInfo_t &info);
  ~TDigiPar();

  void GenParameter();

  void SetDeviceInfo(const CAEN_DGTZ_BoardInfo_t &info) { fDeviceInfo = info; };
  void SetDeviceID(const std::string &id) { fDeviceID = id; };

  // AHHHHHHHHHH, I wanna write auto generator of Gettersssssssssssss.
  const uint &GetNCh() { return fNCh; };
  const uint &GetTimeSample() { return fTimeSample; };
  const uint &GetDigiModel() { return fDigiModel; };
  const DPPType &GetDPPType() { return fDPPType; };
  const uint &GetRecordLength() { return fRecordLength; };
  const CAEN_DGTZ_IOLevel_t &GetIOLevel() { return fIOLevel; };

  const std::array<bool, kgMaxCh> &GetChFlag() { return fChFlag; };
  const std::array<CAEN_DGTZ_PulsePolarity_t, kgMaxCh> &GetPolarity()
  {
    return fPolarity;
  };
  const std::array<uint, kgMaxCh> &GetDCOffset() { return fDCOffset; };
  const std::array<uint, kgMaxCh> &GetPreTrigger() { return fPreTrigger; };
  const std::array<uint, kgMaxCh> &GetDecimation() { return fDecimation; };
  const std::array<uint, kgMaxCh> &GetTrapPoleZero() { return fTrapPoleZero; };
  const std::array<uint, kgMaxCh> &GetTrapFlatTop() { return fTrapFlatTop; };
  const std::array<uint, kgMaxCh> &GetTrapRiseTime() { return fTrapRiseTime; };
  const std::array<uint, kgMaxCh> &GetPeakingTime() { return fPeakingTime; };
  const std::array<uint, kgMaxCh> &GetTTFDelay() { return fTTFDelay; };
  const std::array<uint, kgMaxCh> &GetTTFSmoothing() { return fTTFSmoothing; };
  const std::array<uint, kgMaxCh> &GetThreshold() { return fThreshold; };
  const std::array<uint, kgMaxCh> &GetTrgHoldOff() { return fTrgHoldOff; };
  const std::array<float, kgMaxCh> &GetEneCoarseGain()
  {
    return fEneCoarseGain;
  };
  const std::array<float, kgMaxCh> &GetEneFineGain() { return fEneFineGain; };
  const std::array<uint, kgMaxCh> &GetNSPeak() { return fNSPeak; };
  const std::array<uint, kgMaxCh> &GetTrapNSBaseline()
  {
    return fTrapNSBaseline;
  };
  const std::array<uint, kgMaxCh> &GetFakeEventFlag()
  {
    return fFakeEventFlag;
  };
  const std::array<uint, kgMaxCh> &GetTrapCFDFraction()
  {
    return fTrapCFDFraction;
  };
  const std::array<uint, kgMaxCh> &GetDynamicRange() { return fDynamicRange; };
  const std::array<uint, kgMaxCh> &GetTrapNorm() { return fTrapNorm; };
  const std::array<uint, kgMaxCh> &GetSHF() { return fSHF; };

 private:
  CAEN_DGTZ_BoardInfo_t fDeviceInfo;
  std::string fDeviceID;
  boost::property_tree::ptree fXML;

  void InitPar();
  void LoadDeviceInfo();
  void LoadXML();

  uint fNCh;
  uint fTimeSample;
  uint fNBit;
  uint fDigiModel;
  DPPType fDPPType;

  // Board parameters
  uint fRecordLength;
  void LoadRecordLength();

  CAEN_DGTZ_IOLevel_t fIOLevel;
  void LoadIOLevel();

  // Channel parameters
  template <typename T>
  void LoadChPar(std::string key, std::array<T, kgMaxCh> &array);

  std::array<bool, kgMaxCh> fChFlag;
  void LoadChFlag();

  std::array<CAEN_DGTZ_PulsePolarity_t, kgMaxCh> fPolarity;
  void LoadPolarity();

  std::array<uint, kgMaxCh> fDCOffset;
  void LoadDCOffset();

  std::array<uint, kgMaxCh> fPreTrigger;
  void LoadPreTrigger();

  std::array<uint, kgMaxCh> fDecimation;
  std::array<uint, kgMaxCh> fTperS;  // in digiTES, it is "tu".
  void LoadDecimation();

  std::array<uint, kgMaxCh> fTrapPoleZero;
  void LoadTrapPoleZero();

  std::array<uint, kgMaxCh> fTrapFlatTop;
  void LoadTrapFlatTop();

  std::array<uint, kgMaxCh> fTrapRiseTime;
  void LoadTrapRiseTime();

  std::array<uint, kgMaxCh> fPeakingTime;
  void LoadPeakingTime();

  std::array<uint, kgMaxCh> fTTFDelay;
  void LoadTTFDelay();

  std::array<uint, kgMaxCh> fTTFSmoothing;
  void LoadTTFSmoothing();

  std::array<uint, kgMaxCh> fThreshold;
  void LoadThreshold();

  std::array<uint, kgMaxCh> fTrgHoldOff;
  void LoadTrgHoldOff();

  std::array<float, kgMaxCh> fEneCoarseGain;
  void LoadEneCoarseGain();

  std::array<float, kgMaxCh> fEneFineGain;
  void LoadEneFineGain();

  std::array<uint, kgMaxCh> fNSPeak;
  void LoadNSPeak();

  std::array<uint, kgMaxCh> fTrapNSBaseline;
  void LoadTrapNSBaseline();

  std::array<uint, kgMaxCh> fFakeEventFlag;
  void LoadFakeEventFlag();

  std::array<uint, kgMaxCh> fTrapCFDFraction;
  void LoadTrapCFDFraction();

  std::array<uint, kgMaxCh> fDynamicRange;
  void LoadDynamicRange();

  std::array<uint, kgMaxCh> fTrapNorm;
  std::array<uint, kgMaxCh> fSHF;
  void CalParameters();
};

#endif

#include <iostream>

// boost
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "TDigiPar.hpp"

using std::cerr;
using std::cout;
using std::endl;

TDigiPar::TDigiPar() { InitPar(); }
TDigiPar::TDigiPar(const std::string &id, const CAEN_DGTZ_BoardInfo_t &info)
{
  InitPar();
  SetDeviceID(id);
  SetDeviceInfo(info);
  LoadDeviceInfo();
  LoadXML();
}

TDigiPar::~TDigiPar() {}

void TDigiPar::InitPar()
{
  fNCh = 0;
  fRecordLength = kNSamples;

  for (auto iCh = 0; iCh < kgMaxCh; iCh++) {
    fChFlag[iCh] = false;
    fPolarity[iCh] = CAEN_DGTZ_PulsePolarityNegative;
    fDCOffset[iCh] = 0;
    fPreTrigger[iCh] = 128;
    fDecimation[iCh] = 0;
    fTrapPoleZero[iCh] = 50000;
    fTrapFlatTop[iCh] = 1000;
    fTrapRiseTime[iCh] = 5000;
    fPeakingTime[iCh] = 800;
    fTTFDelay[iCh] = 200;
    fTTFSmoothing[iCh] = 1;
    fThreshold[iCh] = 300;
    fTrgHoldOff[iCh] = 1;
    fEneCoarseGain[iCh] = 1.;
    fEneFineGain[iCh] = 1.;
    fNSPeak[iCh] = 0;
    fTrapNSBaseline[iCh] = 5;
    fFakeEventFlag[iCh] = 1;
    fTrapCFDFraction[iCh] = 1;
    fDynamicRange[iCh] = 0;
    fTperS[iCh] = 1;
    fTrapNorm[iCh] = 1;
    fSHF[iCh] = 8;
  }
}

void TDigiPar::LoadDeviceInfo()
{
  fNCh = fDeviceInfo.Channels;

  if (fDeviceInfo.FamilyCode == 5) {
    fDigiModel = 751;
    fTimeSample = 1;
    fNBit = 10;
  } else if (fDeviceInfo.FamilyCode == 7) {
    fDigiModel = 780;
    fTimeSample = 10;
    fNBit = 14;
  } else if (fDeviceInfo.FamilyCode == 13) {
    fDigiModel = 781;
    fTimeSample = 10;
    fNBit = 14;
  } else if (fDeviceInfo.FamilyCode == 0) {
    fDigiModel = 724;
    fTimeSample = 10;
    fNBit = 14;
  } else if (fDeviceInfo.FamilyCode == 11) {
    fDigiModel = 730;
    fTimeSample = 2;
    fNBit = 14;
  } else if (fDeviceInfo.FamilyCode == 14) {
    fDigiModel = 725;
    fTimeSample = 4;
    fNBit = 14;
  } else if (fDeviceInfo.FamilyCode == 3) {
    fDigiModel = 720;
    fTimeSample = 4;
    fNBit = 12;
  } else if (fDeviceInfo.FamilyCode == 999) {  // temporary code for Hexagon
    fDigiModel = 5000;
    fTimeSample = 10;
    fNBit = 14;
  } else {
    cerr << "ERROR: Unknown digitizer model" << endl;
    exit(0);
  }

  // From digiTES
  auto FWCode = std::string(fDeviceInfo.AMC_FirmwareRel);
  int major = stoi(FWCode.substr(0, FWCode.find(".")));
  int minor = stoi(FWCode.substr(FWCode.find(".") + 1,
                                 FWCode.size() - (FWCode.find(".") + 1)));
  if (fDigiModel == 5000) {  // Hexagon
    fDPPType = DPPType::DPP_nPHA_724;
  } else if (major == 128) {
    if (minor >= 0x40) {
      fDPPType = DPPType::DPP_nPHA_724;
    } else {
      fDPPType = DPPType::DPP_PHA_724;
    }
  } else if (major == 130) {
    fDPPType = DPPType::DPP_CI;
  } else if (major == 131) {
    fDPPType = DPPType::DPP_PSD_720;
  } else if (major == 132) {
    fDPPType = DPPType::DPP_PSD_751;
  } else if (major == 136) {
    fDPPType = DPPType::DPP_PSD_730;  // NOTE: valid also for x725
  } else if (major == 139) {
    fDPPType = DPPType::DPP_PHA_730;  // NOTE: valid also for x725
  } else {
    fDPPType = DPPType::STD_730;
  }
}

void TDigiPar::LoadXML()
{
  // Need file checker
  std::string fileName = "/Data/DAQ/Parameters/parameters.xml";
  boost::property_tree::ptree pt;
  boost::property_tree::xml_parser::read_xml(fileName, pt);
  for (auto &childTree : pt.get_child("root")) {
    auto id = childTree.second.get_optional<std::string>("<xmlattr>.ID");
    if (id == fDeviceID) {
      fXML = childTree.second;
      break;  // Logically, I can break this loop.
    }
  }

  // How to check fXML?  This is most important in this class.
  // Temporary to use the size of it.
  if (fXML.size() == 0) {
    std::cerr << "The XML file is not loaded.  Check it." << std::endl;
    exit(0);
  }
}

void TDigiPar::GenParameter()
{
  // Loading information from XML file and CAEN_DGTZ_fDeviceInfo_t
  LoadChFlag();
  LoadRecordLength();
  LoadIOLevel();
  LoadPolarity();
  LoadDCOffset();  // This uses polarity information
  LoadPreTrigger();
  LoadDecimation();
  // Many parameters bellow use Decimation
  LoadTrapPoleZero();
  LoadTrapFlatTop();
  LoadTrapRiseTime();
  LoadPeakingTime();
  LoadTTFDelay();
  LoadTTFSmoothing();
  LoadThreshold();
  LoadTrgHoldOff();
  LoadEneCoarseGain();
  LoadEneFineGain();
  LoadNSPeak();
  LoadTrapNSBaseline();
  LoadFakeEventFlag();
  LoadTrapCFDFraction();
  CalParameters();
}

void TDigiPar::LoadRecordLength()
{
  auto length = fXML.get_optional<uint>("RecordLength");
  std::cout << *length << std::endl;
  if (length)
    fRecordLength = *length;
  else {
    std::cerr << "The record length is not loaded.  Check it." << std::endl;
    exit(0);
  }
}

void TDigiPar::LoadIOLevel()
{
  auto ioLevel = fXML.get_optional<std::string>("IOLevel");
  if (*ioLevel == "NIM") {
    fIOLevel = CAEN_DGTZ_IOLevel_NIM;
  } else if (*ioLevel == "TTL") {
    fIOLevel = CAEN_DGTZ_IOLevel_TTL;
  } else {
    std::cerr << "The IO level is " << *ioLevel << std::endl;
    std::cerr << "The IO level is not loaded.  Check it." << std::endl;
    exit(0);
  }
}

template <typename T>
void TDigiPar::LoadChPar(std::string key, std::array<T, kgMaxCh> &array)
{
  auto counter = 0;
  for (auto &child : fXML.get_child("")) {
    if (child.first != "channel") continue;

    auto id = child.second.get_optional<uint>("<xmlattr>.ID");
    if (!id || *id >= kgMaxCh || *id < 0) continue;

    if (auto val = child.second.get_optional<T>(key)) {
      counter++;
      array[*id] = *val;
      // std::cout << "key: " << key << "\tval: " << *val << std::endl;
    }
  }

  if (counter == 0) {
    cerr << "No parameters are loaded.  Key: " << key << endl;
  }
}

void TDigiPar::LoadChFlag() { LoadChPar("Using", fChFlag); }

void TDigiPar::LoadPreTrigger()
{
  LoadChPar("PreTrigger", fPreTrigger);
  for (auto &val : fPreTrigger) {
    val /= fTimeSample;  // ns -> samples
    val /= 4;            // samples -> register val
  }
}

void TDigiPar::LoadPolarity()
{
  std::array<std::string, kgMaxCh> tmp{""};
  LoadChPar("Polarity", tmp);
  for (auto iCh = 0; iCh < fNCh; iCh++) {
    CAEN_DGTZ_PulsePolarity_t pol;
    if (tmp[iCh] == "Positive")
      pol = CAEN_DGTZ_PulsePolarityPositive;
    else if (tmp[iCh] == "Negative")
      pol = CAEN_DGTZ_PulsePolarityNegative;
    else {
      std::cerr << "The polarity is " << tmp[iCh] << std::endl;
      std::cerr << "The polarity is not loaded.  Check it." << std::endl;
      exit(0);
    }

    fPolarity[iCh] = pol;
  }
}

void TDigiPar::LoadDCOffset()
{
  // This uses polarity information
  // LoadPolarity();
  // LoadChPar("DCOffset", fDCOffset);
  // for (auto iCh = 0; iCh < fNCh; iCh++) {
  //   auto offset = fDCOffset[iCh];
  //   if (fPolarity[iCh] == CAEN_DGTZ_PulsePolarityPositive)
  //     offset = 100 - offset;
  //
  //   fDCOffset[iCh] = offset * ((1 << 16) - 1) / 100;
  //   cout << fPolarity[iCh] << "\t" << offset << "\t" << fDCOffset[iCh] <<
  //   endl;
  // }
  LoadChPar("DCOffset", fDCOffset);
  for (auto iCh = 0; iCh < fNCh; iCh++)
    fDCOffset[iCh] = fDCOffset[iCh] * 0xFFFF / 100;
}

void TDigiPar::LoadDecimation()
{
  LoadChPar("Decimation", fDecimation);
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) {
    fTperS[iCh] = fTimeSample * 4 * (1 << fDecimation[iCh]);
  }
}

void TDigiPar::LoadTrapPoleZero()
{
  LoadChPar("TrapPoleZero", fTrapPoleZero);
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) fTrapPoleZero[iCh] /= fTperS[iCh];
}

void TDigiPar::LoadTrapFlatTop()
{
  LoadChPar("TrapFlatTop", fTrapFlatTop);
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) fTrapFlatTop[iCh] /= fTperS[iCh];
}

void TDigiPar::LoadTrapRiseTime()
{
  LoadChPar("TrapRiseTime", fTrapRiseTime);
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) fTrapRiseTime[iCh] /= fTperS[iCh];
}

void TDigiPar::LoadPeakingTime()
{
  LoadChPar("PeakingTime", fPeakingTime);
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) {
    fPeakingTime[iCh] /= fTperS[iCh];
    if (fPeakingTime[iCh] > fTrapFlatTop[iCh])
      fPeakingTime[iCh] = fTrapFlatTop[iCh];
  }
}

void TDigiPar::LoadTTFDelay()
{
  LoadChPar("TTFDelay", fTTFDelay);
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) fTTFDelay[iCh] /= fTimeSample * 4;
}

void TDigiPar::LoadTTFSmoothing() { LoadChPar("TTFSmoothing", fTTFSmoothing); }

void TDigiPar::LoadThreshold() { LoadChPar("Threshold", fThreshold); }

void TDigiPar::LoadTrgHoldOff()
{
  // Stupid coding!!!!  8 means stu in digiTES.
  // What is this?
  LoadChPar("TrgHoldOff", fTrgHoldOff);
  auto stu = 8;  // For 730.  16 for 725
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) fTrgHoldOff[iCh] /= stu;
}

void TDigiPar::LoadEneCoarseGain()
{
  LoadChPar("EneCoarseGain", fEneCoarseGain);
}

void TDigiPar::LoadEneFineGain() { LoadChPar("EneFineGain", fEneFineGain); }

void TDigiPar::LoadNSPeak() { LoadChPar("NSPeak", fNSPeak); }

void TDigiPar::LoadTrapNSBaseline()
{
  LoadChPar("TrapNSBaseline", fTrapNSBaseline);
}

void TDigiPar::LoadFakeEventFlag()
{
  LoadChPar("FakeEventFlag", fFakeEventFlag);
}

void TDigiPar::LoadTrapCFDFraction()
{
  LoadChPar("TrapCFDFraction", fTrapCFDFraction);
}

void TDigiPar::LoadDynamicRange() { LoadChPar("DynamicRange", fDynamicRange); }

void TDigiPar::CalParameters()
{
  for (auto iCh = 0; iCh < kgMaxCh; iCh++) {
    auto Tg = (fTrapPoleZero[iCh] * fTrapRiseTime[iCh]) /
              (fEneCoarseGain[iCh] * fEneFineGain[iCh]);

    for (uint i = 0; i < 31; i++) {
      if ((1 << i) > Tg) {
        fSHF[iCh] = i;
        break;
      }
    }

    fTrapNorm[iCh] = (65535.0 * (1 << fSHF[iCh]) / Tg);
  }
}

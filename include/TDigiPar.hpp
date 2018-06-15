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

class TDigiPar
{
 public:
  TDigiPar();
  explicit TDigiPar(const std::string &id, const CAEN_DGTZ_BoardInfo_t &info);
  ~TDigiPar();

  void SetDeviceInfo(const CAEN_DGTZ_BoardInfo_t &info) { fDeviceInfo = info; };
  void SetDeviceID(const std::string &id) { fDeviceID = id; };

  void test();

 private:
  CAEN_DGTZ_BoardInfo_t fDeviceInfo;
  std::string fDeviceID;
  boost::property_tree::ptree fXML;

  void InitPar();
  void LoadXML();
};

#endif

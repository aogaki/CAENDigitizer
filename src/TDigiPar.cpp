#include <iostream>

// boost
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "TDigiPar.hpp"

TDigiPar::TDigiPar() {}
TDigiPar::TDigiPar(const std::string &id, const CAEN_DGTZ_BoardInfo_t &info)
{
  SetDeviceID(id);
  SetDeviceInfo(info);
  InitPar();
  LoadXML();
}

TDigiPar::~TDigiPar() {}

void TDigiPar::InitPar() {}

void TDigiPar::LoadXML()
{
  // Need file checker
  std::string fileName = "/Data/DAQ/Parameters/parameters.xml";
  boost::property_tree::ptree pt;
  boost::property_tree::xml_parser::read_xml(fileName, pt);
  for (auto &device : pt.get_child("root")) {
    auto id = device.second.get_optional<std::string>("<xmlattr>.ID");
    if (id == fDeviceID) {
      fXML = device.second;
      break;  // Logically, I can break this loop.
    }
  }
}

void TDigiPar::test()
{
  std::cout << fXML.size() << std::endl;

  for (auto &module : fXML.get_child("")) {
    auto modName = std::string(module.first.data());
    std::cout << modName << std::endl;
    // auto id =
    //     fXML.get_optional<std::string>("root." + modName + ".<xmlattr>.ID");
    // if (id == fDeviceID) {
    //   for (auto &parameters : fXML.get_child("root.module0.channel0")) {
    //     auto parName = std::string(parameters.first.data());
    //     if (parName == "enf") {
    //       auto parVal = boost::lexical_cast<float>(parameters.second.data());
    //       std::cout << parName << "\t" << parVal << std::endl;
    //     } else {
    //       auto parVal = boost::lexical_cast<int>(parameters.second.data());
    //       std::cout << parName << "\t" << parVal << std::endl;
    //     }
    //   }
    // }
  }
}

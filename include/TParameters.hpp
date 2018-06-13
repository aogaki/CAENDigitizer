// This is the superclass of parameter handler.
// Subclasses are for PHA and PSD firmware

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>
#include <boost/property_tree/xml_parser.hpp>

class TParameters
{
 public:
  TParameters();

 private:
  // To provide the parameters, the class should know information of the module
  CAEN_DGTZ_BoardInfo_t fBoardInfo;
  //  To read XML parameter file
  boost::property_tree::ptree fPropertyTree;
  // CAEN_DGTZ_DPP_PSD_Params_t PSDParams;
  // CAEN_DGTZ_DPP_PHA_Params_t PHAParams;

  //  For Acquisition config ===================================================
  uint32_t fChMask;

  // Acquisition mode
  CAEN_DGTZ_AcqMode_t fAcqMode;

  // Record length (length of waveform?) in the number of samples;
  uint32_t fRecordLength;

  // Mix means waveform list and energy
  CAEN_DGTZ_DPP_AcqMode_t fDPPAcqMode;
  CAEN_DGTZ_DPP_SaveParam_t fSaveParam;

  //
};

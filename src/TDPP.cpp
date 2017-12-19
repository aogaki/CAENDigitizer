#include <iostream>

#include "TDPP.hpp"

TDPP::TDPP() : TDigitizer() { SetParameters(); }

TDPP::TDPP(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd)
    : TDPP()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();
}

TDPP::~TDPP()
{
  Reset();
  Close();
}

void TDPP::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  // err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
  //                                     &fMaxBufferSize);
  // PrintError(err, "MallocReadoutBuffer");

  // 0 means automatically set
  err = CAEN_DGTZ_SetDPPEventAggregation(fHandler, 0, 0);
  PrintError(err, "SetDPPEventAggregation");

  uint32_t mask = 0xFF;
  if (fFirmware == FirmWareCode::DPP_PHA)
    err = CAEN_DGTZ_SetDPPParameters(fHandler, mask, &fParPHA);
  else if (fFirmware == FirmWareCode::DPP_PSD)
    err = CAEN_DGTZ_SetDPPParameters(fHandler, mask, &fParPSD);
  PrintError(err, "SetDPPParameters");

  BoardCalibration();
}

void TDPP::SetParameters()
{
  fRecordLength = 4096;
  fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  fPostTriggerSize = 80;
}

void TDPP::ReadEvents() {}

void TDPP::AcquisitionConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Eanble all channels
  uint32_t mask = ((1 << fNChs) - 1);
  err = CAEN_DGTZ_SetChannelEnableMask(fHandler, mask);
  PrintError(err, "SetChannelEnableMask");

  // Set the acquisition mode
  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  // Set record length (length of waveform?);
  err = CAEN_DGTZ_SetRecordLength(fHandler, fRecordLength);
  PrintError(err, "SetRecordLength");

  // Mix means waveform list and energy
  err = CAEN_DGTZ_SetDPPAcquisitionMode(
      fHandler, CAEN_DGTZ_DPP_AcqMode_t::CAEN_DGTZ_DPP_ACQ_MODE_Mixed,
      CAEN_DGTZ_DPP_SaveParam_t::CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
  PrintError(err, "SetDPPAcquisitionMode");
}

void TDPP::TriggerConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Set the trigger threshold
  // The unit of its are V
  int32_t th = ((1 << fNBits) / 2) * ((fVth / (fVpp / 2)));
  uint32_t thVal = (1 << fNBits) / 2;
  if (thVal == 0) thVal = ((1 << fNBits) / 2);
  thVal += th;
  std::cout << "Vth:\t" << thVal << std::endl;

  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPHA.thr[iCh] = thVal;
    fParPSD.thr[iCh] = thVal;
  }

  // Set the triggermode
  uint32_t mask = ((1 << fNChs) - 1);
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, fTriggerMode, mask);
  PrintError(err, "SetChannelSelfTrigger");

  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, fTriggerMode);
  PrintError(err, "SetSWTriggerMode");

  uint32_t samples = fRecordLength * 0.2;
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, iCh, samples);
    PrintError(err, "SetDPPPreTriggerSize");
  }

  CAEN_DGTZ_PulsePolarity_t pol = CAEN_DGTZ_PulsePolarityNegative;
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, iCh, pol);
    PrintError(err, "CAEN_DGTZ_SetChannelPulsePolarity");
  }

  // // Set post trigger size
  // err = CAEN_DGTZ_SetPostTriggerSize(fHandler, fPostTriggerSize);
  // PrintError(err, "SetPostTriggerSize");

  // // Set the triiger polarity
  // for (uint32_t iCh = 0; iCh < fNChs; iCh++)
  //   CAEN_DGTZ_SetTriggerPolarity(fHandler, iCh, fPolarity);
  //
  // CAEN_DGTZ_TriggerPolarity_t pol;
  // CAEN_DGTZ_GetTriggerPolarity(fHandler, 0, &pol);
  // std::cout << "Polarity:\t" << pol << std::endl;
}

void TDPP::StartAcquisition()
{
  // CAEN_DGTZ_ErrorCode err;
  // err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  // PrintError(err, "StartAcquisition");
  //
  // err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  // PrintError(err, "AllocateEvent");
  //
  // fTimeOffset = 0;
  // fPreviousTime = 0;
}

void TDPP::StopAcquisition()
{
  // CAEN_DGTZ_ErrorCode err;
  // err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  // PrintError(err, "StopAcquisition");
  //
  // err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  // PrintError(err, "FreeEvent");
}

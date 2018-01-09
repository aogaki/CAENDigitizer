#include <math.h>
#include <string.h>
#include <iostream>

#include "TDPP.hpp"
#include "TStdData.hpp"

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

TDPP::TDPP() : TDigitizer() { SetParameters(); }

TDPP::TDPP(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd)
    : TDPP()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();

  fDataArray = new unsigned char[fBLTEvents * ONE_HIT_SIZE * fNChs];
}

TDPP::~TDPP()
{
  Reset();
  FreeMemory();
  Close();
}

void TDPP::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  // Buffer setting
  err = CAEN_DGTZ_SetNumEventsPerAggregate(fHandler, fBLTEvents);
  PrintError(err, "SetNumEventsPerAggregate");
  // 0 means automatically set
  err = CAEN_DGTZ_SetDPPEventAggregation(fHandler, 0, 0);
  PrintError(err, "SetDPPEventAggregation");

  // Synchronization Mode
  err = CAEN_DGTZ_SetRunSynchronizationMode(fHandler,
                                            CAEN_DGTZ_RUN_SYNC_Disabled);
  PrintError(err, "SetRunSynchronizationMode");

  // Following for loop is copied from sample.
  for (int i = 0; i < fNChs; i++) {
    // Set the number of samples for each waveform (you can set different RL
    // for different channels)
    err = CAEN_DGTZ_SetRecordLength(fHandler, kNSamples, i);

    // Set a DC offset to the input signal to adapt it to digitizer's dynamic
    // range
    err = CAEN_DGTZ_SetChannelDCOffset(fHandler, i, (1 << fNBits) / 2);

    // Set the Pre-Trigger size (in samples)
    err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, i, 80);

    // Set the polarity for the given channel (CAEN_DGTZ_PulsePolarityPositive
    // or CAEN_DGTZ_PulsePolarityNegative)
    err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, i,
                                            CAEN_DGTZ_PulsePolarityNegative);
  }

  AllocateMemory();

  BoardCalibration();
}

void TDPP::ReadEvents() {}

void TDPP::SetParameters()
{
  fBLTEvents = 1023;  // It is max, why not 1024?
}

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
  err = CAEN_DGTZ_SetRecordLength(fHandler, kNSamples);
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

  // Set the triggermode
  auto triggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
  uint32_t mask = ((1 << fNChs) - 1);
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, triggerMode, mask);
  PrintError(err, "SetChannelSelfTrigger");

  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, triggerMode);
  PrintError(err, "SetSWTriggerMode");

  uint32_t samples = kNSamples * 0.2;
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, iCh, samples);
    PrintError(err, "SetDPPPreTriggerSize");
  }

  CAEN_DGTZ_PulsePolarity_t pol = CAEN_DGTZ_PulsePolarityNegative;
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, iCh, pol);
    PrintError(err, "CAEN_DGTZ_SetChannelPulsePolarity");
  }

  if (fFirmware == FirmWareCode::DPP_PSD) {
    err = CAEN_DGTZ_SetDPPTriggerMode(
        fHandler,
        CAEN_DGTZ_DPP_TriggerMode_t::CAEN_DGTZ_DPP_TriggerMode_Normal);
    PrintError(err, "SetDPPTriggerMode");
  }
}

void TDPP::AllocateMemory()
{
  CAEN_DGTZ_ErrorCode err;
  uint32_t size;

  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer, &size);
  PrintError(err, "MallocReadoutBuffer");
}

void TDPP::FreeMemory()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
  PrintError(err, "FreeReadoutBuffer");
  // DelPointer(fpReadoutBuffer);
}

CAEN_DGTZ_ErrorCode TDPP::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  return err;
}

void TDPP::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  // err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  // PrintError(err, "FreeEvent");
}

void TDPP::SetMaster()
{  // Synchronization Mode
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SetRunSynchronizationMode(
      fHandler, CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain);
  PrintError(err, "SetRunSynchronizationMode");

  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  uint32_t mask = ((1 << fNChs) - 1);
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler,
                                        CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, mask);
  PrintError(err, "SetChannelSelfTrigger");
}

void TDPP::SetSlave()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SetRunSynchronizationMode(
      fHandler, CAEN_DGTZ_RUN_SYNC_TrgOutTrgInDaisyChain);
  PrintError(err, "SetRunSynchronizationMode");

  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_FIRST_TRG_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  uint32_t mask = ((1 << fNChs) - 1);
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler,
                                        CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, mask);
  PrintError(err, "SetChannelSelfTrigger");

  err = CAEN_DGTZ_SetExtTriggerInputMode(fHandler,
                                         CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  PrintError(err, "SetExtTriggerInputMode");
}

void TDPP::StartSyncMode(uint32_t nMods)
{
  // copy from digiTes
  // CAEN_DGTZ_ErrorCode err;
  int err{0};
  uint32_t d32;
  constexpr uint32_t RUN_START_ON_TRGIN_RISING_EDGE = 0xE;
  err |= CAEN_DGTZ_ReadRegister(fHandler, CAEN_DGTZ_ACQ_CONTROL_ADD, &d32);
  err |= CAEN_DGTZ_WriteRegister(
      fHandler, CAEN_DGTZ_ACQ_CONTROL_ADD,
      (d32 & 0xFFFFFFF0) | RUN_START_ON_TRGIN_RISING_EDGE);  // Arm acquisition
                                                             // (Run will start
                                                             // with 1st
                                                             // trigger)
  // Run Delay to deskew the start of acquisition
  if (fModNumber == 0)
    err |= CAEN_DGTZ_WriteRegister(fHandler, 0x8170, (nMods - 1) * 3 + 1);
  else
    err |=
        CAEN_DGTZ_WriteRegister(fHandler, 0x8170, (nMods - fModNumber - 1) * 3);

  // StartMode 1: use the TRGIN-TRGOUT daisy chain; the 1st trigger starts the
  // acquisition
  uint32_t mask = (fModNumber == 0) ? 0x80000000 : 0x40000000;
  err |=
      CAEN_DGTZ_WriteRegister(fHandler, CAEN_DGTZ_TRIGGER_SRC_ENABLE_ADD, mask);
  err |= CAEN_DGTZ_WriteRegister(fHandler, CAEN_DGTZ_FP_TRIGGER_OUT_ENABLE_ADD,
                                 mask);

  PrintError(CAEN_DGTZ_ErrorCode(err), "StartSyncMode");
}

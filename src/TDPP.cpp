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

TDPP::TDPP()
    : TDigitizer(),
      fpReadoutBuffer(nullptr),
      fppPSDEvents(nullptr),
      fpPSDWaveform(nullptr)
{
  SetParameters();
}

TDPP::TDPP(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd)
    : TDPP()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();

  fTime.resize(fNChs);
  fTimeOffset.resize(fNChs);
  fPreviousTime.resize(fNChs);

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

  // err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
  //                                     &fMaxBufferSize);
  // PrintError(err, "MallocReadoutBuffer");

  // Buffer setting
  err = CAEN_DGTZ_SetNumEventsPerAggregate(fHandler, fBLTEvents);
  PrintError(err, "SetNumEventsPerAggregate");
  // 0 means automatically set
  err = CAEN_DGTZ_SetDPPEventAggregation(fHandler, 0, 0);
  PrintError(err, "SetDPPEventAggregation");

  SetPSDPar();
  uint32_t mask = 0xFF;
  if (fFirmware == FirmWareCode::DPP_PHA)
    err = CAEN_DGTZ_SetDPPParameters(fHandler, mask, &fParPHA);
  else if (fFirmware == FirmWareCode::DPP_PSD)
    err = CAEN_DGTZ_SetDPPParameters(fHandler, mask, &fParPSD);
  PrintError(err, "SetDPPParameters");

  // Following for loop is copied from sample.  Shame on me!!!!!!!
  for (int i = 0; i < fNChs; i++) {
    // Set the number of samples for each waveform (you can set different RL
    // for different channels)
    err = CAEN_DGTZ_SetRecordLength(fHandler, fRecordLength, i);

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

void TDPP::ReadEvents()
{
  fNEvents = 0;  // Event counter

  CAEN_DGTZ_ErrorCode err;

  uint32_t bufferSize;
  err = CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                           fpReadoutBuffer, &bufferSize);
  PrintError(err, "ReadData");
  if (bufferSize == 0) return;  // in the case of 0, GetDPPEvents makes crush

  uint32_t nEvents[fNChs];
  err = CAEN_DGTZ_GetDPPEvents(fHandler, fpReadoutBuffer, bufferSize,
                               (void **)fppPSDEvents, nEvents);
  PrintError(err, "GetDPPEvents");

  // for (int i = 0; i < nEvents[0]; i++)
  //   std::cout << (fppPSDEvents[0][i]).ChargeLong << std::endl;

  for (int iCh = 0; iCh < fNChs; iCh++) {
    for (int iEve = 0; iEve < nEvents[iCh]; iEve++) {
      err = CAEN_DGTZ_DecodeDPPWaveforms(fHandler, &fppPSDEvents[iCh][iEve],
                                         fpPSDWaveform);
      PrintError(err, "DecodeDPPWaveforms");

      fTime[iCh] =
          (fppPSDEvents[iCh][iEve].TimeTag + fTimeOffset[iCh]) * fTSample;
      if (fTime[iCh] < fPreviousTime[iCh]) {
        constexpr uint32_t maxTime = 0xFFFFFFFF / 2;  // Check manual
        fTime[iCh] += maxTime * fTSample;
        fTimeOffset[iCh] += maxTime;
      }
      fPreviousTime[iCh] = fTime[iCh];

      auto index = fNEvents * ONE_HIT_SIZE;
      fDataArray[index++] = 0;    // fModNumber is needed.
      fDataArray[index++] = iCh;  // int to char.  Dangerous

      constexpr auto timeSize = sizeof(fTime[0]);
      memcpy(&fDataArray[index], &fTime[iCh], timeSize);
      index += timeSize;

      constexpr auto adcSize = sizeof(fppPSDEvents[0][0].ChargeLong);
      // auto adc = sumCharge;
      memcpy(&fDataArray[index], &fppPSDEvents[iCh][iEve].ChargeLong, adcSize);
      index += adcSize;

      std::cout << fppPSDEvents[iCh][iEve].ChargeLong << std::endl;

      constexpr auto waveSize = sizeof(fpPSDWaveform->Trace1[0]) * kNSamples;
      memcpy(&fDataArray[index], fpPSDWaveform->Trace1, waveSize);

      fNEvents++;
    }
  }
}

void TDPP::SetParameters()
{
  fRecordLength = kNSamples;
  fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  fPostTriggerSize = 80;
  fBLTEvents = 1023;  // It is max, why not 1024?

  void SetPSDPar();
}

void TDPP::SetPHAPar() {}

void TDPP::SetPSDPar()
{  // Copy from sample
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPSD.thr[iCh] = 100;  // Trigger Threshold
    /* The following parameter is used to specifiy the number of samples for the
    baseline averaging: 0 -> absolute Bl 1 -> 4samp 2 -> 8samp 3 -> 16samp 4 ->
    32samp 5 -> 64samp 6 -> 128samp */
    fParPSD.nsbl[iCh] = 2;
    fParPSD.lgate[iCh] = 128;  // Long Gate Width (N*4ns)
    fParPSD.sgate[iCh] = 0;    // Short Gate Width (N*4ns)
    fParPSD.pgate[iCh] = 16;   // Pre Gate Width (N*4ns)
    /* Self Trigger Mode:
    0 -> Disabled
    1 -> Enabled */
    fParPSD.selft[iCh] = 1;
    // Trigger configuration:
    // CAEN_DGTZ_DPP_TriggerConfig_Peak       -> trigger on peak. NOTE: Only for
    // FW <= 13X.5 CAEN_DGTZ_DPP_TriggerConfig_Threshold  -> trigger on
    // threshold */
    // fParPSD.trgc[iCh] = CAEN_DGTZ_DPP_TriggerConfig_Threshold;
    fParPSD.trgc[iCh] = CAEN_DGTZ_DPP_TriggerConfig_Peak;
    /* Trigger Validation Acquisition Window */
    fParPSD.tvaw[iCh] = 50;
    /* Charge sensibility: 0->40fc/LSB; 1->160fc/LSB; 2->640fc/LSB; 3->2,5pc/LSB
     */
    fParPSD.csens[iCh] = 0;
  }
  /* Pile-Up rejection Mode
  CAEN_DGTZ_DPP_PSD_PUR_DetectOnly -> Only Detect Pile-Up
  CAEN_DGTZ_DPP_PSD_PUR_Enabled -> Reject Pile-Up */
  fParPSD.purh = CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;
  fParPSD.purgap = 100;  // Purity Gap
  fParPSD.blthr = 3;     // Baseline Threshold
  fParPSD.bltmo = 100;   // Baseline Timeout
  fParPSD.trgho = 8;     // Trigger HoldOff
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
  int32_t th = fabs((1 << fNBits) * (fVth / fVpp));
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPHA.thr[iCh] = th;
    fParPSD.thr[iCh] = th;
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

  if (fFirmware == FirmWareCode::DPP_PSD) {
    err = CAEN_DGTZ_SetDPPTriggerMode(
        fHandler,
        CAEN_DGTZ_DPP_TriggerMode_t::CAEN_DGTZ_DPP_TriggerMode_Normal);
    PrintError(err, "SetDPPTriggerMode");
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

void TDPP::AllocateMemory()
{
  CAEN_DGTZ_ErrorCode err;
  uint32_t size;

  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer, &size);
  PrintError(err, "MallocReadoutBuffer");

  // CAEN_DGTZ_DPP_PSD_Event_t *events[fNChs];
  fppPSDEvents = new CAEN_DGTZ_DPP_PSD_Event_t *[fNChs];
  err = CAEN_DGTZ_MallocDPPEvents(fHandler, (void **)fppPSDEvents, &size);
  PrintError(err, "MallocDPPEvents");

  err = CAEN_DGTZ_MallocDPPWaveforms(fHandler, (void **)&fpPSDWaveform, &size);
  PrintError(err, "MallocDPPWaveforms");
}

void TDPP::FreeMemory()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
  PrintError(err, "FreeReadoutBuffer");
  // DelPointer(fpReadoutBuffer);

  err = CAEN_DGTZ_FreeDPPEvents(fHandler, (void **)fppPSDEvents);
  PrintError(err, "FreeDPPEvents");
  // DelPointer(fppPSDEvents);

  err = CAEN_DGTZ_FreeDPPWaveforms(fHandler, fpPSDWaveform);
  PrintError(err, "FreeDPPWaveforms");
  // DelPointer(fpPSDWaveform);
}

CAEN_DGTZ_ErrorCode TDPP::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  for (auto &t : fTime) t = 0;
  for (auto &t : fTimeOffset) t = 0;
  for (auto &t : fPreviousTime) t = 0;

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

#include <math.h>
#include <string.h>
#include <iostream>

#include "TPHA.hpp"
#include "TStdData.hpp"

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

TPHA::TPHA()
    : TDigitizer(),
      fpReadoutBuffer(nullptr),
      fppPHAEvents(nullptr),
      fpPHAWaveform(nullptr)
{
  SetParameters();
}

TPHA::TPHA(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd)
    : TPHA()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();

  fTime.resize(fNChs);
  fTimeOffset.resize(fNChs);
  fPreviousTime.resize(fNChs);

  fDataArray = new unsigned char[fBLTEvents * ONE_HIT_SIZE * fNChs];
}

TPHA::~TPHA()
{
  Reset();
  FreeMemory();
  Close();
}

void TPHA::Initialize()
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

  // Synchronization Mode
  err = CAEN_DGTZ_SetRunSynchronizationMode(fHandler,
                                            CAEN_DGTZ_RUN_SYNC_Disabled);
  PrintError(err, "SetRunSynchronizationMode");

  SetPHAPar();
  if (fFirmware == FirmWareCode::DPP_PHA) {
    uint32_t mask = 0xFF;
    err = CAEN_DGTZ_SetDPPParameters(fHandler, mask, &fParPHA);
    PrintError(err, "SetDPPParameters");
  }

  // Following for loop is copied from sample.  Shame on me!!!!!!!
  for (int i = 0; i < fNChs; i++) {
    // Set the number of samples for each waveform (you can set different RL
    // for different channels)
    err = CAEN_DGTZ_SetRecordLength(fHandler, fRecordLength, i);

    // Set a DC offset to the input signal to adapt it to digitizer's dynamic
    // range
    // err = CAEN_DGTZ_SetChannelDCOffset(fHandler, i, (1 << fNBits));
    err = CAEN_DGTZ_SetChannelDCOffset(fHandler, i, 50000);  // sample

    // Set the Pre-Trigger size (in samples)
    err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, i, 80);

    // Set the polarity for the given channel (CAEN_DGTZ_PulsePolarityPositive
    // or CAEN_DGTZ_PulsePolarityNegative)
    err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, i,
                                            CAEN_DGTZ_PulsePolarityNegative);
  }

  AllocateMemory();

  BoardCalibration();

  // Set register to use extended 47 bit time stamp
  // But, for 1725 with PHA, it is not working as same as 1730 with PSD
  // for (int i = 0; i < fNChs; i++) RegisterSetBits(0x10A0 + (i << 8), 8, 10,
  // 2);
}

void TPHA::ReadEvents()
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
                               (void **)fppPHAEvents, nEvents);
  PrintError(err, "GetDPPEvents");

  // for (int i = 0; i < nEvents[0]; i++)
  //   std::cout << (fppPHAEvents[0][i]).ChargeLong << std::endl;

  for (int iCh = 0; iCh < fNChs; iCh++) {
    for (int iEve = 0; iEve < nEvents[iCh]; iEve++) {
      err = CAEN_DGTZ_DecodeDPPWaveforms(fHandler, &fppPHAEvents[iCh][iEve],
                                         fpPHAWaveform);
      PrintError(err, "DecodeDPPWaveforms");

      auto index = fNEvents * ONE_HIT_SIZE;
      fDataArray[index++] = fModNumber;  // fModNumber is needed.
      fDataArray[index++] = iCh;         // int to char.  Dangerous

      constexpr uint64_t timeMask = 0x7FFFFFFF;
      auto tdc =
          (fppPHAEvents[iCh][iEve].TimeTag & timeMask) + fTimeOffset[iCh];
      if (tdc < fPreviousTime[iCh]) {
        tdc += (timeMask + 1);
        fTimeOffset[iCh] += (timeMask + 1);
      }
      fPreviousTime[iCh] = tdc;

      fTime[iCh] = tdc;
      constexpr auto timeSize = sizeof(fTime[0]);
      memcpy(&fDataArray[index], &fTime[iCh], timeSize);
      index += timeSize;

      constexpr auto adcSize = sizeof(fppPHAEvents[0][0].Energy);
      // auto adc = sumCharge;
      memcpy(&fDataArray[index], &fppPHAEvents[iCh][iEve].Energy, adcSize);
      index += adcSize;

      // std::cout << fppPHAEvents[iCh][iEve].ChargeLong << std::endl;

      constexpr auto waveSize = sizeof(fpPHAWaveform->Trace1[0]) * kNSamples;
      memcpy(&fDataArray[index], fpPHAWaveform->Trace1, waveSize);

      fNEvents++;
    }
  }
}

void TPHA::SetParameters()
{
  fRecordLength = kNSamples;
  // fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
  fPostTriggerSize = 80;
  fBLTEvents = 1023;  // It is max, why not 1024?

  SetPHAPar();
}

void TPHA::SetPHAPar()
{
  // Copy from sample
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPHA.thr[iCh] = 200;     // Trigger Threshold
    fParPHA.k[iCh] = 30;        // Trapezoid Rise Time (N*10ns)
    fParPHA.m[iCh] = 100;       // Trapezoid Flat Top  (N*10ns)
    fParPHA.M[iCh] = 50;        // Decay Time Constant (N*10ns) HACK-FPEP the
                                // one expected from fitting algorithm?
    fParPHA.ftd[iCh] = 8;       // Flat top delay (peaking time) (N*10ns) ??
    fParPHA.a[iCh] = 4;         // Trigger Filter smoothing factor
    fParPHA.b[iCh] = 20;        // Input Signal Rise time (N*10ns)
    fParPHA.trgho[iCh] = 1200;  // Trigger Hold Off
    fParPHA.nsbl[iCh] = 4;      // 3 = bx10 = 64 samples
    fParPHA.nspk[iCh] = 0;
    fParPHA.pkho[iCh] = 2000;
    fParPHA.blho[iCh] = 500;
    fParPHA.enf[iCh] = 1.0;  // Energy Normalization Factor
    fParPHA.decimation[iCh] = 0;
    fParPHA.dgain[iCh] = 0;
    fParPHA.otrej[iCh] = 0;
    fParPHA.trgwin[iCh] = 0;
    fParPHA.twwdt[iCh] = 0;
    // fParPHA.tsampl[iCh] = 10;
    // fParPHA.dgain[iCh] = 1;
  }
}

void TPHA::AcquisitionConfig()
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

void TPHA::TriggerConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Set the trigger threshold
  // The unit of its are V
  int32_t th = fabs((1 << fNBits) * (fVth / fVpp));
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPHA.thr[iCh] = th;
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

void TPHA::AllocateMemory()
{
  CAEN_DGTZ_ErrorCode err;
  uint32_t size;

  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer, &size);
  PrintError(err, "MallocReadoutBuffer");

  fppPHAEvents = new CAEN_DGTZ_DPP_PHA_Event_t *[fNChs];
  err = CAEN_DGTZ_MallocDPPEvents(fHandler, (void **)fppPHAEvents, &size);
  PrintError(err, "MallocDPPEvents");

  err = CAEN_DGTZ_MallocDPPWaveforms(fHandler, (void **)&fpPHAWaveform, &size);
  PrintError(err, "MallocDPPWaveforms");
}

void TPHA::FreeMemory()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
  PrintError(err, "FreeReadoutBuffer");
  // DelPointer(fpReadoutBuffer);

  err = CAEN_DGTZ_FreeDPPEvents(fHandler, (void **)fppPHAEvents);
  PrintError(err, "FreeDPPEvents");
  // DelPointer(fppPHAEvents);

  err = CAEN_DGTZ_FreeDPPWaveforms(fHandler, fpPHAWaveform);
  PrintError(err, "FreeDPPWaveforms");
  // DelPointer(fpPHAWaveform);
}

CAEN_DGTZ_ErrorCode TPHA::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  for (auto &t : fTime) t = 0;
  for (auto &t : fTimeOffset) t = 0;
  for (auto &t : fPreviousTime) t = 0;

  return err;
}

void TPHA::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  // err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  // PrintError(err, "FreeEvent");
}

void TPHA::SetMaster()
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

void TPHA::SetSlave()
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

void TPHA::StartSyncMode(uint32_t nMods)
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

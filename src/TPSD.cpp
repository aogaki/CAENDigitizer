#include <math.h>
#include <string.h>
#include <fstream>
#include <iostream>

#include "TPSD.hpp"
#include "TStdData.hpp"

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

TPSD::TPSD()
    : TDigitizer(),
      fpReadoutBuffer(nullptr),
      fppPSDEvents(nullptr),
      fpPSDWaveform(nullptr)
{
  SetParameters();
}

TPSD::TPSD(CAEN_DGTZ_ConnectionType type, int link, int node, uint32_t VMEadd)
    : TPSD()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();

  fTime.resize(fNChs);
  fTimeOffset.resize(fNChs);
  fPreviousTime.resize(fNChs);

  fDataArray = new unsigned char[fBLTEvents * ONE_HIT_SIZE * fNChs * 10];
}

TPSD::~TPSD()
{
  Reset();
  // FreeMemory();
  Close();
}

void TPSD::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  err = CAEN_DGTZ_SetChannelEnableMask(fHandler, fChMask);
  PrintError(err, "SetChannelEnableMask");

  err = CAEN_DGTZ_SetIOLevel(fHandler, CAEN_DGTZ_IOLevel_NIM);
  PrintError(err, "SetIOLevel");

  // Buffer setting
  err = CAEN_DGTZ_SetNumEventsPerAggregate(fHandler, fBLTEvents);
  PrintError(err, "SetNumEventsPerAggregate");

  // Synchronization Mode
  err = CAEN_DGTZ_SetRunSynchronizationMode(fHandler,
                                            CAEN_DGTZ_RUN_SYNC_Disabled);
  PrintError(err, "SetRunSynchronizationMode");

  SetPSDPar();
  if (fFirmware == FirmWareCode::DPP_PSD) {
    err = CAEN_DGTZ_SetDPPParameters(fHandler, fChMask, &fParPSD);
    PrintError(err, "SetDPPParameters");
  }
  // Following for loop is copied from sample.  Shame on me!!!!!!!
  for (uint i = 0; i < fNChs; i++) {
    // Set the number of samples for each waveform (you can set different RL
    // for different channels)
    err = CAEN_DGTZ_SetRecordLength(fHandler, fRecordLength, i);

    // Set the Pre-Trigger size (in samples)
    err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, i, 0.2 * kNSamples);

    err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, i, fPolarity);

    // Dynamic range
    err = CAEN_DGTZ_WriteRegister(fHandler, 0x1028 + (i << 8), 0);

    uint16_t offset = 0xFFFF * fDCOffset;
    err = CAEN_DGTZ_SetChannelDCOffset(fHandler, i, offset);
  }

  BoardCalibration();

  // Set register to use extended 47 bit time stamp
  for (uint i = 0; i < fNChs; i++) RegisterSetBits(0x1084 + (i << 8), 8, 10, 0);

  // 0 means automatically set
  err = CAEN_DGTZ_SetDPPEventAggregation(fHandler, 0, 0);
  PrintError(err, "SetDPPEventAggregation");
}

void TPSD::ReadEvents()
{
  fNEvents = 0;  // Event counter.  This should be the first of this function
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

  for (uint iCh = 0; iCh < fNChs; iCh++) {
    for (uint iEve = 0; iEve < nEvents[iCh]; iEve++) {
      err = CAEN_DGTZ_DecodeDPPWaveforms(fHandler, &fppPSDEvents[iCh][iEve],
                                         fpPSDWaveform);
      PrintError(err, "DecodeDPPWaveforms");

      auto tdc =
          fppPSDEvents[iCh][iEve].TimeTag +
          ((uint64_t)((fppPSDEvents[iCh][iEve].Extras >> 16) & 0xFFFF) << 31);
      if (fTSample > 0) tdc *= fTSample;
      fTime[iCh] = tdc;

      auto index = fNEvents * ONE_HIT_SIZE;
      fDataArray[index++] = fModNumber;  // fModNumber is needed.
      fDataArray[index++] = iCh;         // int to char.  Dangerous

      constexpr auto timeSize = sizeof(fTime[0]);
      memcpy(&fDataArray[index], &fTime[iCh], timeSize);
      index += timeSize;

      constexpr auto adcSize = sizeof(fppPSDEvents[0][0].ChargeLong);
      // auto adc = sumCharge;
      memcpy(&fDataArray[index], &fppPSDEvents[iCh][iEve].ChargeLong, adcSize);
      index += adcSize;

      constexpr auto waveSize = sizeof(fpPSDWaveform->Trace1[0]) * kNSamples;
      memcpy(&fDataArray[index], fpPSDWaveform->Trace1, waveSize);

      fNEvents++;
    }
  }
}

void TPSD::ReadPar()
{
  std::ifstream fin("par.dat");
  std::string key, val;
  while (fin >> key >> val) {
    std::cout << key << "\t" << val << std::endl;
    if (key == "Polarity") {
      if (val == "Positive")
        fPolarity = CAEN_DGTZ_PulsePolarityPositive;
      else if (val == "Negative")
        fPolarity = CAEN_DGTZ_PulsePolarityNegative;
      else
        std::cout << val << " is not used for " << val << std::endl;
    } else if (key == "DCOffset") {
      auto offset = std::stod(val);
      if (offset < 0. || offset > 1.)
        std::cout << "DCOffset should be 0. to 1." << std::endl;
      else
        fDCOffset = offset;
    } else if (key == "Threshold") {
      auto th = std::stoi(val);
      fVth = th;
    }
  }
  fin.close();
}

void TPSD::SetParameters()
{
  fRecordLength = kNSamples;
  fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_ONLY;
  // fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
  fPostTriggerSize = 80;
  fBLTEvents = 1023;  // It is max, why not 1024?
  fDCOffset = 0.5;
  fVth = 100;
  fPolarity = CAEN_DGTZ_PulsePolarityNegative;
  fChMask = 0b00000001;

  ReadPar();

  void SetPSDPar();
}

void TPSD::SetPSDPar()
{  // Copy from sample
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPSD.thr[iCh] = fVth;  // Trigger Threshold
    /* The following parameter is used to specifiy the number of samples for
    the baseline averaging: 0 -> absolute Bl 1 -> 16samp 2 -> 64samp 3 ->
    256samp 4 -> 1024samp */
    fParPSD.nsbl[iCh] = 2;
    fParPSD.lgate[iCh] =
        100;  // Long Gate Width (N*2ns for x730  and N*4ns for x725)
    fParPSD.sgate[iCh] =
        24;  // Short Gate Width (N*2ns for x730  and N*4ns for x725)
    fParPSD.pgate[iCh] =
        8;  // Pre Gate Width (N*2ns for x730  and N*4ns for x725)
    /* Self Trigger Mode:
    0 -> Disabled
    1 -> Enabled */
    fParPSD.selft[iCh] = 1;

    // Trigger configuration:
    //    CAEN_DGTZ_DPP_TriggerMode_Normal ->  Each channel can self-trigger
    //    independently from the other channels
    //    CAEN_DGTZ_DPP_TriggerMode_Coincidence -> A validation signal must
    //    occur inside the shaped trigger coincidence window
    // fParPSD.trgc[iCh] =
    //     CAEN_DGTZ_DPP_TriggerConfig_t(CAEN_DGTZ_DPP_TriggerMode_Normal);
    fParPSD.trgc[iCh] = CAEN_DGTZ_DPP_TriggerConfig_Threshold;

    /*Discrimination mode for the event selection
    CAEN_DGTZ_DPP_DISCR_MODE_LED -> Leading Edge Distrimination
    CAEN_DGTZ_DPP_DISCR_MODE_CFD -> Constant Fraction Distrimination*/
    fParPSD.discr[iCh] = CAEN_DGTZ_DPP_DISCR_MODE_LED;
    // fParPSD.discr[iCh] = CAEN_DGTZ_DPP_DISCR_MODE_CFD;

    /*CFD delay (N*2ns for x730  and N*4ns for x725)  */
    fParPSD.cfdd[iCh] = 4;

    /*CFD fraction: 0->25%; 1->50%; 2->75%; 3->100% */
    fParPSD.cfdf[iCh] = 0;

    /* Trigger Validation Acquisition Window */
    fParPSD.tvaw[iCh] = 50;

    /* Charge sensibility:
                Options for Input Range 2Vpp: 0->5fC/LSB; 1->20fC/LSB;
       2->80fC/LSB; 3->320fC/LSB; 4->1.28pC/LSB; 5->5.12pC/LSB Options for
       Input Range 0.5Vpp: 0->1.25fC/LSB; 1->5fC/LSB; 2->20fC/LSB;
       3->80fC/LSB; 4->320fC/LSB; 5->1.28pC/LSB */
    fParPSD.csens[iCh] = 1;
  }
  /* Pile-Up rejection Mode
  CAEN_DGTZ_DPP_PSD_PUR_DetectOnly -> Only Detect Pile-Up
  CAEN_DGTZ_DPP_PSD_PUR_Enabled -> Reject Pile-Up */
  fParPSD.purh = CAEN_DGTZ_DPP_PSD_PUR_DetectOnly;
  fParPSD.purgap = 10;  // Purity Gap in LSB units (1LSB = 0.12 mV for 2Vpp
                        // Input Range, 1LSB = 0.03 mV for 0.5 Vpp Input Range )
  fParPSD.blthr = 3;    // Baseline Threshold
  fParPSD.trgho = 8;    // Trigger HoldOff
}

void TPSD::AcquisitionConfig()
{
  CAEN_DGTZ_ErrorCode err;
  // Mix means waveform list and energy
  err = CAEN_DGTZ_SetDPPAcquisitionMode(fHandler, CAEN_DGTZ_DPP_ACQ_MODE_Mixed,
                                        CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
  PrintError(err, "SetDPPAcquisitionMode");

  err = CAEN_DGTZ_SetChannelEnableMask(fHandler, fChMask);
  PrintError(err, "SetChannelEnableMask");

  // Set the acquisition mode
  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  // Set record length (length of waveform?);
  err = CAEN_DGTZ_SetRecordLength(fHandler, fRecordLength);
  PrintError(err, "SetRecordLength");
}

void TPSD::TriggerConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Set the triggermode
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, fTriggerMode, fChMask);
  PrintError(err, "SetChannelSelfTrigger");

  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, fTriggerMode);
  PrintError(err, "SetSWTriggerMode");

  // uint32_t samples = fRecordLength * 0.1;
  // for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
  //   err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, iCh, samples);
  //   PrintError(err, "SetDPPPreTriggerSize");
  // }
  //
  // for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
  //   err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, iCh, fPolarity);
  //   PrintError(err, "CAEN_DGTZ_SetChannelPulsePolarity");
  // }

  err = CAEN_DGTZ_SetDPPTriggerMode(
      fHandler, CAEN_DGTZ_DPP_TriggerMode_t::CAEN_DGTZ_DPP_TriggerMode_Normal);
  PrintError(err, "SetDPPTriggerMode");
}

void TPSD::AllocateMemory()
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

void TPSD::FreeMemory()
{
  CAEN_DGTZ_ErrorCode err;
  if (fpReadoutBuffer != nullptr) {
    err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
    PrintError(err, "FreeReadoutBuffer");
    fpReadoutBuffer = nullptr;
  }
  if (fppPSDEvents != nullptr) {
    err = CAEN_DGTZ_FreeDPPEvents(fHandler, (void **)fppPSDEvents);
    PrintError(err, "FreeDPPEvents");
    fppPSDEvents = nullptr;
  }
  if (fpPSDWaveform != nullptr) {
    err = CAEN_DGTZ_FreeDPPWaveforms(fHandler, fpPSDWaveform);
    PrintError(err, "FreeDPPWaveforms");
    fpPSDWaveform = nullptr;
  }
}

CAEN_DGTZ_ErrorCode TPSD::StartAcquisition()
{
  AllocateMemory();

  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  for (auto &t : fTime) t = 0;
  for (auto &t : fTimeOffset) t = 0;
  for (auto &t : fPreviousTime) t = 0;

  return err;
}

void TPSD::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  FreeMemory();
  // err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  // PrintError(err, "FreeEvent");
}

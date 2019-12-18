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
    : TDPP(),
      fpReadoutBuffer(nullptr),
      fppPSDEvents(nullptr),
      fpPSDWaveform(nullptr)
{
  InitParameters();
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

  fDataArray =
      new unsigned char[fParameters.GetBLTEvents() * ONE_HIT_SIZE * fNChs * 10];
}

TPSD::~TPSD()
{
  Reset();
  FreeMemory();
  Close();
}

void TPSD::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  err = CAEN_DGTZ_SetIOLevel(fHandler, CAEN_DGTZ_IOLevel_NIM);
  PrintError(err, "SetIOLevel");

  // Buffer setting
  err =
      CAEN_DGTZ_SetNumEventsPerAggregate(fHandler, fParameters.GetBLTEvents());
  PrintError(err, "SetNumEventsPerAggregate");

  // Synchronization Mode
  err = CAEN_DGTZ_SetRunSynchronizationMode(fHandler,
                                            CAEN_DGTZ_RUN_SYNC_Disabled);
  PrintError(err, "SetRunSynchronizationMode");

  // SetPSDPar();
  if (fFirmware == FirmWareCode::DPP_PSD) {
    err =
        CAEN_DGTZ_SetDPPParameters(fHandler, fParameters.GetChMask(), &fParPSD);
    PrintError(err, "SetDPPParameters");
  }

  // Following for loop is copied from sample.  Shame on me!!!!!!!
  for (uint i = 0; i < fNChs; i++) {
    // Set the number of samples for each waveform (you can set different RL
    // for different channels)
    err = CAEN_DGTZ_SetRecordLength(fHandler, fParameters.GetRecordLength(), i);

    // Set the Pre-Trigger size (in samples)
    err = CAEN_DGTZ_SetDPPPreTriggerSize(fHandler, i, 0.2 * kNSamples);

    err = CAEN_DGTZ_SetChannelPulsePolarity(fHandler, i,
                                            fParameters.GetPolarity());

    // Dynamic range
    err = CAEN_DGTZ_WriteRegister(fHandler, 0x1028 + (i << 8), 0);

    uint16_t offset = 0xFFFF * fParameters.GetDCOffset();
    err = CAEN_DGTZ_SetChannelDCOffset(fHandler, i, offset);

    // Set register to use extended 47 bit time stamp
    RegisterSetBits(0x1084 + (i << 8), 8, 10, 0);

    // Set register to use extended time stamp, flags and fine time stamp
    // RegisterSetBits(0x1084 + (i << 8), 8, 10, 2);
  }

  // TriggerTest();

  BoardCalibration();

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
    if (((fParameters.GetChMask() >> iCh) & 0b1) != 0b1) continue;
    for (uint iEve = 0; iEve < nEvents[iCh]; iEve++) {
      err = CAEN_DGTZ_DecodeDPPWaveforms(fHandler, &fppPSDEvents[iCh][iEve],
                                         fpPSDWaveform);
      PrintError(err, "DecodeDPPWaveforms");

      // For Extended time stamp
      auto tdc =
          fppPSDEvents[iCh][iEve].TimeTag +
          ((uint64_t)((fppPSDEvents[iCh][iEve].Extras >> 16) & 0xFFFF) << 31);
      if (fTSample > 0) tdc *= fTSample;
      // Fine time stamp
      // tdc = (fppPSDEvents[iCh][iEve].Extras & 0x3FF);

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

void TPSD::InitParameters() { SetPSDPar(); }

void TPSD::SetPSDPar()
{
  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    fParPSD.thr[iCh] = fParameters.GetThreshold(iCh);
    fParPSD.nsbl[iCh] = fParameters.GetBaselineSamples(iCh);
    fParPSD.lgate[iCh] = fParameters.GetLongGate(iCh);
    fParPSD.sgate[iCh] = fParameters.GetShortGate(iCh);
    fParPSD.pgate[iCh] = fParameters.GetPreTriggerSize();
    fParPSD.selft[iCh] = fParameters.GetSelfTrigger(iCh);
    // fParPSD.trgc[iCh] = CAEN_DGTZ_DPP_TriggerConfig_Threshold;
    fParPSD.trgc[iCh] = fParameters.GetDPPTriggerConfig();  // Deprecated?
    fParPSD.discr[iCh] = fParameters.GetDiscrMode(iCh);
    fParPSD.cfdd[iCh] = fParameters.GetCFDDelay(iCh);
    fParPSD.cfdf[iCh] = fParameters.GetCFDFraction(iCh);
    fParPSD.tvaw[iCh] = fParameters.GetTrgValAccWindow(iCh);
    fParPSD.csens[iCh] = fParameters.GetChargeSensitivity(iCh);
  }
  fParPSD.purh = fParameters.GetPileUpOption();
  fParPSD.purgap = fParameters.GetPileUpRejGap();
  fParPSD.blthr = 3;  // Deprecated?
  fParPSD.trgho = fParameters.GetTriggerHoldOff();
}

void TPSD::AcquisitionConfig()
{
  CAEN_DGTZ_ErrorCode err;
  // Mix means waveform list and energy
  err = CAEN_DGTZ_SetDPPAcquisitionMode(fHandler, CAEN_DGTZ_DPP_ACQ_MODE_Mixed,
                                        CAEN_DGTZ_DPP_SAVE_PARAM_EnergyAndTime);
  PrintError(err, "SetDPPAcquisitionMode");

  err = CAEN_DGTZ_SetChannelEnableMask(fHandler, fParameters.GetChMask());
  PrintError(err, "SetChannelEnableMask");

  // Set the acquisition mode
  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  // Set record length (length of waveform?);
  err = CAEN_DGTZ_SetRecordLength(fHandler, fParameters.GetRecordLength());
  PrintError(err, "SetRecordLength");
}

void TPSD::TriggerConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Set the trigger modes
  // Not DPP functions
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, fParameters.GetTriggerMode(),
                                        fParameters.GetChMask());
  PrintError(err, "SetChannelSelfTrigger");

  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, fParameters.GetTriggerMode());
  PrintError(err, "SetSWTriggerMode");

  // DPP trigger mode
  err = CAEN_DGTZ_SetDPPTriggerMode(fHandler, fParameters.GetDPPTriggerMode());
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

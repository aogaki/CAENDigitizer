#include <iostream>

#include "TWaveRecord.hpp"

TWaveRecord::TWaveRecord()
    : fpReadoutBuffer(nullptr),
      fpEventPtr(nullptr),
      fpEventStd(nullptr),
      fMaxBufferSize(0),
      fBufferSize(0),
      fNEvents(0),
      fReadSize(0),
      fBLTEvents(0),
      fRecordLength(0),
      fBaseLine(0),
      fVpp(0.),
      fVth(0.),
      fTriggerMode(CAEN_DGTZ_TRGMODE_ACQ_ONLY),
      fPolarity(CAEN_DGTZ_TriggerOnRisingEdge),
      fPostTriggerSize(50),
      fGateSize(0),
      fTimeOffset(0),
      fPreviousTime(0),
      fData(nullptr)
{
  SetParameters();
}

TWaveRecord::TWaveRecord(CAEN_DGTZ_ConnectionType type, int link, int node,
                         uint32_t VMEadd)
    : TWaveRecord()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();
}

TWaveRecord::~TWaveRecord()
{
  auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
  PrintError(err, "FreeReadoutBuffer");
  Reset();
  Close();

  delete fData;
}

void TWaveRecord::SetParameters()
{
  // Reading parameter functions should be implemented!!!!!!!
  fRecordLength = kNSamples;
  fBLTEvents = 200;
  fVpp = 2.;
  fVth = -0.03;
  // fVth = -0.001;
  fPolarity = CAEN_DGTZ_TriggerOnFallingEdge;
  // fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
  fPostTriggerSize = 80;
  fGateSize = 400;  // ns

  fData = new std::vector<TStdData>;
  fData->reserve(fBLTEvents * 32);  // 32 means nothing.  minimum is No. chs
}

void TWaveRecord::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, fBLTEvents);
  PrintError(err, "SetMaxNEventsBLT");
  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
                                      &fMaxBufferSize);
  PrintError(err, "MallocReadoutBuffer");

  BoardCalibration();
}

void TWaveRecord::ReadEvents()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                           fpReadoutBuffer, &fBufferSize);
  PrintError(err, "ReadData");

  err =
      CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, fBufferSize, &fNEvents);
  PrintError(err, "GetNumEvents");
  // std::cout << fNEvents << " Events" << std::endl;

  fData->clear();
  TStdData data;
  for (int32_t iEve = 0; iEve < fNEvents; iEve++) {
    err = CAEN_DGTZ_GetEventInfo(fHandler, fpReadoutBuffer, fBufferSize, iEve,
                                 &fEventInfo, &fpEventPtr);
    PrintError(err, "GetEventInfo");
    // std::cout << "Event number:\t" << iEve << '\n'
    //           << "Event size:\t" << fEventInfo.EventSize << '\n'
    //           << "Board ID:\t" << fEventInfo.BoardId << '\n'
    //           << "Pattern:\t" << fEventInfo.Pattern << '\n'
    //           << "Ch mask:\t" << fEventInfo.ChannelMask << '\n'
    //           << "Event counter:\t" << fEventInfo.EventCounter << '\n'
    //           << "Trigger time tag:\t" << fEventInfo.TriggerTimeTag
    //           << std::endl;

    err = CAEN_DGTZ_DecodeEvent(fHandler, fpEventPtr, (void **)&fpEventStd);
    PrintError(err, "DecodeEvent");

    for (int iCh = 0; iCh < fNChs; iCh++) {
      const uint32_t chSize = fpEventStd->ChSize[iCh];
      // if (chSize != kNSamples) {
      //   std::cout << "No. samples of wave form error" << std::endl;
      // }

      int32_t sumCharge = 0.;
      // for (uint32_t i = 0; i < chSize; i++) {
      // const uint32_t start = 0;
      int32_t start = (chSize * (100 - fPostTriggerSize) / 100) -
                      (fGateSize / fTSample / 2);
      if (start < 0) start = 0;
      // const uint32_t stop = chSize;
      int32_t stop = (chSize * (100 - fPostTriggerSize) / 100) +
                     (fGateSize / fTSample / 2);
      uint32_t baseSample = 256;
      if (baseSample > start) baseSample = start - 1;
      fBaseLine = 0;
      for (uint32_t i = 0; i < baseSample; i++) {
        fBaseLine += fpEventStd->DataChannel[iCh][i];
      }
      fBaseLine /= baseSample;

      for (uint32_t i = start; i < stop; i++) {
        sumCharge += (fBaseLine - fpEventStd->DataChannel[iCh][i]);
        // std::cout << fBaseLine <<"\t"<< fpEventStd->DataChannel[iCh][i] <<
        // std::endl;
      }

      // uint64_t timeStamp = fEventInfo.TriggerTimeTag + fTimeOffset;
      // if (timeStamp < fPreviousTime) {
      //   timeStamp += 0xFFFFFFFF;
      //   fTimeOffset += 0xFFFFFFFF;
      // }
      // fPreviousTime = timeStamp;
      // timeStamp *= fTSample;

      data.ModNumber = 0;  // fModNumber is needed.
      data.ChNumber = iCh;
      data.ADC = sumCharge;
      // data.TimeStamp = timeStamp;
      data.TimeStamp = fEventInfo.TriggerTimeTag;
      for (uint32_t i = 0; i < kNSamples; i++)
        data.Waveform[i] = fpEventStd->DataChannel[iCh][i];
      fData->push_back(data);
      // std::cout << "time:\t" << fEventInfo.TriggerTimeTag << "\t" <<
      // timeStamp
      //<< std::endl;
    }
  }
}

void TWaveRecord::AcquisitionConfig()
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
}

void TWaveRecord::TriggerConfig()
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
    // Think about multiple channel setting
    err = CAEN_DGTZ_SetChannelTriggerThreshold(fHandler, iCh, thVal);
    PrintError(err, "SetChannelTriggerThreshold");
  }

  // Set the triggermode
  uint32_t mask = ((1 << fNChs) - 1);
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, fTriggerMode, mask);
  PrintError(err, "SetChannelSelfTrigger");
  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, fTriggerMode);
  PrintError(err, "SetSWTriggerMode");

  // Set post trigger size
  err = CAEN_DGTZ_SetPostTriggerSize(fHandler, fPostTriggerSize);
  PrintError(err, "SetPostTriggerSize");

  // Set the triiger polarity
  for (uint32_t iCh = 0; iCh < fNChs; iCh++)
    CAEN_DGTZ_SetTriggerPolarity(fHandler, iCh, fPolarity);

  CAEN_DGTZ_TriggerPolarity_t pol;
  CAEN_DGTZ_GetTriggerPolarity(fHandler, 0, &pol);
  std::cout << "Polarity:\t" << pol << std::endl;
}

void TWaveRecord::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "AllocateEvent");

  fTimeOffset = 0;
  fPreviousTime = 0;
}

void TWaveRecord::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "FreeEvent");
}

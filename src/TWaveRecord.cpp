#include <string.h>

#include <iostream>

#include "TWaveRecord.hpp"

TWaveRecord::TWaveRecord()
    : fpReadoutBuffer(nullptr),
      fpEventPtr(nullptr),
      fpEventStd(nullptr),
      fMaxBufferSize(0),
      fBufferSize(0),
      fTimeOffset(0),
      fPreviousTime(0)
{
  InitParameters();
}

TWaveRecord::TWaveRecord(CAEN_DGTZ_ConnectionType type, int link, int node,
                         uint32_t VMEadd)
    : TWaveRecord()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();

  fData = new std::vector<WaveFormData_t>;
}

TWaveRecord::~TWaveRecord()
{
  auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
  PrintError(err, "FreeReadoutBuffer");
  Reset();
  Close();

  delete fData;
}

void TWaveRecord::InitParameters()
{
  fParameters.SetRecordLength(512);
  fParameters.SetBLTEvents(512);
  fParameters.SetVpp(2.);
  fParameters.SetVth(-0.5);
  fParameters.SetDCOffset(0.8);
  fParameters.SetPolarity(CAEN_DGTZ_TriggerOnFallingEdge);
  fParameters.SetTriggerMode(CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  fParameters.SetPostTriggerSize(80);
  fParameters.SetChMask(0b11111111);
}

void TWaveRecord::SetParameter(TWaveRecordPar par)
{
  fParameters.SetRecordLength(par.GetRecordLength());
  fParameters.SetBLTEvents(par.GetBLTEvents());
  fParameters.SetVpp(par.GetVpp());
  fParameters.SetVth(par.GetVth());
  fParameters.SetDCOffset(par.GetDCOffset());
  fParameters.SetPolarity(par.GetPolarity());
  fParameters.SetTriggerMode(par.GetTriggerMode());
  fParameters.SetPostTriggerSize(par.GetPostTriggerSize());
  fParameters.SetChMask(par.GetChMask());
}

void TWaveRecord::Initialize()
{
  CAEN_DGTZ_ErrorCode err;

  Reset();
  AcquisitionConfig();
  TriggerConfig();

  err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, fParameters.GetBLTEvents());
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

  uint32_t nEvents;
  err =
      CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, fBufferSize, &nEvents);
  PrintError(err, "GetNumEvents");
  // std::cout << nEvents << " Events" << std::endl;

  // fData->clear();
  fData->clear();
  for (uint iEve = 0; iEve < nEvents; iEve++) {
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

    uint64_t timeStamp = (fEventInfo.TriggerTimeTag + fTimeOffset) * fTSample;
    if (timeStamp < fPreviousTime) {
      constexpr uint32_t maxTime = 0xFFFFFFFF / 2;  // Check manual
      timeStamp += maxTime * fTSample;
      fTimeOffset += maxTime;
    }
    fPreviousTime = timeStamp;

    // All channels have same time stamp.
    // Need speed or reduce memory, first think this
    for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
      WaveFormData_t dataEle;
      dataEle.ModNumber = fModNumber;
      dataEle.ChNumber = iCh;
      dataEle.TimeStamp = timeStamp;
      dataEle.RecordLength = fpEventStd->ChSize[iCh];
      dataEle.WaveForm = fpEventStd->DataChannel[iCh];

      fData->push_back(dataEle);
    }
  }
}

void TWaveRecord::AcquisitionConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Eanble all channels
  err = CAEN_DGTZ_SetChannelEnableMask(fHandler, fParameters.GetChMask());
  PrintError(err, "SetChannelEnableMask");

  // Set DC offset
  auto fac = (1. - fParameters.GetDCOffset());
  if (fac <= 0. || fac >= 1.) fac = 0.5;
  uint32_t offset = 0xFFFF * fac;
  // uint32_t offset = (1 << fNBits) * fac;
  for (uint32_t iCh = 0; iCh < fNChs; iCh++)
    err = CAEN_DGTZ_SetChannelDCOffset(fHandler, iCh, offset);
  PrintError(err, "SetChannelDCOffset");

  // Set the acquisition mode
  err = CAEN_DGTZ_SetAcquisitionMode(fHandler, CAEN_DGTZ_SW_CONTROLLED);
  PrintError(err, "SetAcquisitionMode");

  // Set record length (length of waveform?);
  err = CAEN_DGTZ_SetRecordLength(fHandler, fParameters.GetRecordLength());
  PrintError(err, "SetRecordLength");
}

void TWaveRecord::TriggerConfig()
{
  CAEN_DGTZ_ErrorCode err;

  // Set the trigger threshold
  // The unit of its are V
  // To calculate offset is a little bit difference from DC offset setting
  int32_t th = (1 << fNBits) * (fParameters.GetVth() / fParameters.GetVpp());
  auto offset = (1 << fNBits) * fParameters.GetDCOffset();
  auto thVal = th + offset;
  // std::cout << "Vth:\t" << thVal << "\t" << fParameters.GetVth() << std::endl;

  for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
    // Think about multiple channel setting
    err = CAEN_DGTZ_SetChannelTriggerThreshold(fHandler, iCh, thVal);
    PrintError(err, "SetChannelTriggerThreshold");
  }

  // Set the triggermode
  err = CAEN_DGTZ_SetChannelSelfTrigger(fHandler, fParameters.GetTriggerMode(),
                                        fParameters.GetChMask());
  PrintError(err, "SetChannelSelfTrigger");
  err = CAEN_DGTZ_SetSWTriggerMode(fHandler, fParameters.GetTriggerMode());
  PrintError(err, "SetSWTriggerMode");

  // Set post trigger size
  err =
      CAEN_DGTZ_SetPostTriggerSize(fHandler, fParameters.GetPostTriggerSize());
  PrintError(err, "SetPostTriggerSize");

  // Set the triiger polarity
  for (uint32_t iCh = 0; iCh < fNChs; iCh++)
    CAEN_DGTZ_SetTriggerPolarity(fHandler, iCh, fParameters.GetPolarity());

  CAEN_DGTZ_TriggerPolarity_t pol;
  CAEN_DGTZ_GetTriggerPolarity(fHandler, 0, &pol);
  std::cout << "Polarity:\t" << pol << std::endl;
}

CAEN_DGTZ_ErrorCode TWaveRecord::StartAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  err = CAEN_DGTZ_AllocateEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "AllocateEvent");

  fTimeOffset = 0;
  fPreviousTime = 0;

  fData->reserve(fParameters.GetBLTEvents() * fNChs);

  return err;
}

void TWaveRecord::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  PrintError(err, "FreeEvent");
}

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
      fCharge(nullptr),
      fTime(nullptr),
      fTimeOffset(0),
      fPreviousTime(0),
      fPlotWaveformFlag(false),
      fCanvas(nullptr),
      fGraph(nullptr),
      fData(nullptr)
{
  SetParameters();
}

TWaveRecord::TWaveRecord(CAEN_DGTZ_ConnectionType type, int link, int node,
                         uint32_t VMEadd, bool plotWaveform)
    : TWaveRecord()
{
  Open(type, link, node, VMEadd);
  Reset();
  GetBoardInfo();

  fPlotWaveformFlag = plotWaveform;
  if (fPlotWaveformFlag) {
    fCanvas = new TCanvas();
    fGraph = new TGraph();
    fGraph->SetMinimum(7500);
    fGraph->SetMaximum(8500);
  }
}

TWaveRecord::~TWaveRecord()
{
  auto err = CAEN_DGTZ_FreeReadoutBuffer(&fpReadoutBuffer);
  PrintError(err, "FreeReadoutBuffer");
  Reset();
  Close();

  delete fCharge;
  delete fTime;
  delete fData;

  if (fPlotWaveformFlag) {
    delete fCanvas;
    delete fGraph;
  }
}

void TWaveRecord::SetParameters()
{
  // Reading parameter functions should be implemented!!!!!!!
  fRecordLength = 4096;
  fBLTEvents = 1024;
  fVpp = 2.;
  fVth = -0.03;
  fPolarity = CAEN_DGTZ_TriggerOnFallingEdge;
  // fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
  fPostTriggerSize = 50;

  fCharge = new std::vector<int32_t>;
  fCharge->reserve(fBLTEvents * 4);
  fTime = new std::vector<uint64_t>;
  fTime->reserve(fBLTEvents * 4);

  fData = new std::vector<TStdData>;
  fData->reserve(fBLTEvents * 4);
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

  fCharge->clear();
  fTime->clear();
  fData->clear();
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
    // std::cout << "Trigger time tag:\t" << fEventInfo.TriggerTimeTag
    //           << std::endl;

    err = CAEN_DGTZ_DecodeEvent(fHandler, fpEventPtr, (void **)&fpEventStd);
    PrintError(err, "DecodeEvent");

    std::cout << "ch check" << std::endl;
    for (int iCh = 0; iCh < fNChs; iCh++) {
      const uint32_t chSize = fpEventStd->ChSize[iCh];
      std::cout << chSize << std::endl;
    }
    const uint32_t chSize = fpEventStd->ChSize[0];
    int32_t sumCharge = 0.;
    // for (uint32_t i = 0; i < chSize; i++) {
    const uint32_t start = 0;
    // const uint32_t start = 400;
    const uint32_t stop = chSize;
    // const uint32_t stop = 600;
    const uint32_t baseSample = 256;
    fBaseLine = 0;
    for (uint32_t i = 0; i < baseSample; i++) {
      fBaseLine += fpEventStd->DataChannel[0][i];
    }
    fBaseLine /= baseSample;

    for (uint32_t i = start; i < stop; i++) {
      if (fPlotWaveformFlag)
        fGraph->SetPoint(i - start, i, (fpEventStd->DataChannel[0])[i]);
      sumCharge += (fBaseLine - fpEventStd->DataChannel[0][i]);
      // std::cout << fBaseLine <<"\t"<< fpEventStd->DataChannel[0][i] <<
      // std::endl;
    }
    fCharge->push_back(sumCharge);

    uint64_t timeStamp = fEventInfo.TriggerTimeTag + fTimeOffset;
    if (timeStamp < fPreviousTime) {
      timeStamp += 0xFFFFFFFF;
      fTimeOffset += 0xFFFFFFFF;
    }
    fPreviousTime = timeStamp;
    timeStamp *= fTSample;
    fTime->push_back(timeStamp);
    // std::cout << "time:\t" << fEventInfo.TriggerTimeTag << "\t" << timeStamp
    //<< std::endl;
  }
  if (fPlotWaveformFlag) {
    if (fGraph->GetN() > 0) {
      fCanvas->cd();
      fGraph->Draw("AL");
      fCanvas->Update();
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
  uint32_t thVal = fBaseLine;
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

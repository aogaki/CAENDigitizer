#include <iostream>

#include <TH1.h>

#include "TWaveRecord.hpp"

TWaveRecord::TWaveRecord()
    : fHandler(-1),
      fpReadoutBuffer(nullptr),
      fpEventPtr(nullptr),
      fpEventStd(nullptr),
      fMaxBufferSize(0),
      fBufferSize(0),
      fNEvents(0),
      fReadSize(0),
      fDigitizerModel(0),
      fNChs(0),
      fTSample(0),
      fNBits(0),
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
      fGraph(nullptr)
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
  GetBaseLine();

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

  if (fPlotWaveformFlag) {
    delete fCanvas;
    delete fGraph;
  }
}

void TWaveRecord::SetParameters()
{
  // Reading parameter functions should be implemented!!!!!!!
  fRecordLength = 256;
  fBLTEvents = 10;
  fVpp = 2.;
  fVth = -0.003;
  fPolarity = CAEN_DGTZ_TriggerOnFallingEdge;
  // fTriggerMode = CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT;
  fPostTriggerSize = 50;

  fCharge = new std::vector<int32_t>;
  fCharge->reserve(fBLTEvents * 4);
  fTime = new std::vector<uint64_t>;
  fTime->reserve(fBLTEvents * 4);
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
  std::cout << fNEvents << " Events" << std::endl;

  fCharge->clear();
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

    const uint32_t chSize = fpEventStd->ChSize[0];
    int32_t sumCharge = 0.;
    // for (uint32_t i = 0; i < chSize; i++) {
    // const uint32_t start = 0;
    const uint32_t start = 50;
    // const uint32_t stop = chSize;
    const uint32_t stop = 100;
    const uint32_t baseSample = 20;
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
    std::cout << sumCharge << std::endl;
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

void TWaveRecord::GetBoardInfo()
{
  CAEN_DGTZ_BoardInfo_t info;
  auto err = CAEN_DGTZ_GetInfo(fHandler, &info);
  PrintError(err, "GetInfo");

  fNChs = info.Channels;

  std::cout << "Model name:\t" << info.ModelName << "\n"
            << "Model number:\t" << info.Model << "\n"
            << "No. channels:\t" << info.Channels << "\n"
            << "Format factor:\t" << info.FormFactor << "\n"
            << "Family code:\t" << info.FamilyCode << "\n"
            << "Firmware revision of the FPGA on the mother board (ROC):\t"
            << info.ROC_FirmwareRel << "\n"
            << "Firmware revision of the FPGA on the daughter board (AMC):\t"
            << info.AMC_FirmwareRel << "\n"
            << "Serial number:\t" << info.SerialNumber << "\n"
            << "PCB revision:\t" << info.PCB_Revision << "\n"
            << "No. bits of the ADC:\t" << info.ADC_NBits << "\n"
            << "Device handler of CAENComm:\t" << info.CommHandle << "\n"
            << "Device handler of CAENVME:\t" << info.VMEHandle << "\n"
            << "License number:\t" << info.License << std::endl;

  // Copy from digites
  if (info.FamilyCode == 5) {
    fDigitizerModel = 751;
    fTSample = 1;
    fNBits = 10;
  } else if (info.FamilyCode == 7) {
    fDigitizerModel = 780;
    fTSample = 10;
    fNBits = 14;
  } else if (info.FamilyCode == 13) {
    fDigitizerModel = 781;
    fTSample = 10;
    fNBits = 14;
  } else if (info.FamilyCode == 0) {
    fDigitizerModel = 724;
    fTSample = 10;
    fNBits = 14;
  } else if (info.FamilyCode == 11) {
    fDigitizerModel = 730;
    fTSample = 2;
    fNBits = 14;
  } else if (info.FamilyCode == 14) {
    fDigitizerModel = 725;
    fTSample = 4;
    fNBits = 14;
  } else if (info.FamilyCode == 3) {
    fDigitizerModel = 720;
    fTSample = 4;
    fNBits = 12;
  } else if (info.FamilyCode == 999) {  // temporary code for Hexagon
    fDigitizerModel = 5000;
    fTSample = 10;
    fNBits = 14;
  } else {
    PrintError(err, "Check Family code @ GetBoardInfo");
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

void TWaveRecord::BoardCalibration()
{
  if ((fDigitizerModel == 730) || (fDigitizerModel == 725)) {
    // Copy from digiTes
    // Honestly, I do not know what is this.
    uint32_t lock, ctrl;
    int ret = 0;
    for (uint32_t iCh = 0; iCh < fNChs; iCh++) {
      // enter engineering functions
      ret |= WriteSPIRegister(iCh, 0x7A, 0x59);
      ret |= WriteSPIRegister(iCh, 0x7A, 0x1A);
      ret |= WriteSPIRegister(iCh, 0x7A, 0x11);
      ret |= WriteSPIRegister(iCh, 0x7A, 0xAC);

      // read lock value
      ret |= ReadSPIRegister(iCh, 0xA7, lock);
      // write lock value
      ret |= WriteSPIRegister(iCh, 0xA5, lock);

      // enable lock
      ret |= ReadSPIRegister(iCh, 0xA4, ctrl);
      ctrl |= 0x4;  // set bit 2
      ret |= WriteSPIRegister(iCh, 0xA4, ctrl);

      ret |= ReadSPIRegister(iCh, 0xA4, ctrl);
      // return ret;
    }
  } else if (fDigitizerModel == 751) {
    auto err = CAEN_DGTZ_Calibrate(fHandler);
    PrintError(err, "Calibrate");
  }
}

void TWaveRecord::Open(CAEN_DGTZ_ConnectionType type, int link, int node,
                       uint32_t VMEadd)
{
  auto err = CAEN_DGTZ_OpenDigitizer(type, link, node, VMEadd, &fHandler);
  PrintError(err, "OpenDigitizer");

  if (err != CAEN_DGTZ_Success) {
    std::cout << "Can not open the device!" << std::endl;
    exit(0);
  }
}

void TWaveRecord::Close()
{
  auto err = CAEN_DGTZ_CloseDigitizer(fHandler);
  PrintError(err, "CloseDigitizer");
}

CAEN_DGTZ_ErrorCode TWaveRecord::WriteSPIRegister(uint32_t ch, uint32_t address,
                                                  uint32_t value)
{
  uint32_t SPIBusy = 1;
  int32_t ret = CAEN_DGTZ_Success;

  uint32_t SPIBusyAddr = 0x1088 + (ch << 8);
  uint32_t addressingRegAddr = 0x80B4;
  uint32_t valueRegAddr = 0x10B8 + (ch << 8);

  while (SPIBusy) {
    if ((ret = CAEN_DGTZ_ReadRegister(fHandler, SPIBusyAddr, &SPIBusy)) !=
        CAEN_DGTZ_Success)
      return CAEN_DGTZ_CommError;
    SPIBusy = (SPIBusy >> 2) & 0x1;
    if (!SPIBusy) {
      if ((ret = CAEN_DGTZ_WriteRegister(fHandler, addressingRegAddr,
                                         address)) != CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
      if ((ret = CAEN_DGTZ_WriteRegister(fHandler, valueRegAddr, value)) !=
          CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
    }
    usleep(1000);
  }
  return CAEN_DGTZ_Success;
}

CAEN_DGTZ_ErrorCode TWaveRecord::ReadSPIRegister(uint32_t ch, uint32_t address,
                                                 uint32_t &value)
{  // Copy from digiTES
  uint32_t SPIBusy = 1;
  int32_t ret = CAEN_DGTZ_Success;
  uint32_t SPIBusyAddr = 0x1088 + (ch << 8);
  uint32_t addressingRegAddr = 0x80B4;
  uint32_t valueRegAddr = 0x10B8 + (ch << 8);

  while (SPIBusy) {
    if ((ret = CAEN_DGTZ_ReadRegister(fHandler, SPIBusyAddr, &SPIBusy)) !=
        CAEN_DGTZ_Success)
      return CAEN_DGTZ_CommError;
    SPIBusy = (SPIBusy >> 2) & 0x1;
    if (!SPIBusy) {
      if ((ret = CAEN_DGTZ_WriteRegister(fHandler, addressingRegAddr,
                                         address)) != CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
      if ((ret = CAEN_DGTZ_ReadRegister(fHandler, valueRegAddr, &value)) !=
          CAEN_DGTZ_Success)
        return CAEN_DGTZ_CommError;
    }
    usleep(1000);
  }
  return CAEN_DGTZ_Success;
}

void TWaveRecord::Reset()
{
  auto err = CAEN_DGTZ_Reset(fHandler);
  PrintError(err, "Reset");
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

void TWaveRecord::GetBaseLine()
{
  BoardCalibration();

  AcquisitionConfig();

  CAEN_DGTZ_ErrorCode err;

  err = CAEN_DGTZ_SetMaxNumEventsBLT(fHandler, fBLTEvents);
  PrintError(err, "SetMaxNEventsBLT");
  err = CAEN_DGTZ_MallocReadoutBuffer(fHandler, &fpReadoutBuffer,
                                      &fMaxBufferSize);
  PrintError(err, "MallocReadoutBuffer");

  TH1D *his = new TH1D("his", "test", 20000, 0.5, 20000.5);

  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  for (int i = 0; i < 1000; i++) CAEN_DGTZ_SendSWtrigger(fHandler);

  err = CAEN_DGTZ_ReadData(fHandler, CAEN_DGTZ_SLAVE_TERMINATED_READOUT_MBLT,
                           fpReadoutBuffer, &fBufferSize);
  PrintError(err, "ReadData");

  err =
      CAEN_DGTZ_GetNumEvents(fHandler, fpReadoutBuffer, fBufferSize, &fNEvents);
  PrintError(err, "GetNumEvents");

  for (int32_t iEve = 0; iEve < fNEvents; iEve++) {
    err = CAEN_DGTZ_GetEventInfo(fHandler, fpReadoutBuffer, fBufferSize, iEve,
                                 &fEventInfo, &fpEventPtr);
    PrintError(err, "GetEventInfo");

    err = CAEN_DGTZ_DecodeEvent(fHandler, fpEventPtr, (void **)&fpEventStd);
    PrintError(err, "DecodeEvent");

    const UInt_t chSize = fpEventStd->ChSize[0];
    for (UInt_t iSample = 0; iSample < chSize; iSample++)
      his->Fill(fpEventStd->DataChannel[0][iSample]);
  }

  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  // fBaseLine = his->GetMean();
  fBaseLine = his->GetMaximumBin();
  std::cout << "Base line:\t" << fBaseLine << std::endl;

  delete his;
}

void TWaveRecord::PrintError(const CAEN_DGTZ_ErrorCode &err,
                             const std::string &funcName)
{
  if (err < 0) {  // 0 is success
    std::cout << "In " << funcName << ", error code = " << err << std::endl;
    // CAEN_DGTZ_CloseDigitizer(fHandler);
    // throw err;
  }
}

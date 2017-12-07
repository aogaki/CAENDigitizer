#include <iostream>

#include "TTestDigi.hpp"

TTestDigi::TTestDigi() : TDigitizerCommand() {}

TTestDigi::TTestDigi(CAEN_DGTZ_ConnectionType type, int link, int node,
                     uint32_t VMEadd)
    : TDigitizerCommand(type, link, node, VMEadd)
{
}

TTestDigi::~TTestDigi() {}

void TTestDigi::Initialize()
{
  SetRecordLength(4096);

  TriggerAcqConfig();

  SetMaxNEventsBLT(1023);
  std::cout << GetMaxNEventsBLT() << std::endl;
  MallocReadoutBuffer();
  // AllocateEvent();

  BoardCalibration();
}

void TTestDigi::TriggerAcqConfig()
{
  // Eanble all channels
  uint32_t mask = ((1 << fNChs) - 1);
  SetChannelEnableMask(mask);

  // Set the trigger threshold
  // The unit of its are V
  const double_t Vpp = 2.;  // Fuck the hardcoding!
  const double_t Vth = 0.001;
  uint32_t thVal = ((1 << fNBits) / 2) * (1. - (Vth / (Vpp / 2)));
  std::cout << "Vth: ";
  for (uint32_t i = 0; i < fNChs; i++) {
    SetChannelTriggerThreshold(i, thVal);
    std::cout << GetChannelTriggerThreshold(i) << "\t";
  }
  std::cout << std::endl;

  // Set the triggermode
  SetChannelSelfTrigger(CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT, mask);
  SetSWTriggerMode(CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  std::cout << "Channel trigger mode: ";
  for (uint32_t i = 0; i < fNChs; i++) {
    std::cout << GetChannelSelfTrigger(i) << "\t";
  }
  std::cout << std::endl;

  // Set the acquisition mode
  SetAcquisitionMode(CAEN_DGTZ_SW_CONTROLLED);
}

void TTestDigi::ReadEvents()
{
  ReadData();
  GetNumEvents();
  for (int32_t i = 0; i < fNEvents; i++) {
    GetEventInfo(i);
    DecodeEvent();
    fConverter->SetData(fpEventStd);
    FreeEvent();
  }
  fConverter->DrawGraph();
  fConverter->DrawHis();
}

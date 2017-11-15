#include <iostream>

#include "TDPP.hpp"

int main()
{
  int link = 0;
  auto digi = new TDPP(CAEN_DGTZ_USB, link);
  digi->GetInfo();

  for (int i = 0; i < 8; i++)
    std::cout << digi->ReadTemperature(i) << std::endl;

  digi->SetMaxNEventsBLT(1024);
  digi->MallocReadoutBuffer();
  digi->AllocateEvent();
  digi->SWStartAcquisition();
  digi->SetSWTriggerMode(
      CAEN_DGTZ_TriggerMode_t::CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  std::cout << digi->GetSWTriggerMode() << std::endl;

  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 100; j++) digi->SendSWTrigger();

    digi->ReadEvents();
    // sleep(0.5);
  }
  digi->SWStopAcquisition();

  delete digi;
}

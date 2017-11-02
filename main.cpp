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
  digi->SWStartAcquisition();
  for (int i = 0; i < 100; i++) {
    digi->SendSWTrigger();
    digi->ReadData();
  }
  digi->SWStopAcquisition();

  delete digi;
}

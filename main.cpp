#include <iostream>

#include "TUSBDigitizer.hpp"

int main()
{
  int link;
  auto digi = new TUSBDigitizer(link = 0);
  digi->GetInfo();

  for (int i = 0; i < 8; i++)
    std::cout << digi->ReadTemperature(i) << std::endl;

  digi->SetMaxNEventsBLT(1024);
  digi->MallocReadoutBuffer();
  digi->SWStartAcquisition();
  for (int i = 0; i < 1000; i++) {
    digi->SendSWTrigger();
    digi->ReadData();
  }
  digi->SWStopAcquisition();

  delete digi;
}

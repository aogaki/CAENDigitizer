#include <iostream>

#include "TUSBDigitizer.hpp"

int main()
{
  int link;
  auto digi = new TUSBDigitizer(link = 0);
  digi->GetInfo();

  digi->SetMaxNEventsBLT(1024);
  digi->MallocReadoutBuffer();
  for (int i = 0; i < 1000; i++) digi->ReadData();
  delete digi;
}

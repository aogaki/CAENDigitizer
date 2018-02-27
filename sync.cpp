#include <fcntl.h>
#include <iostream>
#include <termios.h>

#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>

#include "TPSD.hpp"
#include "TWaveRecord.hpp"

int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  TApplication app("testApp", &argc, argv);

  int link = 0;
  auto master = new TPSD(CAEN_DGTZ_OpticalLink, link, 1);
  master->Initialize();
  master->StartAcquisition();

  auto slave = new TPSD(CAEN_DGTZ_OpticalLink, link, 0);
  slave->Initialize();
  slave->StartAcquisition();

  for (int i = 0; true; i++) {
    //   // if (i > 10) break;
    std::cout << " loop" << i << std::endl;

    master->ReadEvents();
    slave->ReadEvents();
    std::cout << master->GetNEvents() << "\t" << slave->GetNEvents()
              << std::endl;

    if (master->GetNEvents() == slave->GetNEvents()) {
      auto masterArray = master->GetDataArray();
      auto slaveArray = slave->GetDataArray();
      const int nHit = master->GetNEvents();

      for (int i = 0; i < nHit; i++) {
        auto index = (i * ONE_HIT_SIZE);
        auto offset = 0;

        SampleData masterData;
        SampleData slaveData;

        masterData.ModNumber = masterArray[index + offset];
        slaveData.ModNumber = slaveArray[index + offset];
        offset += sizeof(masterData.ModNumber);

        masterData.ChNumber = masterArray[index + offset];
        slaveData.ChNumber = slaveArray[index + offset];
        offset += sizeof(masterData.ChNumber);

        memcpy(&masterData.TimeStamp, &masterArray[index + offset],
               sizeof(masterData.TimeStamp));
        memcpy(&slaveData.TimeStamp, &slaveArray[index + offset],
               sizeof(slaveData.TimeStamp));
        offset += sizeof(masterData.TimeStamp);

        memcpy(&masterData.ADC, &masterArray[index + offset],
               sizeof(masterData.ADC));
        memcpy(&slaveData.ADC, &slaveArray[index + offset],
               sizeof(slaveData.ADC));
        offset += sizeof(masterData.ADC);

        std::cout << masterData.TimeStamp << "\t" << slaveData.TimeStamp << "\t"
                  << masterData.TimeStamp - slaveData.TimeStamp << std::endl;
      }
    }

    if (kbhit())
      break;

    usleep(50000);
  }

  master->StopAcquisition();
  slave->StopAcquisition();

  // app.Run();
  delete master;
  delete slave;

  return 0;
}

#include <fcntl.h>
#include <termios.h>
#include <iostream>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1.h>

#include "TDPP.hpp"

int kbhit(void)
{
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

int main(int argc, char **argv)
{
  TApplication app("testApp", &argc, argv);

  int link = 0;
  auto digi = new TDPP(CAEN_DGTZ_USB, link);
  digi->Initialize();

  auto err = digi->StartAcquisition();

  if (err == CAEN_DGTZ_Success) {
    for (int counter = 0; true; counter++) {
      std::cout << counter << std::endl;
      for (int i = 0; i < 10; i++) {
        digi->SendSWTrigger();
        usleep(100000);
      }
      digi->ReadEvents();
      if (kbhit()) break;
      usleep(100000);
    }
    digi->StopAcquisition();
  } else {
    std::cout << "Probably, FW is expired." << std::endl;
  }

  delete digi;

  // app.Run();

  return 0;
}

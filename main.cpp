#include <fcntl.h>
#include <termios.h>
#include <iostream>

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1.h>

#include "TDigitizer.hpp"
#include "TTestDigi.hpp"

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
  auto digi = new TDigitizer(CAEN_DGTZ_USB, link);

  digi->GetBaseLine();
  digi->Initialize();

  digi->StartAcquisition();

  TH1D *hisCharge = new TH1D("hisCharge", "test", 100000, 0, 100000);
  TCanvas *canvas = new TCanvas();
  hisCharge->Draw();

  for (int i = 0; true; i++) {
    // if (i > 10) break;
    std::cout << i << std::endl;

    digi->ReadEvents();

    auto charge = digi->GetCharge();
    for (auto &q : *charge) hisCharge->Fill(q);

    canvas->cd();
    hisCharge->Draw();
    canvas->Update();

    if (kbhit()) break;

    usleep(10000);
  }

  digi->StopAcquisition();

  delete digi;

  // app.Run();

  return 0;
}

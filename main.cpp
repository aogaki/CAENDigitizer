#include <fcntl.h>
#include <termios.h>
#include <iostream>

#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>

#include "TWaveRecord.hpp"

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
  auto digi = new TWaveRecord(CAEN_DGTZ_USB, link);

  digi->Initialize();

  digi->StartAcquisition();

  TH1D *hisCharge = new TH1D("hisCharge", "test", 20000, 0, 20000);
  TCanvas *canvas = new TCanvas();
  TGraph *grWave = new TGraph();
  grWave->SetMaximum(9000);
  grWave->SetMinimum(7000);
  TCanvas *canvas2 = new TCanvas();
  canvas->cd();
  hisCharge->Draw();

  for (int i = 0; true; i++) {
    // if (i > 10) break;
    std::cout << i << std::endl;

    for (int j = 0; j < 10; j++) digi->SendSWTrigger();
    digi->ReadEvents();

    // auto charge = digi->GetCharge();
    // for (auto &q : *charge) hisCharge->Fill(q);
    auto data = digi->GetData();
    std::cout << data->size() << std::endl;
    for (auto &q : *data) {
      if (q.ChNumber == 0) {
        hisCharge->Fill(q.ADC);
        for (int i = 0; i < kNSamples; i++)
          grWave->SetPoint(i, i, q.Waveform[i]);
      }
    }
    canvas2->cd();
    grWave->Draw("AL");
    canvas2->Update();

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

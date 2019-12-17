#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>
#include <fcntl.h>
#include <termios.h>

#include <iostream>

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

  TWaveRecordPar par;
  par.SetRecordLength(512);
  par.SetBLTEvents(512);
  par.SetVpp(2.0);
  par.SetVth(-0.1);
  par.SetDCOffset(0.8);
  par.SetPolarity(CAEN_DGTZ_TriggerOnFallingEdge);
  par.SetTriggerMode(CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
  par.SetPostTriggerSize(80);
  digi->SetParameter(par);

  digi->Initialize();

  digi->StartAcquisition();

  TH1D *hisCharge = new TH1D("hisCharge", "test", 20000, 0, 200000);
  TCanvas *canvas = new TCanvas();
  TGraph *grWave = new TGraph();
  grWave->SetMaximum(20000);
  grWave->SetMinimum(0);
  TCanvas *canvas2 = new TCanvas();
  canvas->cd();
  hisCharge->Draw();

  for (int i = 0; true; i++) {
    //   // if (i > 10) break;
    std::cout << i << std::endl;

    for (int j = 0; j < 10; j++) digi->SendSWTrigger();
    digi->ReadEvents();
    auto data = digi->GetData();
    // std::cout << data->at(0).TimeStamp << std::endl;
    // std::cout << data->size() << std::endl;

    const auto kHit = data->size();
    for (auto iHit = 0; iHit < kHit; iHit++) {
      auto size = data->at(iHit).RecordLength;
      auto signal = data->at(iHit).WaveForm;

      for (auto i = 0; i < size; i++) {
        grWave->SetPoint(i, i * 2, signal[i]);
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

  // app.Run();
  delete digi;

  return 0;
}

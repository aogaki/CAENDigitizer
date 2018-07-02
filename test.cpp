#include <fcntl.h>
#include <termios.h>
#include <iostream>

#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>

#include "MyFunctions.hpp"
#include "TDigitizer.hpp"

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
  // auto digi = new TDigitizer();
  int link = 0;
  auto digi = new TDigitizer("Ge0", CAEN_DGTZ_USB, link);
  digi->ConfigDevice();
  digi->StartAcquisition();

  TH1D *hisCharge = new TH1D("hisCharge", "test", 20000, 0, 20000);
  TCanvas *canvas = new TCanvas();
  TGraph *grWave = new TGraph();
  grWave->SetMaximum(18000);
  grWave->SetMinimum(0);
  TCanvas *canvas2 = new TCanvas();
  canvas->cd();
  hisCharge->Draw();
  std::cout << hisCharge->GetEntries() << std::endl;
  while (true) {
    // for (auto i = 0; i < 1000; i++) digi->SendSWTrigger();
    digi->ReadEvent();
    const int nHit = digi->GetNEvent();
    auto dataArray = digi->GetDataArray();
    std::cout << nHit << " hits" << std::endl;

    for (int i = 0; i < nHit; i++) {
      auto index = (i * ONE_HIT_SIZE);
      auto offset = 0;
      SampleData data;

      data.ModNumber = dataArray[index + offset];
      offset += sizeof(data.ModNumber);

      data.ChNumber = dataArray[index + offset];
      offset += sizeof(data.ChNumber);

      memcpy(&data.TimeStamp, &dataArray[index + offset],
             sizeof(data.TimeStamp));
      offset += sizeof(data.TimeStamp);

      memcpy(&data.ADC, &dataArray[index + offset], sizeof(data.ADC));
      offset += sizeof(data.ADC);
      if (data.ChNumber == 0) {
        hisCharge->Fill(data.ADC);

        for (int iSample = 0; iSample < kNSamples; iSample++) {
          unsigned short pulse;
          memcpy(&pulse, &dataArray[index + offset], sizeof(pulse));
          offset += sizeof(pulse);

          grWave->SetPoint(iSample, iSample * 2, pulse);  // one sample 2 ns
        }
      }
    }
    canvas2->cd();
    grWave->Draw("AL");
    canvas2->Update();

    canvas->cd();
    hisCharge->Draw();
    canvas->Update();
    usleep(10000);
    if (kbhit()) break;
  }

  digi->StopAcquisition();

  delete digi;
  return 0;
}

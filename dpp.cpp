#include <fcntl.h>
#include <termios.h>
#include <iostream>

#include <TApplication.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>

#include "TDPP.hpp"
#include "TStdData.hpp"

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

  TH1D *hisCharge = new TH1D("hisCharge", "test", 120000, -60000, 60000);
  TCanvas *canHis = new TCanvas("canHis", "his");
  TGraph *grWave = new TGraph();
  grWave->SetMaximum(9000);
  grWave->SetMinimum(7000);
  TCanvas *canGr = new TCanvas("canGr", "graph");
  canHis->cd();
  hisCharge->Draw();

  unsigned char *dataArray{nullptr};

  if (err == CAEN_DGTZ_Success) {
    for (int counter = 0; true; counter++) {
      std::cout << counter << std::endl;
      for (int i = 0; i < 10; i++) {
        // digi->SendSWTrigger();
        usleep(100000);
      }

      digi->ReadEvents();
      dataArray = digi->GetDataArray();
      const int nHit = digi->GetNEvents();
      std::cout << nHit << std::endl;
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

        if (data.ChNumber == 5) {
          hisCharge->Fill(data.ADC);

          for (int iSample = 0; iSample < kNSamples; iSample++) {
            unsigned short pulse;
            memcpy(&pulse, &dataArray[index + offset], sizeof(pulse));
            offset += sizeof(pulse);

            grWave->SetPoint(iSample, iSample * 2, pulse);  // one sample 2 ns
          }
        }
      }

      canGr->cd();
      grWave->Draw("AL");
      canGr->Update();

      canHis->cd();
      hisCharge->Draw();
      canHis->Update();

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

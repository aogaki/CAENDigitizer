#include <iostream>

#include "TEveConverter.hpp"

TEveConverter::TEveConverter()
    : fGrCanvas(nullptr), fHisCanvas(nullptr), fGraph(nullptr), fHis(nullptr)
{
  Init();
}
TEveConverter::TEveConverter(Int_t nChs)
    : fNChs(nChs),
      fGrCanvas(nullptr),
      fHisCanvas(nullptr),
      fGraph(nullptr),
      fHis(nullptr)
{
  Init();
}

TEveConverter::~TEveConverter()
{
  delete fHis;
  delete fGraph;
  delete fGrCanvas;
  delete fHisCanvas;
}

void TEveConverter::Init()
{
  fGrCanvas = new TCanvas("GrCanvas", "test");
  fHisCanvas = new TCanvas("HisCanvas", "test");
  // fGraph = new TGraph("MyGraph", "test:[ns]:[V]");
  fGraph = new TGraph();
  fGraph->SetMinimum(6000);
  fGraph->SetMaximum(10000);

  fHis = new TH1D("His", "test", 100000, -50000, 50000);
}

void TEveConverter::SetData(CAEN_DGTZ_UINT16_EVENT_t *eve)
{
  const UInt_t grSize = fGraph->GetN();
  const UInt_t chSize = eve->ChSize[0];
  Double_t sumCharge = 0.;
  for (UInt_t i = 0; i < chSize; i++) {
    fGraph->SetPoint(i, i, (eve->DataChannel[0])[i]);
    sumCharge += eve->DataChannel[0][i];
  }
  // Stupid!
  Double_t baseLine = 0.;
  const Int_t nSamples = 20;
  for (Int_t i = 0; i < nSamples; i++) baseLine += fGraph->GetY()[i];
  baseLine /= nSamples;

  fCharge = sumCharge - (baseLine * chSize);
  fHis->Fill(fCharge);

  for (UInt_t i = chSize; i < grSize; i++) {
    fGraph->SetPoint(i, i, baseLine);
  }
}

void TEveConverter::DrawGraph()
{
  if (fGraph->GetN() > 0) {
    fGrCanvas->cd();
    fGraph->Draw("AL");
    fGrCanvas->Update();
  }
}

void TEveConverter::DrawHis()
{
  fHisCanvas->cd();
  fHis->Draw();
  fHisCanvas->Update();
}

#ifndef TEveConverter_hpp
#define TEveConverter_hpp 1

#include <CAENDigitizerType.h>

#include <TCanvas.h>
#include <TGraph.h>
#include <TH1.h>

class TEveConverter
{
 public:
  TEveConverter();
  explicit TEveConverter(Int_t nChs);
  ~TEveConverter();

  void SetData(CAEN_DGTZ_UINT16_EVENT_t *eve);
  Double_t GetCharge() { return fCharge; };
  void DrawGraph();
  void DrawHis();

 private:
  Int_t fNChs;
  Double_t fCharge;

  TCanvas *fGrCanvas;
  TCanvas *fHisCanvas;
  TGraph *fGraph;
  TH1D *fHis;

  void Init();
};

#endif

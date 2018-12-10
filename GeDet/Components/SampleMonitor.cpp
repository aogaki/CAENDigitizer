// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */
#include <arpa/inet.h>

#include <TStyle.h>
#include <TSystem.h>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;

#include "SampleMonitor.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *samplemonitor_spec[] = {"implementation_id",
                                           "SampleMonitor",
                                           "type_name",
                                           "SampleMonitor",
                                           "description",
                                           "SampleMonitor component",
                                           "version",
                                           "1.0",
                                           "vendor",
                                           "Kazuo Nakayoshi, KEK",
                                           "category",
                                           "example",
                                           "activity_type",
                                           "DataFlowComponent",
                                           "max_instance",
                                           "1",
                                           "language",
                                           "C++",
                                           "lang_type",
                                           "compile",
                                           ""};

Double_t fitFnc(Double_t *pos, Double_t *par)
{  // This should be class not function.

  auto x = pos[0];
  auto mean = par[1];
  auto sigma = par[2];

  auto facRange = 3;
  auto limitHigh = mean + facRange * sigma;
  auto limitLow = mean - facRange * sigma;

  auto val = par[0] * TMath::Gaus(x, mean, sigma);

  auto backGround = 0.;
  if (x < limitLow)
    backGround = par[3] + par[4] * x;
  else if (x > limitHigh)
    backGround = par[5] + par[6] * x;
  else {
    auto xInc = limitHigh - limitLow;
    auto yInc = (par[5] + par[6] * limitHigh) - (par[3] + par[4] * limitLow);
    auto slope = yInc / xInc;

    backGround = (par[3] + par[4] * limitLow) + slope * (x - limitLow);
  }

  if (backGround < 0.) backGround = 0.;
  val += backGround;

  return val;
}

std::vector<Double_t> FindPeaks(TH1D *his)
{
  // Find peaks
  // Searching is till 700 keV from the first peak

  std::vector<Double_t> retVec{-1.};

  const auto nBins = his->GetNbinsX();
  const auto maxContent = his->GetMaximum();
  auto upCount = 0;
  auto downCount = 0;
  const UInt_t nSamples = 10;
  auto lastVal = 0.;
  auto lastSlope = 0.;

  for (auto iBin = nBins - nSamples; iBin - nSamples > 0; iBin -= nSamples) {
    auto val = 0.;
    for (auto i = iBin - nSamples / 2; i < iBin + nSamples; i++)
      val += his->GetBinContent(i) / nSamples;

    auto slope = (val - lastVal) / nSamples;
    lastVal = val;

    // Fuck the dirty code!!!!!!
    const auto th = 5;
    if (upCount < th && slope < 0. && lastSlope < 0.) upCount = 0;

    if (lastSlope * slope > 0.) {
      if (upCount > 0 && slope < 0.)
        downCount++;
      else if (slope > 0.)
        upCount++;
    }
    lastSlope = slope;

    if (upCount >= th && downCount >= th) {
      upCount = downCount = 0;
      auto peakPos = iBin + th * nSamples;
      if (his->GetBinContent(peakPos) > maxContent / 20.) {
        if (retVec[0] == -1)
          retVec[0] = peakPos;
        else
          retVec.push_back(peakPos);
      }
    }

    if (retVec.size() >= 2) break;
  }

  if (retVec[0] == -1) retVec[0] = his->GetMaximumBin();

  return retVec;
}

SampleMonitor::SampleMonitor(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("samplemonitor_in", m_in_data),
      m_in_status(BUF_SUCCESS),
      m_debug(true),
      m_bin(0),
      m_min(0),
      m_max(0),
      m_monitor_update_rate(100),
      m_event_byte_size(0),
      fServ(nullptr)
{
  // Registration: InPort/OutPort/Service
  std::cout << "Number of Samples" << kNSample << std::endl;
  // Set InPort buffers
  registerInPort("samplemonitor_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLEMONITOR");

  for (uint i = 0; i < kNCh; i++) {
    fHis[i] =
        new TH1D(Form("His%02d", i), "Energy distribution", 20000, 0., 20000.);

    fHisInput[i] =
        new TH2D(Form("HisInput%02d", i), "Integral of input signal",
                 kNSample / 10, 0.5, kNSample + 0.5, 1800, 0.5, 18000.5);

    fGr[i] = new TGraph();
    for (auto j = 0; j < kNSample; j++) fGr[i]->SetPoint(j, j + 1, 8000);
    fGr[i]->SetTitle(Form("Input signal: ch %d", i));
    fGr[i]->SetMinimum(0);
    fGr[i]->SetMaximum(18000);
    fCanvas[i] = new TCanvas(Form("signal%02d", i), "test");
    fGr[i]->Draw("AL");

    // fFitFnc[i] = new TF1(Form("FitFnc%02d", i), "gaus");
    // fFitFnc[i] = new TF1(Form("FitFnc%02d", i), fitFnc);
    fFitFnc[i] = nullptr;
  }

  fServ = new THttpServer("http:8080?monitoring=5000;rw;noglobal");
  // fServ->SetDefaultPage("index.html");
  for (uint i = 0; i < kNCh; i++) {
    fServ->Register("/Energy", fHis[i]);
    fServ->Register("/Input", fCanvas[i]);
    fServ->Register("/Test", fHisInput[i]);
  }

  fFirstFitFlag = true;
  fFitCan = new TCanvas();
  fServ->Register("/Fit", fFitCan);

  gStyle->SetOptFit(1111);
}

SampleMonitor::~SampleMonitor()
{
  for (auto i = 0; i < kNCh; i++) {
    fServ->Unregister(fHis[i]);
    fServ->Unregister(fCanvas[i]);
    fServ->Unregister(fHisInput[i]);
  }
  for (auto i = 0; i < kNCh; i++) {
    DelPointer(fFitFnc[i]);
    DelPointer(fHis[i]);
    DelPointer(fHisInput[i]);
    DelPointer(fGr[i]);
    DelPointer(fCanvas[i]);
  }
  DelPointer(fServ);  // Need unregister hists?
}

RTC::ReturnCode_t SampleMonitor::onInitialize()
{
  if (m_debug) {
    std::cerr << "SampleMonitor::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t SampleMonitor::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int SampleMonitor::daq_dummy()
{
  gSystem->ProcessEvents();

  return 0;
}

int SampleMonitor::daq_configure()
{
  std::cerr << "*** SampleMonitor::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int SampleMonitor::parse_params(::NVList *list)
{
  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;
  }

  return 0;
}

int SampleMonitor::daq_unconfigure()
{
  std::cerr << "*** SampleMonitor::unconfigure" << std::endl;

  for (auto i = 0; i < kNCh; i++) {
    fHis[i]->Reset();
    fHisInput[i]->Reset();
  }

  fFirstFitFlag = true;

  return 0;
}

int SampleMonitor::daq_start()
{
  std::cerr << "*** SampleMonitor::start" << std::endl;

  m_in_status = BUF_SUCCESS;

  return 0;
}

int SampleMonitor::daq_stop()
{
  std::cerr << "*** SampleMonitor::stop" << std::endl;

  // Check! which really needed
  gSystem->ProcessEvents();
  FitHis();
  gSystem->ProcessEvents();

  UploadResults();

  reset_InPort();

  return 0;
}

int SampleMonitor::daq_pause()
{
  std::cerr << "*** SampleMonitor::pause" << std::endl;

  return 0;
}

int SampleMonitor::daq_resume()
{
  std::cerr << "*** SampleMonitor::resume" << std::endl;

  return 0;
}

int SampleMonitor::reset_InPort()
{
  int ret = true;
  while (ret == true) {
    ret = m_InPort.read();
  }

  return 0;
}

unsigned int SampleMonitor::read_InPort()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status = check_inPort_status(m_InPort);
    if (m_in_status == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {        // Check if stop command has come.
        set_trans_unlock();            // Transit to CONFIGURE state.
      }
    } else if (m_in_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(INPORT_ERROR);
    }
  } else {
    recv_byte_size = m_in_data.data.length();
  }

  if (m_debug) {
    std::cerr << "m_in_data.data.length():" << recv_byte_size << std::endl;
  }

  return recv_byte_size;
}

int SampleMonitor::daq_run()
{
  if (m_debug) {
    std::cerr << "*** SampleMonitor::run" << std::endl;
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) {  // Timeout
    std::cout << "0 size" << std::endl;
    gSystem->ProcessEvents();
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size);  // check header and footer
  // unsigned int event_byte_size = get_event_size(recv_byte_size);
  m_event_byte_size = get_event_size(recv_byte_size);

  /////////////  Write component main logic here. /////////////
  // online_analyze();
  /////////////////////////////////////////////////////////////

  memcpy(&m_recv_data[0], &m_in_data.data[HEADER_BYTE_SIZE], m_event_byte_size);

  fill_data(&m_recv_data[0], m_event_byte_size);

  // if (m_monitor_update_rate == 0) m_monitor_update_rate = 300;
  // m_monitor_update_rate = 300;

  unsigned long sequence_num = get_sequence_num();
  if ((sequence_num % m_monitor_update_rate) == 0) {
    FitHis();
    gSystem->ProcessEvents();
  }

  inc_sequence_num();                      // increase sequence num.
  inc_total_data_size(m_event_byte_size);  // increase total data byte size

  return 0;
}

int SampleMonitor::fill_data(const unsigned char *mydata, const int size)
{
  for (int i = 0; i < size / int(ONE_HIT_SIZE); i++) {
    decode_data(mydata);
    auto ch = uint(m_sampleData.ChNumber);
    if (ch >= 0 && ch < kNCh) {
      fHis[ch]->Fill(m_sampleData.ADC);
      for (int iSample = 0; iSample < kNSample; iSample++) {
        fGr[ch]->SetPoint(iSample, iSample, m_sampleData.Waveform[iSample]);
        fHisInput[ch]->Fill(iSample, m_sampleData.Waveform[iSample]);
      }
    }
    mydata += ONE_HIT_SIZE;
  }

  return 0;
}

int SampleMonitor::decode_data(const unsigned char *mydata)
{
  unsigned int index = 0;
  m_sampleData.ModNumber = mydata[index++];
  m_sampleData.ChNumber = mydata[index++];

  unsigned long timeStamp = *(unsigned long *)&mydata[index];
  m_sampleData.TimeStamp = timeStamp;
  index += sizeof(timeStamp);

  unsigned short adc = *(unsigned short *)&mydata[index];
  m_sampleData.ADC = adc;
  index += sizeof(adc);

  for (int i = 0; i < kNSample; i++) {
    unsigned short pulse = *(unsigned short *)&mydata[index];
    m_sampleData.Waveform[i] = pulse;
    index += sizeof(pulse);
  }
}

void SampleMonitor::FitHis()
{
  for (auto iCh = 0; iCh < kNCh; iCh++) {
    if (fHis[iCh]->GetEntries() == 0) continue;
    fFitCan->cd();
    auto his = fHis[iCh];
    his->Draw();
    // auto myFit = fFitFnc[iCh][iCh];

    // if (fFirstFitFlag)
    //   FirstFit(his, f);
    // else {
    //   // RangeFit(his, f);
    //   // RangeFit(his, f);
    //   FirstFit(his, f);
    // }

    auto v = FindPeaks(his);
    auto mean = v[0];
    auto sigma = sqrt(mean);

    auto f = new TF1("f", "gaus");
    f->SetRange(mean - sigma, mean + sigma);
    his->Fit(f, "R");

    mean = f->GetParameter(1);
    sigma = f->GetParameter(2);
    const auto facFWHM = 2 * sqrt(2 * log(2));
    auto FWHM = sigma * facFWHM;
    f->SetRange(mean - FWHM / 2, mean + FWHM / 2);
    his->Fit(f, "R");

    mean = f->GetParameter(1);
    sigma = f->GetParameter(2);
    auto rangeFac = 5.;
    auto left = his->GetBinContent(his->FindBin(mean - rangeFac * sigma));
    auto right = his->GetBinContent(his->FindBin(mean + rangeFac * sigma));

    if (fFitFnc[iCh] == nullptr)
      fFitFnc[iCh] =
          new TF1(Form("FitFnc%d", iCh), fitFnc, mean - rangeFac * sigma,
                  mean + rangeFac * sigma, 7);
    else
      fFitFnc[iCh]->SetRange(mean - rangeFac * sigma, mean + rangeFac * sigma);

    fFitFnc[iCh]->SetParameters(f->GetParameter(0), mean, sigma, left, -0.001,
                                right, 0.);
    fFitFnc[iCh]->FixParameter(6, 0.);
    fFitFnc[iCh]->SetParLimits(1, mean - sigma, mean + sigma);
    his->Fit(fFitFnc[iCh], "R");

    fFitCan->Modified();
    fFitCan->Update();
  }
}

void SampleMonitor::FirstFit(TH1D *his, TF1 *f)
{
  // Range setter is needed
  // Now stupid hard coding
  // his->GetXaxis()->SetRange(1, 10000);
  auto mean = his->GetMaximumBin();
  auto sigma = his->GetStdDev();
  // his->GetXaxis()->SetRange(0, 0);

  sigma = 10;  // Stupid!
  constexpr auto facFWHM = 2 * sqrt(2 * log(2));
  auto FWHM = sigma * facFWHM;
  f->SetRange(mean - FWHM / 2, mean + FWHM / 2);

  his->Fit(f, "R");

  RangeFit(his, f);
  RangeFit(his, f);

  fFirstFitFlag = false;
}

void SampleMonitor::RangeFit(TH1D *his, TF1 *f)
{
  auto mean = f->GetParameter(1);
  auto sigma = f->GetParameter(2);
  constexpr auto facFWHM = 2 * sqrt(2 * log(2));
  auto FWHM = sigma * facFWHM;
  // f->SetRange(mean - FWHM / 2, mean + FWHM / 2);
  f->SetRange(mean - 5 * sigma, mean + 5 * sigma);
  fFitCan->cd();
  his->Fit(f, "R");
  fFitCan->Modified();
  fFitCan->Update();
}

void SampleMonitor::UploadResults()
{
  auto mean = 0.;
  auto sigma = 0.;
  if (fFitFnc[0]) {
    mean = fFitFnc[0]->GetParameter(1);
    sigma = fFitFnc[0]->GetParameter(2);
  }
  auto FWHM = sigma * 2 * sqrt(2 * log(2));
  std::cout << "Mean: " << mean << "\n"
            << "FWHM: " << FWHM << std::endl;

  // Do image save
  // I expect noone do at same time
  // But such the expection is done by only the fool
  auto fileName = TString(Form("fit-%ld.jpg", time(nullptr)));
  auto fullPath = "/home/aogaki/DAQ/Outputs/images/" + fileName;
  if (fFitCan) fFitCan->Print(fullPath, "jpg");

  // Connect to Mongo DB
  // mongocxx::instance *inst{};
  mongocxx::client conn{mongocxx::uri{"mongodb://192.168.161.73/"}};
  // mongocxx::client conn{mongocxx::uri{}};

  auto collection = conn["node-angular"]["posts"];

  bsoncxx::builder::stream::document buf{};
  buf << "title" << mean << "content" << FWHM << "imagePath"
      << Form("http://192.168.161.73:3000/images/%s", fileName.Data());
  collection.insert_one(buf.view());
  buf.clear();

  auto cursor = collection.find({});
  for (auto &&doc : cursor) {
    std::cout << bsoncxx::to_json(doc) << std::endl;
  }
}

extern "C" {
void SampleMonitorInit(RTC::Manager *manager)
{
  RTC::Properties profile(samplemonitor_spec);
  manager->registerFactory(profile, RTC::Create<SampleMonitor>,
                           RTC::Delete<SampleMonitor>);
}
};

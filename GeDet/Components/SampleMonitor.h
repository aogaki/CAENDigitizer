// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef SAMPLEMONITOR_H
#define SAMPLEMONITOR_H

#include <TCanvas.h>
#include <TF1.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH2.h>
#include <THttpServer.h>
#include <TStyle.h>

#include "DaqComponentBase.h"
#include "SampleData.h"

using namespace RTC;

class SampleMonitor : public DAQMW::DaqComponentBase
{
 public:
  SampleMonitor(RTC::Manager *manager);
  ~SampleMonitor();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

 private:
  TimedOctetSeq m_in_data;
  InPort<TimedOctetSeq> m_InPort;

 private:
  int daq_dummy();
  int daq_configure();
  int daq_unconfigure();
  int daq_start();
  int daq_run();
  int daq_stop();
  int daq_pause();
  int daq_resume();

  int parse_params(::NVList *list);
  int reset_InPort();

  unsigned int read_InPort();
  // int online_analyze();

  BufferStatus m_in_status;
  bool m_debug;

  // User defined
  int decode_data(const unsigned char *mydata);
  int fill_data(const unsigned char *mydata, const int size);

  static constexpr uint kNCh = 8;
  TH1D *fHis[kNCh];
  TH2D *fHisInput[kNCh];
  TGraph *fGr[kNCh];
  TCanvas *fCanvas[kNCh];

  int m_bin;
  double m_min;
  double m_max;
  int m_monitor_update_rate;
  unsigned char m_recv_data[BUFFER_SIZE];
  unsigned int m_event_byte_size;
  struct SampleData m_sampleData;

  // Fitting
  void FitHis();
  void FirstFit(TH1D *his, TF1 *f);
  void RangeFit(TH1D *his, TF1 *f);
  bool fFirstFitFlag;
  TCanvas *fFitCan;
  TF1 *fFitFnc[kNCh];
  void UploadResults();

  // For online monitoring
  THttpServer *fServ;
};

extern "C" {
void SampleMonitorInit(RTC::Manager *manager);
};

#endif  // SAMPLEMONITOR_H

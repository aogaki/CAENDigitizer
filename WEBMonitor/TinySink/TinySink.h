// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef TINYSINK_H
#define TINYSINK_H

#include "DaqComponentBase.h"

#include <TApplication.h>
#include <TCanvas.h>
#include <TH1.h>
#include <THttpServer.h>
#include <TStyle.h>

#include "SampleData.h"

using namespace RTC;

class TinySink : public DAQMW::DaqComponentBase
{
 public:
  TinySink(RTC::Manager *manager);
  ~TinySink();

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
  static const unsigned int RECV_BUFFERSIZE = 4096;
  unsigned char m_data[RECV_BUFFERSIZE];

  BufferStatus m_in_status;

  int decode_data(const unsigned char *mydata);
  int fill_data(const unsigned char *mydata, const int size);

  TCanvas *m_canvas;
  TH1F *m_hist;
  constexpr static Int_t kNrHists{2400};  // Use a number multiplied by 8
  TH1D *fHisTest[kNrHists];
  int m_bin;
  double m_max;
  double m_min;
  int m_monitor_update_rate;
  unsigned char m_recv_data[4096];
  unsigned int m_event_byte_size;
  struct sampleData m_sampleData;

  // For online monitoring
  THttpServer *fServ;

  bool m_debug;
};

extern "C" {
void TinySinkInit(RTC::Manager *manager);
};

#endif  // TINYSINK_H

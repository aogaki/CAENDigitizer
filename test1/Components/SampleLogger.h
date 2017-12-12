// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef SAMPLELOGGER_H
#define SAMPLELOGGER_H

#include <TFile.h>
#include <TTree.h>

#include "DaqComponentBase.h"
#include "SampleData.h"

using namespace RTC;

class SampleLogger : public DAQMW::DaqComponentBase
{
 public:
  SampleLogger(RTC::Manager *manager);
  ~SampleLogger();

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

  TFile *fFile;
  TTree *fTree;
  Int_t fADC;

  int decode_data(const unsigned char *mydata);
  int fill_data(const unsigned char *mydata, const int size);

  unsigned char m_recv_data[4096];
  unsigned int m_event_byte_size;
  struct SampleData m_sampleData;
};

extern "C" {
void SampleLoggerInit(RTC::Manager *manager);
};

#endif  // SAMPLELOGGER_H

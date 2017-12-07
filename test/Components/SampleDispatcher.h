// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#ifndef SAMPLEDISPATCHER_H
#define SAMPLEDISPATCHER_H

#include "DaqComponentBase.h"

using namespace RTC;

class SampleDispatcher : public DAQMW::DaqComponentBase
{
 public:
  SampleDispatcher(RTC::Manager *manager);
  ~SampleDispatcher();

  // The initialize action (on CREATED->ALIVE transition)
  // former rtc_init_entry()
  virtual RTC::ReturnCode_t onInitialize();

  // The execution action that is invoked periodically
  // former rtc_active_do()
  virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

 private:
  TimedOctetSeq m_out_data1;
  OutPort<TimedOctetSeq> m_OutPort1;
  TimedOctetSeq m_out_data2;
  OutPort<TimedOctetSeq> m_OutPort2;

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
  int set_data(unsigned int data_byte_size);
  int write_OutPort();

  int reset_InPort();
  unsigned int read_InPort();

  static const int SEND_BUFFER_SIZE = 4096;
  unsigned char m_data[SEND_BUFFER_SIZE];
  unsigned char m_recv_data[SEND_BUFFER_SIZE];
  unsigned int m_recv_byte_size;

  BufferStatus m_out_status1;
  BufferStatus m_out_status2;
  BufferStatus m_in_status;
  bool m_debug;
};

extern "C" {
void SampleDispatcherInit(RTC::Manager *manager);
};

#endif  // SAMPLEDISPATCHER_H

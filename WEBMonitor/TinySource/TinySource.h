// -*- C++ -*-
/*!
 * @file 
 * @brief
 * @date
 * @author
 *
 */

#ifndef TINYSOURCE_H
#define TINYSOURCE_H

#include "DaqComponentBase.h"

#include <daqmw/Sock.h>

using namespace RTC;

class TinySource
   : public DAQMW::DaqComponentBase
{
public:
   TinySource(RTC::Manager* manager);
   ~TinySource();

   // The initialize action (on CREATED->ALIVE transition)
   // former rtc_init_entry()
   virtual RTC::ReturnCode_t onInitialize();

   // The execution action that is invoked periodically
   // former rtc_active_do()
   virtual RTC::ReturnCode_t onExecute(RTC::UniqueId ec_id);

private:
   TimedOctetSeq          m_out_data;
   OutPort<TimedOctetSeq> m_OutPort;

private:
   int daq_dummy();
   int daq_configure();
   int daq_unconfigure();
   int daq_start();
   int daq_run();
   int daq_stop();
   int daq_pause();
   int daq_resume();

   int parse_params(::NVList* list);
   int read_data_from_detectors();
   int set_data(unsigned int data_byte_size);
   int write_OutPort();

   DAQMW::Sock *m_sock;
   
   static const int EVENT_BYTE_SIZE = 8;
   static const int SEND_BUFFER_SIZE = 1024;
   unsigned char m_data[SEND_BUFFER_SIZE];
   unsigned int m_recv_byte_size;

   int m_srcPort;
   std::string m_srcAddr;
   
   BufferStatus m_out_status;
   bool m_debug;
};


extern "C"
{
    void TinySourceInit(RTC::Manager* manager);
};

#endif // TINYSOURCE_H
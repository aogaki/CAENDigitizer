// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "SampleDispatcher.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *sampledispatcher_spec[] = {"implementation_id",
                                              "SampleDispatcher",
                                              "type_name",
                                              "SampleDispatcher",
                                              "description",
                                              "SampleDispatcher component",
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

SampleDispatcher::SampleDispatcher(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort0("sampledispatcher_out0", m_out_data),
      m_OutPort1("sampledispatcher_out1", m_out_data),
      m_InPort0("sampledispatcher_in0", m_in_data),
      m_InPort1("sampledispatcher_in1", m_in_data),
      m_recv_byte_size(0),
      fDataSize(0),
      m_in_status0(BUF_SUCCESS),
      m_in_status1(BUF_SUCCESS),
      m_out_status0(BUF_SUCCESS),
      m_out_status1(BUF_SUCCESS),
      m_debug(true)
{
  // Registration: InPort/OutPort/Service

  // Set ports
  registerInPort("sampledispatcher_in0", m_InPort0);
  registerInPort("sampledispatcher_in1", m_InPort1);
  registerOutPort("sampledispatcher_out0", m_OutPort0);
  registerOutPort("sampledispatcher_out1", m_OutPort1);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLEDISPATCHER");
}

SampleDispatcher::~SampleDispatcher() {}

RTC::ReturnCode_t SampleDispatcher::onInitialize()
{
  if (m_debug) {
    std::cerr << "SampleDispatcher::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t SampleDispatcher::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int SampleDispatcher::daq_dummy() { return 0; }

int SampleDispatcher::daq_configure()
{
  std::cerr << "*** SampleDispatcher::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int SampleDispatcher::parse_params(::NVList *list)
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

int SampleDispatcher::daq_unconfigure()
{
  std::cerr << "*** SampleDispatcher::unconfigure" << std::endl;

  return 0;
}

int SampleDispatcher::daq_start()
{
  std::cerr << "*** SampleDispatcher::start" << std::endl;

  m_out_status0 = BUF_SUCCESS;
  bool outport_conn0 = check_dataPort_connections(m_OutPort0);
  if (!outport_conn0) {
    std::cerr << "No Connection0" << std::endl;
    fatal_error_report(DATAPATH_DISCONNECTED);
  }

  m_out_status0 = BUF_SUCCESS;
  bool outport_conn1 = check_dataPort_connections(m_OutPort1);
  if (!outport_conn1) {
    std::cerr << "No Connection1" << std::endl;
    fatal_error_report(DATAPATH_DISCONNECTED);
  }

  m_in_status0 = BUF_SUCCESS;
  m_in_status1 = BUF_SUCCESS;

  return 0;
}

int SampleDispatcher::daq_stop()
{
  std::cerr << "*** SampleDispatcher::stop" << std::endl;

  return 0;
}

int SampleDispatcher::daq_pause()
{
  std::cerr << "*** SampleDispatcher::pause" << std::endl;

  return 0;
}

int SampleDispatcher::daq_resume()
{
  std::cerr << "*** SampleDispatcher::resume" << std::endl;

  return 0;
}

int SampleDispatcher::reset_InPort()
{
  bool ret = true;
  while (ret == true) {
    ret = m_InPort0.read();
  }
  ret = true;
  while (ret == true) {
    ret = m_InPort1.read();
  }

  return 0;
}

unsigned int SampleDispatcher::read_InPort()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort0.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status0 = check_inPort_status(m_InPort0);
    if (m_in_status0 == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {         // Check if stop command has come.
        set_trans_unlock();             // Transit to CONFIGURE state.
      }
    } else if (m_in_status0 == BUF_FATAL) {  // Fatal error
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

unsigned int SampleDispatcher::read_InPort0()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort0.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status0 = check_inPort_status(m_InPort0);
    if (m_in_status0 == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {         // Check if stop command has come.
        set_trans_unlock();             // Transit to CONFIGURE state.
      }
    } else if (m_in_status0 == BUF_FATAL) {  // Fatal error
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

unsigned int SampleDispatcher::read_InPort1()
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = m_InPort1.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status1 = check_inPort_status(m_InPort1);
    if (m_in_status1 == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {         // Check if stop command has come.
        set_trans_unlock();             // Transit to CONFIGURE state.
      }
    } else if (m_in_status1 == BUF_FATAL) {  // Fatal error
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

unsigned int SampleDispatcher::read_InPort(InPort<TimedOctetSeq> &port)
{
  /////////////// read data from InPort Buffer ///////////////
  unsigned int recv_byte_size = 0;
  bool ret = port.read();

  //////////////////// check read status /////////////////////
  if (ret == false) {  // false: TIMEOUT or FATAL
    m_in_status0 = check_inPort_status(port);
    if (m_in_status0 == BUF_TIMEOUT) {  // Buffer empty.
      if (check_trans_lock()) {         // Check if stop command has come.
        set_trans_unlock();             // Transit to CONFIGURE state.
      }
    } else if (m_in_status0 == BUF_FATAL) {  // Fatal error
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

int SampleDispatcher::set_data(unsigned int data_byte_size)
{
  data_byte_size = fDataSize;

  unsigned char header[8];
  unsigned char footer[8];

  set_header(&header[0], data_byte_size);
  set_footer(&footer[0]);

  /// set OutPort buffer length
  m_out_data.data.length(data_byte_size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
  memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
  // memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &m_data[0], data_byte_size);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &fDataBuffer[0], data_byte_size);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + data_byte_size]), &footer[0],
         FOOTER_BYTE_SIZE);

  return 0;
}

int SampleDispatcher::write_OutPort()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort0.write();
  ret |= m_OutPort1.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out_status0 = check_outPort_status(m_OutPort0);
    if (m_out_status0 == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    } else if (m_out_status0 == BUF_TIMEOUT) {  // Timeout
      return -1;
    }

    m_out_status1 = check_outPort_status(m_OutPort1);
    if (m_out_status1 == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    } else if (m_out_status1 == BUF_TIMEOUT) {  // Timeout
      return -1;
    }
  } else {
    m_out_status0 = BUF_SUCCESS;  // successfully done
    m_out_status1 = BUF_SUCCESS;  // successfully done
  }

  return 0;
}

int SampleDispatcher::daq_run()
{
  if (m_debug) {
    std::cerr << "*** SampleDispatcher::run" << std::endl;
  }

  // unsigned int recv_byte_size = read_InPort();
  unsigned int recv_byte_size = read_InPort0();
  if (recv_byte_size > 0) {
    unsigned int event_byte_size = get_event_size(recv_byte_size);
    inc_total_data_size(event_byte_size);  // increase total data byte size
    memcpy(&m_data[0], &m_in_data.data[HEADER_BYTE_SIZE], event_byte_size);

    memcpy(&fDataBuffer[fDataSize], &m_in_data.data[HEADER_BYTE_SIZE],
           event_byte_size);
    fDataSize += event_byte_size;

    // if (fDataSize > kMaxPacketSize) {
    if (fDataSize > 0) {
      if (m_out_status0 == BUF_SUCCESS &&
          m_out_status1 ==
              BUF_SUCCESS) {  // previous OutPort.write() successfully done
        if (event_byte_size > 0) {
          set_data(event_byte_size);  // set data to OutPort Buffer
        }
      }

      if (write_OutPort() < 0) {
        ;       // Timeout. do nothing.
      } else {  // OutPort write successfully done
        // inc_sequence_num();                    // increase sequence num.
        // inc_total_data_size(event_byte_size);  // increase total data byte
        // size
      }

      fDataSize = 0;
    }
  }

  recv_byte_size = read_InPort1();
  if (recv_byte_size == 0) {  // Timeout
    return 0;
  }
  // check_header_footer(m_in_data, recv_byte_size);  // check header and footer
  unsigned int event_byte_size = get_event_size(recv_byte_size);
  memcpy(&m_data[0], &m_in_data.data[HEADER_BYTE_SIZE], event_byte_size);

  memcpy(&fDataBuffer[fDataSize], &m_in_data.data[HEADER_BYTE_SIZE],
         event_byte_size);
  fDataSize += event_byte_size;

  // if (fDataSize > kMaxPacketSize) {
  if (fDataSize > 0) {
    if (m_out_status0 == BUF_SUCCESS &&
        m_out_status1 ==
            BUF_SUCCESS) {  // previous OutPort.write() successfully done
      if (event_byte_size > 0) {
        set_data(event_byte_size);  // set data to OutPort Buffer
      }
    }

    if (write_OutPort() < 0) {
      ;       // Timeout. do nothing.
    } else {  // OutPort write successfully done
      // inc_sequence_num();                    // increase sequence num.
      // inc_total_data_size(event_byte_size);  // increase total data byte size
    }

    fDataSize = 0;
  }

  inc_sequence_num();                    // increase sequence num.
  inc_total_data_size(event_byte_size);  // increase total data byte size
  return 0;
}

extern "C" {
void SampleDispatcherInit(RTC::Manager *manager)
{
  RTC::Properties profile(sampledispatcher_spec);
  manager->registerFactory(profile, RTC::Create<SampleDispatcher>,
                           RTC::Delete<SampleDispatcher>);
}
};

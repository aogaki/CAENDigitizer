// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */
#include <arpa/inet.h>

#include "SampleMonitor.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

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

SampleMonitor::SampleMonitor(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("samplemonitor_in", m_in_data),
      m_in_status(BUF_SUCCESS),
      m_debug(true),
      fHisCanvas(nullptr),
      fHis(nullptr),
      fGrCanvas(nullptr),
      fGr(nullptr),
      m_bin(0),
      m_min(0),
      m_max(0),
      m_monitor_update_rate(300),
      m_event_byte_size(0)
{
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort("samplemonitor_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLEMONITOR");
}

SampleMonitor::~SampleMonitor() {}

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
  if (fHisCanvas) {
    fHisCanvas->Update();
    sleep(1);
  }

  return 0;
}

int SampleMonitor::daq_configure()
{
  std::cerr << "*** SampleMonitor::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  DelPointer(fHisCanvas);
  fHisCanvas = new TCanvas("HisCanvas", "test");

  DelPointer(fHis);
  fHis = new TH1D("hist", "test", 20000, 0., 20000.);

  DelPointer(fGrCanvas);
  fGrCanvas = new TCanvas("GrCanvas", "test");

  DelPointer(fGr);
  fGr = new TGraph();
  fGr->SetMinimum(7000);
  fGr->SetMaximum(9000);

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

  DelPointer(fHisCanvas);
  DelPointer(fHis);
  DelPointer(fGrCanvas);
  DelPointer(fGr);

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

  fHisCanvas->cd();
  fHis->Draw();
  fHisCanvas->Update();

  fGrCanvas->cd();
  fGr->Draw();
  fGrCanvas->Update();

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

  if (m_monitor_update_rate == 0) m_monitor_update_rate = 1000;

  unsigned long sequence_num = get_sequence_num();
  if ((sequence_num % m_monitor_update_rate) == 0) {
    fHisCanvas->cd();
    fHis->Draw();
    fHisCanvas->Update();

    fGrCanvas->cd();
    fGr->Draw();
    fGrCanvas->Update();
  }

  inc_sequence_num();                      // increase sequence num.
  inc_total_data_size(m_event_byte_size);  // increase total data byte size

  return 0;
}

int SampleMonitor::fill_data(const unsigned char *mydata, const int size)
{
  for (int i = 0; i < size / int(ONE_HIT_SIZE); i++) {
    decode_data(mydata);
    if (m_sampleData.ChNumber == 0) {
      fHis->Fill(m_sampleData.ADC);
      for (int i = 0; i < kNSamples; i++)
        fGr->SetPoint(i, i, m_sampleData.Waveform[i]);
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

  unsigned long timeStamp = *(unsigned int *)&mydata[index];
  m_sampleData.TimeStamp = timeStamp;
  index += sizeof(timeStamp);

  unsigned int adc = *(unsigned int *)&mydata[index];
  m_sampleData.ADC = adc;
  index += sizeof(adc);

  for (int i = 0; i < kNSamples; i++) {
    unsigned short pulse = *(unsigned short *)&mydata[index];
    m_sampleData.Waveform[i] = pulse;
    index += sizeof(pulse);
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

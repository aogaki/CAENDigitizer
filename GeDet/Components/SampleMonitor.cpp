// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */
#include <arpa/inet.h>

#include <TSystem.h>

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

SampleMonitor::SampleMonitor(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("samplemonitor_in", m_in_data),
      m_in_status(BUF_SUCCESS),
      m_debug(true),
      fHis(nullptr),
      fHisInput(nullptr),
      fGr(nullptr),
      m_bin(0),
      m_min(0),
      m_max(0),
      m_monitor_update_rate(300),
      m_event_byte_size(0),
      fServ(nullptr)
{
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort("samplemonitor_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLEMONITOR");

  fHis = new TH1D("His", "Energy distribution", 20000, 0., 20000.);
  fHisInput = new TH2D("HisInput", "Integral of input signal", kNSample / 10,
                       0.5, kNSample + 0.5, 1800, 0.5, 18000.5);

  fGr = new TGraph();
  for (auto i = 0; i < kNSample; i++) fGr->SetPoint(i, i + 1, 8000);
  fGr->SetTitle("Graph");
  fGr->SetMinimum(0);
  fGr->SetMaximum(18000);
  fCanvas = new TCanvas();
  fGr->Draw("AL");

  fServ = new THttpServer("http:8080?monitoring=5000;rw;noglobal");
  fServ->SetDefaultPage("index.html");
  fServ->Register("/", fHis);
  fServ->Register("/", fHisInput);
  fServ->Register("/graph", fCanvas);
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

  fHis->Reset();

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
  gSystem->ProcessEvents();

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

  if (m_monitor_update_rate == 0) m_monitor_update_rate = 1000;

  // unsigned long sequence_num = get_sequence_num();
  // if ((sequence_num % m_monitor_update_rate) == 0) {
  gSystem->ProcessEvents();
  // }

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
      for (int i = 0; i < kNSample; i++) {
        fGr->SetPoint(i, i, m_sampleData.Waveform[i]);
        fHisInput->Fill(i, m_sampleData.Waveform[i]);
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

extern "C" {
void SampleMonitorInit(RTC::Manager *manager)
{
  RTC::Properties profile(samplemonitor_spec);
  manager->registerFactory(profile, RTC::Create<SampleMonitor>,
                           RTC::Delete<SampleMonitor>);
}
};

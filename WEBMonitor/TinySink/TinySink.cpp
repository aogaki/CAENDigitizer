// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include <arpa/inet.h>

#include <TString.h>
#include <TSystem.h>

#include "TinySink.h"

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
static const char *tinysink_spec[] = {"implementation_id",
                                      "TinySink",
                                      "type_name",
                                      "TinySink",
                                      "description",
                                      "TinySink component",
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

TinySink::TinySink(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("tinysink_in", m_in_data),
      m_in_status(BUF_SUCCESS),
      m_canvas(nullptr),
      m_hist(nullptr),
      m_bin(1000),
      m_max(1000.),
      m_min(0.),
      m_monitor_update_rate(30),
      m_event_byte_size(0),
      fServ(nullptr),

      m_debug(false)
{
  // Registration: InPort/OutPort/Service

  for (auto &his : fHisTest) his = nullptr;

  // Set InPort buffers
  registerInPort("tinysink_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("TINYSINK");
}

TinySink::~TinySink() {}

RTC::ReturnCode_t TinySink::onInitialize()
{
  if (m_debug) {
    std::cerr << "TinySink::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t TinySink::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int TinySink::daq_dummy()
{
  if (m_canvas) {
    m_canvas->Update();
    sleep(1);
  }

  gSystem->ProcessEvents();

  return 0;
}

int TinySink::daq_configure()
{
  std::cerr << "*** TinySink::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  if (m_monitor_update_rate == 0) m_monitor_update_rate = 1000;

  DelPointer(m_canvas);
  m_canvas = new TCanvas("m_canvas", "test");

  DelPointer(m_hist);
  m_hist = new TH1F("m_hist", "test", m_bin, m_min, m_max);

  DelPointer(fServ);
  fServ = new THttpServer("http:8080?monitoring=5000;rw;noglobal");
  fServ->SetDefaultPage("index.html");
  fServ->Register("/", m_hist);

  for (auto i = 0; i < kNrHists; i++) {
    DelPointer(fHisTest[i]);
    fHisTest[i] = new TH1D(Form("HisTest%03d", i), "test", m_bin, m_min, m_max);
    fServ->Register(Form("/hists%02d", i / 10), fHisTest[i]);
  }

  return 0;
}

int TinySink::parse_params(::NVList *list)
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

int TinySink::daq_unconfigure()
{
  std::cerr << "*** TinySink::unconfigure" << std::endl;

  DelPointer(m_canvas);
  DelPointer(m_hist);
  DelPointer(fServ);

  for (auto i = 0; i < kNrHists; i++) DelPointer(fHisTest[i]);

  return 0;
}

int TinySink::daq_start()
{
  std::cerr << "*** TinySink::start" << std::endl;

  m_in_status = BUF_SUCCESS;

  return 0;
}

int TinySink::daq_stop()
{
  std::cerr << "*** TinySink::stop" << std::endl;

  m_hist->Draw();
  m_canvas->Update();

  gSystem->ProcessEvents();

  reset_InPort();

  return 0;
}

int TinySink::daq_pause()
{
  std::cerr << "*** TinySink::pause" << std::endl;

  return 0;
}

int TinySink::daq_resume()
{
  std::cerr << "*** TinySink::resume" << std::endl;

  return 0;
}

int TinySink::reset_InPort()
{
  int ret = true;
  while (ret == true) {
    ret = m_InPort.read();
  }

  return 0;
}

unsigned int TinySink::read_InPort()
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

int TinySink::daq_run()
{
  if (m_debug) {
    std::cerr << "*** TinySink::run" << std::endl;
  }

  unsigned int recv_byte_size = read_InPort();
  if (recv_byte_size == 0) {  // Timeout
    return 0;
  }

  check_header_footer(m_in_data, recv_byte_size);  // check header and footer
  m_event_byte_size = get_event_size(recv_byte_size);

  /////////////  Write component main logic here. /////////////
  // online_analyze();
  memcpy(m_recv_data, &m_in_data.data[HEADER_BYTE_SIZE], m_event_byte_size);
  fill_data(m_recv_data, m_event_byte_size);

  /////////////////////////////////////////////////////////////

  auto sequence_num = get_sequence_num();
  if ((sequence_num % m_monitor_update_rate) == 0) {
    m_hist->Draw();
    m_canvas->Update();
  }

  inc_sequence_num();                      // increase sequence num.
  inc_total_data_size(m_event_byte_size);  // increase total data byte size

  gSystem->ProcessEvents();

  return 0;
}

int TinySink::fill_data(const unsigned char *mydata, const int size)
{
  constexpr auto kNMod = 8;
  constexpr auto kLoop = kNrHists / kNMod;

  for (int i = 0; i < size / ONE_EVENT_SIZE; i++) {
    decode_data(mydata);
    m_hist->Fill(m_sampleData.data / 1000.);

    for (auto j = 0; j < kLoop; j++) {
      auto mod = int(m_sampleData.module_num) + j * kNMod;
      fHisTest[mod]->Fill(m_sampleData.data / 1000.);
    }
    mydata += ONE_EVENT_SIZE;
  }

  return 0;
}

int TinySink::decode_data(const unsigned char *mydata)
{
  m_sampleData.magic = mydata[0];
  m_sampleData.format_ver = mydata[1];
  m_sampleData.module_num = mydata[2];
  m_sampleData.reserved = mydata[3];
  m_sampleData.data = ntohl(*(unsigned int *)&mydata[4]);

  return 0;
}

extern "C" {
void TinySinkInit(RTC::Manager *manager)
{
  RTC::Properties profile(tinysink_spec);
  manager->registerFactory(profile, RTC::Create<TinySink>,
                           RTC::Delete<TinySink>);
}
};

// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */
#include <arpa/inet.h>

#include "SampleLogger.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::FOOTER_DATA_MISMATCH;
using DAQMW::FatalType::HEADER_DATA_MISMATCH;
using DAQMW::FatalType::INPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;

// Module specification
// Change following items to suit your component's spec.
static const char *samplelogger_spec[] = {"implementation_id",
                                          "SampleLogger",
                                          "type_name",
                                          "SampleLogger",
                                          "description",
                                          "SampleLogger component",
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

SampleLogger::SampleLogger(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_InPort("samplelogger_in", m_in_data),
      m_in_status(BUF_SUCCESS),

      m_debug(false),
      fFile(nullptr),
      fTree(nullptr)
{
  // Registration: InPort/OutPort/Service

  // Set InPort buffers
  registerInPort("samplelogger_in", m_InPort);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLELOGGER");
}

SampleLogger::~SampleLogger() {}

RTC::ReturnCode_t SampleLogger::onInitialize()
{
  if (m_debug) {
    std::cerr << "SampleLogger::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t SampleLogger::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int SampleLogger::daq_dummy() { return 0; }

int SampleLogger::daq_configure()
{
  std::cerr << "*** SampleLogger::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int SampleLogger::parse_params(::NVList *list)
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

int SampleLogger::daq_unconfigure()
{
  std::cerr << "*** SampleLogger::unconfigure" << std::endl;

  return 0;
}

int SampleLogger::daq_start()
{
  std::cerr << "*** SampleLogger::start" << std::endl;

  m_in_status = BUF_SUCCESS;

  fFile = new TFile("/tmp/daqmw/test.root", "RECREATE");
  fTree = new TTree("ADC", "test data");
  fTree->Branch("ADC", &fADC, "ADC/I");

  return 0;
}

int SampleLogger::daq_stop()
{
  std::cerr << "*** SampleLogger::stop" << std::endl;
  reset_InPort();

  fTree->Write();
  fFile->Close();

  return 0;
}

int SampleLogger::daq_pause()
{
  std::cerr << "*** SampleLogger::pause" << std::endl;

  return 0;
}

int SampleLogger::daq_resume()
{
  std::cerr << "*** SampleLogger::resume" << std::endl;

  return 0;
}

int SampleLogger::reset_InPort()
{
  int ret = true;
  while (ret == true) {
    ret = m_InPort.read();
  }

  return 0;
}

unsigned int SampleLogger::read_InPort()
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

int SampleLogger::daq_run()
{
  if (m_debug) {
    std::cerr << "*** SampleLogger::run" << std::endl;
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

  inc_sequence_num();                      // increase sequence num.
  inc_total_data_size(m_event_byte_size);  // increase total data byte size

  return 0;
}

int SampleLogger::fill_data(const unsigned char *mydata, const int size)
{
  for (int i = 0; i < size / int(ONE_HIT_SIZE); i++) {
    decode_data(mydata);
    fADC = m_sampleData.ADC;
    fTree->Fill();  // Stupid!
    mydata += ONE_HIT_SIZE;
  }

  return 0;
}

int SampleLogger::decode_data(const unsigned char *mydata)
{
  unsigned int index = 0;
  m_sampleData.ModNumber = mydata[index++];
  m_sampleData.ChNumber = mydata[index++];

  unsigned int timeStamp = *(unsigned int *)&mydata[index];
  m_sampleData.TimeStamp = ntohl(timeStamp);
  index += sizeof(timeStamp);

  unsigned int adc = *(unsigned int *)&mydata[index];
  m_sampleData.ADC = ntohl(adc);
  index += sizeof(adc);
  /*
  for (int i = 0; i < kNSamples; i++) {
    unsigned short pulse = *(unsigned short *)&mydata[index];
    m_sampleData.Waveform[i] = ntohs(pulse);
    index += sizeof(pulse);
  }
  */
}

extern "C" {
void SampleLoggerInit(RTC::Manager *manager)
{
  RTC::Properties profile(samplelogger_spec);
  manager->registerFactory(profile, RTC::Create<SampleLogger>,
                           RTC::Delete<SampleLogger>);
}
};

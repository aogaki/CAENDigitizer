// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "SampleData.h"
#include "SampleReader.h"

#include <TRandom3.h>

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;
using DAQMW::FatalType::USER_DEFINED_ERROR2;

// Module specification
// Change following items to suit your component's spec.
static const char *samplereader_spec[] = {"implementation_id",
                                          "SampleReader",
                                          "type_name",
                                          "SampleReader",
                                          "description",
                                          "SampleReader component",
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

SampleReader::SampleReader(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("samplereader_out", m_out_data),
      m_sock(nullptr),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),
      m_debug(true)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("samplereader_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLEREADER");

  fDummyData.reset(new unsigned char[fNHits * ONE_HIT_SIZE]);
}

SampleReader::~SampleReader() {}

RTC::ReturnCode_t SampleReader::onInitialize()
{
  if (m_debug) {
    std::cerr << "SampleReader::onInitialize()" << std::endl;
  }

  return RTC::RTC_OK;
}

RTC::ReturnCode_t SampleReader::onExecute(RTC::UniqueId ec_id)
{
  daq_do();

  return RTC::RTC_OK;
}

int SampleReader::daq_dummy() { return 0; }

int SampleReader::daq_configure()
{
  std::cerr << "*** SampleReader::configure" << std::endl;

  ::NVList *paramList;
  paramList = m_daq_service0.getCompParams();
  parse_params(paramList);

  return 0;
}

int SampleReader::parse_params(::NVList *list)
{
  bool srcAddrSpecified = false;
  bool srcPortSpecified = false;

  std::cerr << "param list length:" << (*list).length() << std::endl;

  int len = (*list).length();
  for (int i = 0; i < len; i += 2) {
    std::string sname = (std::string)(*list)[i].value;
    std::string svalue = (std::string)(*list)[i + 1].value;

    std::cerr << "sname: " << sname << "  ";
    std::cerr << "value: " << svalue << std::endl;

    if (sname == "srcAddr") {
      srcAddrSpecified = true;
      m_srcAddr = svalue;
    } else if (sname == "srcPort") {
      srcPortSpecified = true;
      // char *offset;
      // m_srcPort = (int)strtol(svalue.c_str(), &offset, 10);
      // delete offset;
      m_srcPort = (int)strtol(svalue.c_str(), nullptr, 10);
    }
  }

  if (!srcAddrSpecified) {
    fatal_error_report(USER_DEFINED_ERROR1, "NO SRC ADDRESS");
  }

  if (!srcPortSpecified) {
    fatal_error_report(USER_DEFINED_ERROR2, "NO SRC PORT");
  }

  return 0;
}

int SampleReader::daq_unconfigure()
{
  std::cerr << "*** SampleReader::unconfigure" << std::endl;

  return 0;
}

int SampleReader::daq_start()
{
  std::cerr << "*** SampleReader::start" << std::endl;

  m_out_status = BUF_SUCCESS;

  bool outport_conn = check_dataPort_connections(m_OutPort);
  if (!outport_conn) {
    std::cerr << "No Connection" << std::endl;
    fatal_error_report(DATAPATH_DISCONNECTED);
  }

  return 0;
}

int SampleReader::daq_stop()
{
  std::cerr << "*** SampleReader::stop" << std::endl;

  return 0;
}

int SampleReader::daq_pause()
{
  std::cerr << "*** SampleReader::pause" << std::endl;

  return 0;
}

int SampleReader::daq_resume()
{
  std::cerr << "*** SampleReader::resume" << std::endl;

  return 0;
}

int SampleReader::set_data(unsigned int data_byte_size)
{
  unsigned char header[8];
  unsigned char footer[8];

  set_header(&header[0], data_byte_size);
  set_footer(&footer[0]);

  /// set OutPort buffer length
  m_out_data.data.length(data_byte_size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
  memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &m_data[0], data_byte_size);
  memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + data_byte_size]), &footer[0],
         FOOTER_BYTE_SIZE);

  return 0;
}

int SampleReader::write_OutPort()
{
  ////////////////// send data from OutPort  //////////////////
  bool ret = m_OutPort.write();

  //////////////////// check write status /////////////////////
  if (ret == false) {  // TIMEOUT or FATAL
    m_out_status = check_outPort_status(m_OutPort);
    if (m_out_status == BUF_FATAL) {  // Fatal error
      fatal_error_report(OUTPORT_ERROR);
    }
    if (m_out_status == BUF_TIMEOUT) {  // Timeout
      return -1;
    }
  } else {
    m_out_status = BUF_SUCCESS;  // successfully done
  }

  return 0;
}

int SampleReader::daq_run()
{
  if (m_debug) {
    std::cerr << "*** SampleReader::run" << std::endl;
  }

  if (check_trans_lock()) {  // check if stop command has come
    set_trans_unlock();      // transit to CONFIGURED state
    return 0;
  }

  if (m_out_status == BUF_SUCCESS) {
    // previous OutPort.write() successfully done
    // Stupid! rewrite it!
    // fDigitizer->SendSWTrigger();

    // auto dataArray = fDigitizer->GetDataArray();
    // const int nHit = fDigitizer->GetNEvents();
    MakeDummyData();
    auto dataArray = fDummyData.get();
    const int nHit = fNHits;
    if (m_debug && nHit > 0) std::cout << nHit << std::endl;

    for (unsigned int iHit = 0, iData = 0; iHit < nHit; iHit++) {
      auto index = iHit * ONE_HIT_SIZE;
      memcpy(&m_data[iData * ONE_HIT_SIZE], &dataArray[index], ONE_HIT_SIZE);
      iData++;
      m_recv_byte_size += ONE_HIT_SIZE;

      constexpr int sizeTh = 2000000 - ONE_HIT_SIZE;  // 2M is limit
      // constexpr int sizeTh = 200000000 - ONE_HIT_SIZE;  // 2M is limit
      if (m_recv_byte_size > sizeTh) {
        set_data(m_recv_byte_size);  // set data to OutPort Buffer
        if (write_OutPort() < 0) {
          ;                    // Timeout. do nothing.
        } else {               // OutPort write successfully done
          inc_sequence_num();  // increase sequence num.
          inc_total_data_size(
              m_recv_byte_size);  // increase total data byte size
          m_recv_byte_size = 0;
          iData = 0;
        }
      }
    }
    if (nHit > 0 && m_recv_byte_size > 0) {
      set_data(m_recv_byte_size);
      if (write_OutPort() < 0) {
        ;                    // Timeout. do nothing.
      } else {               // OutPort write successfully done
        inc_sequence_num();  // increase sequence num.
        inc_total_data_size(m_recv_byte_size);
        m_recv_byte_size = 0;
      }
    }
  }

  usleep(1000);

  return 0;
}

void SampleReader::MakeDummyData()
{
  SampleData data;
  for (auto i = 0; i < fNHits; i++) {
    data.ModNumber = 0;
    data.ChNumber = 0;
    data.TimeStamp = gRandom->Poisson(1024);
    data.ADC = gRandom->Poisson(511);
    for (int j = 0; j < kNSamples; j++)
      data.Waveform[j] = gRandom->Gaus(8000, 100);

    auto index = i * ONE_HIT_SIZE;
    fDummyData[index++] = data.ModNumber;
    fDummyData[index++] = data.ChNumber;

    constexpr auto timeSize = sizeof(data.TimeStamp);
    memcpy(&fDummyData[index], &data.TimeStamp, timeSize);
    index += timeSize;

    constexpr auto adcSize = sizeof(data.ADC);
    memcpy(&fDummyData[index], &data.ADC, adcSize);
    index += adcSize;

    constexpr auto waveSize = sizeof(data.Waveform[0]) * kNSamples;
    memcpy(&fDummyData[index], data.Waveform, waveSize);
    index += waveSize;
  }
}

extern "C" {
void SampleReaderInit(RTC::Manager *manager)
{
  RTC::Properties profile(samplereader_spec);
  manager->registerFactory(profile, RTC::Create<SampleReader>,
                           RTC::Delete<SampleReader>);
}
};

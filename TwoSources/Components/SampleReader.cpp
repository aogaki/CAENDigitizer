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

template <class T>
void DelPointer(T *&pointer)
{
  delete pointer;
  pointer = nullptr;
}

SampleReader::SampleReader(RTC::Manager *manager)
    : DAQMW::DaqComponentBase(manager),
      m_OutPort("samplereader_out", m_out_data),
      m_sock(nullptr),
      m_recv_byte_size(0),
      m_out_status(BUF_SUCCESS),
      m_debug(true),
      fSyncMode(false)
{
  // Registration: InPort/OutPort/Service

  // Set OutPort buffers
  registerOutPort("samplereader_out", m_OutPort);

  init_command_port();
  init_state_table();
  set_comp_name("SAMPLEREADER");
}

SampleReader::~SampleReader()
{
  for (auto &&pointer : fDigitizerVec) DelPointer(pointer);
}

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

  fDigitizerVec.push_back(new TPHA(CAEN_DGTZ_OpticalLink, 0, 0));
  // fDigitizerVec.push_back(new TPHA(CAEN_DGTZ_OpticalLink, 0, 1));
  for (unsigned int i = 0; i < fDigitizerVec.size(); i++) {
    fDigitizerVec[i]->Initialize();
    fDigitizerVec[i]->SetModNumber(i);
  }

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

  for (auto &&pointer : fDigitizerVec) DelPointer(pointer);
  fDigitizerVec.clear();

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

  if (fSyncMode) {
    // fDigitizerVec[0]->SetMaster();
    // for (int i = 1; i < fDigitizerVec.size(); i++)
    // fDigitizerVec[i]->SetSlave();
    // fDigitizerVec[0]->StartAcquisition();
    for (auto &&digi : fDigitizerVec) digi->StartSyncMode(fDigitizerVec.size());
    // fDigitizerVec[0]->StartAcquisition();
    // for (auto &&digi : fDigitizerVec) digi->StartAcquisition();
    // fDigitizerVec[0]->SendSWTrigger();
    for (auto &&digi : fDigitizerVec) digi->SendSWTrigger();
  } else {
    for (auto &&digi : fDigitizerVec) digi->StartAcquisition();
  }

  return 0;
}

int SampleReader::daq_stop()
{
  std::cerr << "*** SampleReader::stop" << std::endl;

  for (auto &&digi : fDigitizerVec) {
    digi->StopAcquisition();
    digi->ReadEvents();
  }

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
    for (auto &&digi : fDigitizerVec) {
      digi->ReadEvents();
      auto dataArray = digi->GetDataArray();
      const int nHit = digi->GetNEvents();
      if (m_debug && nHit > 0) std::cout << nHit << std::endl;

      for (unsigned int iHit = 0, iData = 0; iHit < nHit; iHit++) {
        auto index = iHit * ONE_HIT_SIZE;
        memcpy(&m_data[iData * ONE_HIT_SIZE], &dataArray[index], ONE_HIT_SIZE);
        iData++;
        m_recv_byte_size += ONE_HIT_SIZE;

        if (m_recv_byte_size > kMaxPacketSize) {
          set_data(m_recv_byte_size);  // set data to OutPort Buffer
          if (write_OutPort() < 0) {
            std::cout << "time out" << std::endl;
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
  }

  return 0;
}

extern "C" {
void SampleReaderInit(RTC::Manager *manager)
{
  RTC::Properties profile(samplereader_spec);
  manager->registerFactory(profile, RTC::Create<SampleReader>,
                           RTC::Delete<SampleReader>);
}
};

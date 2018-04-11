// -*- C++ -*-
/*!
 * @file
 * @brief
 * @date
 * @author
 *
 */

#include "TinySource.h"

using DAQMW::FatalType::DATAPATH_DISCONNECTED;
using DAQMW::FatalType::OUTPORT_ERROR;
using DAQMW::FatalType::USER_DEFINED_ERROR1;
using DAQMW::FatalType::USER_DEFINED_ERROR2;

// Module specification
// Change following items to suit your component's spec.
static const char* tinysource_spec[] =
{
   "implementation_id", "TinySource",
   "type_name",         "TinySource",
   "description",       "TinySource component",
   "version",           "1.0",
   "vendor",            "Kazuo Nakayoshi, KEK",
   "category",          "example",
   "activity_type",     "DataFlowComponent",
   "max_instance",      "1",
   "language",          "C++",
   "lang_type",         "compile",
   ""
};

TinySource::TinySource(RTC::Manager* manager)
   : DAQMW::DaqComponentBase(manager),
   m_OutPort("tinysource_out", m_out_data),
   m_sock(nullptr),
   m_recv_byte_size(0),
   m_out_status(BUF_SUCCESS),

   m_debug(false)
{
   // Registration: InPort/OutPort/Service

   // Set OutPort buffers
   registerOutPort("tinysource_out", m_OutPort);

   init_command_port();
   init_state_table();
   set_comp_name("TINYSOURCE");
}

TinySource::~TinySource()
{
}

RTC::ReturnCode_t TinySource::onInitialize()
{
   if (m_debug) {
      std::cerr << "TinySource::onInitialize()" << std::endl;
   }

   return RTC::RTC_OK;
}

RTC::ReturnCode_t TinySource::onExecute(RTC::UniqueId ec_id)
{
   daq_do();

   return RTC::RTC_OK;
}

int TinySource::daq_dummy()
{
   return 0;
}

int TinySource::daq_configure()
{
   std::cerr << "*** TinySource::configure" << std::endl;

   ::NVList* paramList;
   paramList = m_daq_service0.getCompParams();
   parse_params(paramList);

   return 0;
}

int TinySource::parse_params(::NVList* list)
{
   std::cerr << "param list length:" << (*list).length() << std::endl;

   bool srcAddrSpecified = false;
   bool srcPortSpecified = false;
   
   int len = (*list).length();
   for (int i = 0; i < len; i+=2) {
      std::string sname  = (std::string)(*list)[i].value;
      std::string svalue = (std::string)(*list)[i+1].value;

      std::cerr << "sname: " << sname << "  ";
      std::cerr << "value: " << svalue << std::endl;

      if(sname == "srcAddr"){
         srcAddrSpecified = true;
         m_srcAddr = svalue;
      }
      else if(sname == "srcPort"){
         srcPortSpecified = true;
         m_srcPort = std::stoi(svalue);
      }
   }

   if(!srcAddrSpecified){
      std::cerr << "ERROR: Data source address is not specified." << std::endl;
      fatal_error_report(USER_DEFINED_ERROR1, "NO SRC ADDRESS");
   }
   if(!srcPortSpecified){
      std::cerr << "ERROR: Data source port is not specified." << std::endl;
      fatal_error_report(USER_DEFINED_ERROR2, "NO SRC PORT");
   }
   
   return 0;
}

int TinySource::daq_unconfigure()
{
   std::cerr << "*** TinySource::unconfigure" << std::endl;

   return 0;
}

int TinySource::daq_start()
{
   std::cerr << "*** TinySource::start" << std::endl;

   m_out_status = BUF_SUCCESS;

   try{
      m_sock = new DAQMW::Sock();
      m_sock->connect(m_srcAddr, m_srcPort);
   } catch (DAQMW::SockException &e){
      std::cerr << "Sock fatal error: " << e.what() << std::endl;
      fatal_error_report(USER_DEFINED_ERROR1, "SOCKET FATAL ERROR");
   } catch (...){
      std::cerr << "Sock fatal error: Unknown" << std::endl;
      fatal_error_report(USER_DEFINED_ERROR1, "SOCKET FATAL ERROR");
   }

   if (check_dataPort_connections(m_OutPort) == false){
      std::cerr << "ERROR: No connection to out port" << std::endl;
      fatal_error_report(DATAPATH_DISCONNECTED);
   }
   
   return 0;
}

int TinySource::daq_stop()
{
   std::cerr << "*** TinySource::stop" << std::endl;

   if (m_sock) {
      m_sock->disconnect();
      delete m_sock;
      m_sock = nullptr;
   }
   
   return 0;
}

int TinySource::daq_pause()
{
   std::cerr << "*** TinySource::pause" << std::endl;

   return 0;
}

int TinySource::daq_resume()
{
   std::cerr << "*** TinySource::resume" << std::endl;

   return 0;
}

int TinySource::read_data_from_detectors()
{
   int received_data_size = 0;
   /// write your logic here
   int status = m_sock->readAll(m_data, SEND_BUFFER_SIZE);
   if(status == DAQMW::Sock::ERROR_FATAL){
      std::cerr << "ERROR: m_sock->ReadAll" << std::endl;
      fatal_error_report(USER_DEFINED_ERROR1, "SOCKET FATAL ERROR");
   }
   else if(status == DAQMW::Sock::ERROR_TIMEOUT){
      std::cerr << "TIMEOUT: m_sock->ReadAll" << std::endl;
      fatal_error_report(USER_DEFINED_ERROR2, "SOCKET TIMEOUT");
   }
   else received_data_size = SEND_BUFFER_SIZE;
    
   return received_data_size;
}

int TinySource::set_data(unsigned int data_byte_size)
{
   unsigned char header[8];
   unsigned char footer[8];

   set_header(&header[0], data_byte_size);
   set_footer(&footer[0]);

   ///set OutPort buffer length
   m_out_data.data.length(data_byte_size + HEADER_BYTE_SIZE + FOOTER_BYTE_SIZE);
   memcpy(&(m_out_data.data[0]), &header[0], HEADER_BYTE_SIZE);
   memcpy(&(m_out_data.data[HEADER_BYTE_SIZE]), &m_data[0], data_byte_size);
   memcpy(&(m_out_data.data[HEADER_BYTE_SIZE + data_byte_size]), &footer[0],
          FOOTER_BYTE_SIZE);

   return 0;
}

int TinySource::write_OutPort()
{
   ////////////////// send data from OutPort  //////////////////
   bool ret = m_OutPort.write();

   //////////////////// check write status /////////////////////
   if (ret == false) {  // TIMEOUT or FATAL
      m_out_status  = check_outPort_status(m_OutPort);
      if (m_out_status == BUF_FATAL) {   // Fatal error
         fatal_error_report(OUTPORT_ERROR);
      }
      if (m_out_status == BUF_TIMEOUT) { // Timeout
         return -1;
      }
   }
   else {
      m_out_status = BUF_SUCCESS; // successfully done
   }

   return 0;
}

int TinySource::daq_run()
{
   if (m_debug) {
      std::cerr << "*** TinySource::run" << std::endl;
   }

   if (check_trans_lock()) {  // check if stop command has come
      set_trans_unlock();    // transit to CONFIGURED state
      return 0;
   }

   if (m_out_status == BUF_SUCCESS) {   // previous OutPort.write() successfully done
      m_recv_byte_size = read_data_from_detectors();
      if (m_recv_byte_size > 0) {
         set_data(m_recv_byte_size); // set data to OutPort Buffer
      }
   }

   if (write_OutPort() < 0) {
      ;     // Timeout. do nothing.
   }
   else {    // OutPort write successfully done
      inc_sequence_num();                     // increase sequence num.
      inc_total_data_size(m_recv_byte_size);  // increase total data byte size
   }

   return 0;
}

extern "C"
{
   void TinySourceInit(RTC::Manager* manager)
   {
      RTC::Properties profile(tinysource_spec);
      manager->registerFactory(profile,
                               RTC::Create<TinySource>,
                               RTC::Delete<TinySource>);
   }
};

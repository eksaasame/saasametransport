
// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.
#pragma once

#ifndef common_service_handler_H
#define common_service_handler_H

#include "saasame_constants.h"
#include "common_service.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TSSLSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include "macho.h"
#include "json_spirit.h"
#include "..\buildenv.h"
#include <openssl\err.h>

#pragma comment(lib, "json_spirit_lib.lib")

#define RESUME_THREAD_TIME  10 
#define RESUME_THREAD_COUNT 10

using boost::shared_ptr;

using namespace  ::saasame::transport;

enum unique_id_format_type{
    Vendor_Specific  = 0,
    Vendor_Id        = 1,
    EUI64            = 2,
    FCPH_Name        = 3,
    SCSI_Name_String = 8
};

class disk_universal_identify{
public:
    disk_universal_identify(macho::windows::storage::disk& d);
    disk_universal_identify(macho::windows::cluster::disk& d);
    disk_universal_identify(const std::string& uri);
    const disk_universal_identify &operator =(const disk_universal_identify& disk_uri);
    bool operator==(const disk_universal_identify& disk_uri);
    bool operator!=(const disk_universal_identify& disk_uri);
    operator std::string();
    std::string    string(){ return operator std::string(); }
private:
    void copy(const disk_universal_identify& disk_uri);
    std::string                                 _cluster_id;
    std::string                                 _friendly_name;
    std::string                                 _serial_number;
    std::string                                 _address;
    macho::windows::storage::ST_PARTITION_STYLE _partition_style;
    macho::guid_                                _gpt_guid;
    DWORD                                       _mbr_signature;
    std::string                                 _unique_id;
    unique_id_format_type                       _unique_id_format;
};

class MyAccessManager : public apache::thrift::transport::AccessManager
{
    Decision verify(const sockaddr_storage& sa) throw()
    {
        return SKIP;
    }

    Decision verify(const std::string& host, const char* name, int size) throw()
    {
        //if (name)
        //{
        //    std::string cn(name);

        //    if (boost::iequals(cn, std::string("saasame")))
        //        return ALLOW;
        //}
        //return DENY;
        return ALLOW;
    }

    Decision verify(const sockaddr_storage& sa, const char* data, int size) throw()
    {
        return DENY;
    }
};

class common_service_handler : virtual public common_serviceIf {
public:
    common_service_handler() {
        // Your initialization goes here
    }
    virtual void ping(service_info& _return) = 0;
    virtual void get_host_detail(physical_machine_info& _return, const std::string& session_id, const machine_detail_filter::type filter);
    virtual void get_service_list(std::set<service_info> & _return, const std::string& session_id);
    virtual void enumerate_disks(std::set<disk_info> & _return, const enumerate_disk_filter_style::type filter);
    virtual bool verify_carrier(const std::string& carrier, const bool is_ssl);
    virtual void take_xray(std::string& _return);
    virtual void take_xrays(std::string& _return);
    virtual bool create_mutex(const std::string& session, const int16_t timeout);
    virtual bool delete_mutex(const std::string& session);

protected:
    std::string get_xray(int port, const std::string& host = std::string("localhost"));

private:
    void get_host_general_info(physical_machine_info& _info);
    void get_storage_info(physical_machine_info& _info);
    void get_network_info(physical_machine_info& _info);
    void get_cluster_info(physical_machine_info& _info);

    void check_vcbt_driver_status(bool& installed, bool& enabled);
    service_info get_service(int port);
};

const json_spirit::mValue& find_value(const json_spirit::mObject& obj, const std::string& name);
const std::string find_value_string(const json_spirit::mObject& obj, const std::string& name);
const bool find_value_bool(const json_spirit::mObject& obj, const std::string& name, bool default_value = false);
const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value = 0);
const json_spirit::mArray& find_value_array(const json_spirit::mObject& obj, const std::string& name);
const uint64_t find_value_int64(const json_spirit::mObject& obj, const std::string& name, uint64_t default_value = 0);

inline void report_exception(){
    saasame::transport::invalid_operation error;      
    error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
    error.why = "Package was expired.";                                               
    throw error;                                                                      
}

#define VALIDATE

#ifndef _DEBUG

#ifdef CONFIG_BUILD_DATE_TIME

#undef VALIDATE
#define VALIDATE                                                                              \
   do{                                                                                        \
        if ( boost::posix_time::microsec_clock::universal_time() > ( boost::posix_time::time_from_string(std::string(CONFIG_BUILD_DATE_TIME)) + boost::posix_time::hours(24 * 60) ) ) {  \
            LOG(LOG_LEVEL_ERROR, L"Package was expired.");                                    \
            report_exception();                                                               \
                }                                                                                     \
      }while(0)   

#endif

#endif

#endif
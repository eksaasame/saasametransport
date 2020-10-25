#pragma once

#ifndef __CARRIER_SERVICE_HANDLER__
#define __CARRIER_SERVICE_HANDLER__

#include "carrier_service.h"
#include "common_service_handler.h"
#include "common_connection_service_handler.h"
#include "universal_disk_rw.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <boost/thread/mutex.hpp>
#include "..\buildenv.h"
#include "common\jobs_scheduler.hpp"
#include "..\irm_imagex\irm_imagex.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace macho;

using boost::shared_ptr;

using namespace  ::saasame::transport;


struct image_instance
{
    typedef boost::shared_ptr<image_instance> ptr;
    typedef std::vector<ptr> vtr;
    image_instance() : buffer_size(0){}
    std::string                                           req_id;
    std::string                                           image_name;
    std::string                                           base_name;
    std::string                                           session_id;
    uint32_t                                              buffer_size;
    boost::posix_time::ptime                              created_time;
    boost::posix_time::ptime                              updated_time;
    saasame::ironman::imagex::irm_transport_image::ptr    outfile;
    std::set<connection>                                  conns;
};

class carrier_service_handler : virtual public common_connection_service_handler, virtual public carrier_serviceIf {
private:
    class carrier_job : public macho::job{
    public:
        carrier_job() : job(L"carrier_job", L""){}
        virtual void execute(){}
    };
public:
    carrier_service_handler(std::wstring session_id, int workers = 0)
        : _working_path(macho::windows::environment::get_working_directory()), _data_path(macho::windows::environment::get_working_directory()), _scheduler(workers)
    {
        // Your initialization goes here
        //saasame::transport::invalid_operation err;

        if (!load_instances())
        {
            //err.what_op = saasame::transport::error_codes::SAASAME_E_INITIAL_FAIL;
            //err.why = "Failed to load image instances information from db.";
            //throw err;
            LOG(LOG_LEVEL_ERROR, L"Failed to load image instances information from db.");
        }
        else
        {
            clean_timeout_image_instances();
            save_instances();
        }
        macho::job::ptr j(new carrier_job()); 
        j->register_job_to_be_executed_callback_function(boost::bind(&carrier_service_handler::job_to_be_executed_callback, this, _1, _2));
        _scheduler.schedule_job(j, macho::interval_trigger(boost::posix_time::minutes(5)));
        _scheduler.start();
        load_jobs();
    }

    void ping(service_info& _return){
        _return.id = saasame::transport::g_saasame_constants.CARRIER_SERVICE;
        _return.version = boost::str(boost::format("%d.%d.%d.0") % PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
        _return.path = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_working_directory());
    }
    void create(std::string& _return, const std::string& session_id, const create_image_info& image);
    void create_ex(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name, const int64_t size, const int32_t block_size, const std::string& parent, const bool checksum_verify);

    void open(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name);

    void read(std::string& _return, const std::string& session_id, const std::string& image_id, const int64_t start, const int32_t number_of_bytes_to_read) {
        // Your implementation goes here
        printf("read\n");
    }

    int32_t write(const std::string& session_id, const std::string& image_id, const int64_t start, const std::string& buffer, const int32_t number_of_bytes_to_write);
    int32_t write_ex(const std::string& session_id, const std::string& image_id, const int64_t start, const std::string& buffer, const int32_t number_of_bytes_to_write, const bool is_compressed);

    bool close(const std::string& session_id, const std::string& image_id, const bool is_cancel);

    void stop_all();

    bool remove_base_image(const std::string& session_id, const std::set<std::string> & base_images);
    bool remove_snapshot_image(const std::string& session_id, const std::map<std::string, image_map_info> & images);
    bool verify_management(const std::string& management, const int32_t port, const bool is_ssl);
    bool set_buffer_size(const std::string& session_id, const int32_t size);
    bool is_buffer_free(const std::string& session_id, const std::string& image_id);
    bool is_image_replicated(const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& image_name);

private:
    void create(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name, const int64_t size, const int32_t block_size, const bool compressed, const bool checksum, const uint8_t version, const std::string& parent, const bool checksum_verify, const std::string& comment, const bool cdr);
    bool save_instances();
    bool load_instances();
    bool save_contents();
    std::string get_image_id_by_name(const std::string& image_name);
    image_instance::ptr get_image_obj_by_id(const std::string& image_id);
    void clean_timeout_image_instances();
    bool validate_session_by_name(const std::string& image_name, const std::string& session_id);
    bool validate_session_by_id(const std::string& image_id, const std::string& session_id);

    static image_instance::vtr                  _image_instances;
    static macho::windows::critical_section     _instance_cs;
    static macho::windows::critical_section     _op_cs;
    static std::map<std::string, uint32_t>      _session_buffers;
    boost::filesystem::path                     _working_path;
    boost::filesystem::path                     _data_path;

    void load_jobs();
    void suspend_jobs();
    void job_to_be_executed_callback(const macho::trigger::ptr& job, const macho::job_execution_context& ec);
    void wait_jobs_completion();
    bool is_image_uploaded(std::string parent);
    macho::scheduler                            _scheduler;
    macho::windows::critical_section            _lock;
    macho::removeable_job::map                  _scheduled;
};

#endif
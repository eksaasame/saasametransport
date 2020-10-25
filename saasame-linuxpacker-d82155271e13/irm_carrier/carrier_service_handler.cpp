#include "stdafx.h"
#include "macho.h"
#include "carrier_service_handler.h"
#include "management_service.h"
#include "irm_imagex_op.h"
#include "boost\uuid\uuid_generators.hpp"
#include "boost\regex.hpp"
#include "json_spirit.h"
#include <codecvt>
#ifndef IRM_TRANSPORTER
#include "repeat_job.h"
#endif
#include "..\gen-cpp\mgmt_op.h"

#if ( defined(VHDX_DEBUG) && !BOOST_PP_IS_EMPTY(VHDX_DEBUG) )
#define __VHDX_DEBUG
#endif

#ifdef __VHDX_DEBUG

#include "..\irm_host_mgmt\vhdx.h"
typedef std::map<std::string, universal_disk_rw::ptr> universal_disk_rw_map;
static universal_disk_rw_map _vhdx_instances;

#ifndef VIXDISKLIB_SECTOR_SIZE
#define VIXDISKLIB_SECTOR_SIZE 512
#endif

#endif

#if ( defined(CARRIER_DEBUG) && !BOOST_PP_IS_EMPTY(CARRIER_DEBUG) )
#define __CARRIER_DEBUG
#endif

#ifdef __CARRIER_DEBUG
#pragma message("CARRIER debug option enabled")
#endif


#define CARRIER_IMAGE_SERVICE_TIMEOUT           3600   //seconds
#define CARRIER_IMAGE_SERVICE_UPDATE_INTERVAL   60    //seconds
#define IMAGE_VERSION                           IRM_IMAGE_TRANSPORT_MODE
using namespace macho;
using namespace saasame::ironman::imagex;

//#define SAASAME_TRANSPORT_IMAGE_FILE_EXTENSION  ".ivd"

image_instance::vtr carrier_service_handler::_image_instances;
macho::windows::critical_section carrier_service_handler::_instance_cs;
macho::windows::critical_section carrier_service_handler::_op_cs;
std::map<std::string, uint32_t>  carrier_service_handler::_session_buffers;

bool is_valid_uuid(const std::string& str)
{
    boost::uuids::uuid u;
    std::istringstream iss(str);

    iss >> u;
    if (iss.good())
        return true;
    else
        return false;
}

bool carrier_service_handler::save_instances()
{
    FUN_TRACE;
    bool result = true;

    macho::windows::auto_lock lock(_instance_cs);

    result = save_contents();

    return result;
}

bool carrier_service_handler::save_contents()
{
    bool result = true;

#ifdef __VHDX_DEBUG
    try
    {
        boost::filesystem::path image_id_folder = _working_path;
        image_id_folder /= "images";

        boost::filesystem::create_directory(image_id_folder);
        boost::filesystem::path _img_id_file = image_id_folder / boost::str(boost::format("vhdx.run"));

        json_spirit::mObject _instances;
        json_spirit::mArray image_instances;

        foreach(universal_disk_rw_map::value_type &instance, _vhdx_instances)
        {
            json_spirit::mObject _instance;
            _instance["req_id"] = instance.first;
            _instance["image_path"] = macho::stringutils::convert_unicode_to_utf8(instance.second->path());
            image_instances.push_back(_instance);
        }

        _instances["processing_images"] = image_instances;

        std::ofstream output(_img_id_file.string(), std::ios::out | std::ios::trunc);
        std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
        output.imbue(loc);
        json_spirit::write(_instances, output, json_spirit::pretty_print | json_spirit::raw_utf8);
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output running vhdx instances info.")));
        result = false;
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, "can't output running vhdx instances info.");
        result = false;
    }

#endif 

    try
    {
        boost::filesystem::path image_id_folder = _working_path;
        image_id_folder /= "images";

        boost::filesystem::create_directory(image_id_folder);
        boost::filesystem::path _img_id_file = image_id_folder / boost::str(boost::format("images.run"));
        
        json_spirit::mObject _instances;
        json_spirit::mArray image_instances;

        foreach(image_instance::ptr instance, _image_instances)
        {
            json_spirit::mObject _instance;

            _instance["req_id"] = instance->req_id;
            _instance["image_name"] = instance->image_name;
            _instance["base_name"] = instance->base_name;
            _instance["session_id"] = instance->session_id;
            _instance["buffer_size"] = (int)instance->buffer_size;
            _instance["created_time"] = boost::posix_time::to_simple_string(instance->created_time);
            _instance["updated_time"] = boost::posix_time::to_simple_string(instance->updated_time);

            json_spirit::mArray _conns;
            foreach(connection conn, instance->conns)
            {
                json_spirit::mObject _conn;

                _conn["connection_id"] = conn.id;
                _conns.push_back(_conn);
            }

            _instance["connections"] = _conns;
            image_instances.push_back(_instance);
        }
        typedef std::map<std::string, uint32_t> _session_buffers_type;
        json_spirit::mArray session_buffers;

        foreach(_session_buffers_type::value_type s, _session_buffers){
            json_spirit::mObject _buffer;
            _buffer["session"] = s.first;
            _buffer["buffer_size"] = (int)s.second;
            session_buffers.push_back(_buffer);
        }
        _instances["processing_images"] = image_instances;
        _instances["session_buffers"] = session_buffers;

        boost::filesystem::path temp = _img_id_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            json_spirit::write(_instances, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _img_id_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _img_id_file.wstring().c_str(), GetLastError());
            result = false;
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output running image instances info.")));
        result = false;
    }
    catch (...)
    {
        LOG(LOG_LEVEL_ERROR, "can't output running image instances info.");
        result = false;
    }

    return result;
}

bool carrier_service_handler::load_instances()
{
    FUN_TRACE;
    bool result = true;

    macho::windows::auto_lock lock(_instance_cs);

#ifdef __VHDX_DEBUG
    try
    {
        boost::filesystem::path image_id_folder = _working_path;
        image_id_folder /= "images";

        boost::filesystem::path _img_id_file = image_id_folder / boost::str(boost::format("vhdx.run"));

        if (boost::filesystem::exists(_img_id_file) && _image_instances.empty())
        {
            std::ifstream is(_img_id_file.string());
            json_spirit::mValue image_instances;
            json_spirit::read(is, image_instances);

            json_spirit::mArray _instances = find_value(image_instances.get_obj(), "processing_images").get_array();

            foreach(json_spirit::mValue _instance, _instances)
            {
                universal_disk_rw::ptr rw = win_vhdx_mgmt::open_virtual_disk_for_rw(macho::stringutils::convert_utf8_to_unicode(find_value(_instance.get_obj(), "image_path").get_str()));
                if (rw) _vhdx_instances[find_value(_instance.get_obj(), "req_id").get_str()] = rw;               
            }
        }
        else
        {
            LOG(LOG_LEVEL_WARNING, macho::stringutils::convert_ansi_to_unicode("Running vhdx instances db not found."));
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't load running vhdx instances info.")));
        result = false;
    }
    catch (...)
    {
        result = false;
    }

#endif

    try
    {
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)))
        {
            if (reg[L"WorkingPath"].exists() && reg[L"WorkingPath"].is_string())
            {
                std::wstring path = reg[L"WorkingPath"].wstring();
                if (path.length()){
                    if (boost::filesystem::exists(path) && boost::filesystem::is_directory(path))
                        _working_path = path;
                    else if (boost::filesystem::create_directories(path))
                        _working_path = path;
                }
            }
            if (reg[L"DataPath"].exists() && reg[L"DataPath"].is_string())
            {
                std::wstring path = reg[L"DataPath"].wstring();
                if (path.length()){
                    if (boost::filesystem::exists(path) && boost::filesystem::is_directory(path))
                        _data_path = path;
                    else if (boost::filesystem::create_directories(path))
                        _data_path = path;
                }
            }
            if (reg[L"Debug"].exists() && reg[L"Debug"].is_dword())
            {
                boost::this_thread::sleep(boost::posix_time::seconds((DWORD)reg[L"Debug"]));
            }
        }

        boost::filesystem::path image_id_folder = _working_path;
        image_id_folder /= "images";

        boost::filesystem::path _img_id_file = image_id_folder / boost::str(boost::format("images.run"));

        if (boost::filesystem::exists(_img_id_file) && _image_instances.empty())
        {
            std::ifstream is(_img_id_file.string());
            json_spirit::mValue image_instances;
            json_spirit::read(is, image_instances);
            const json_spirit::mObject image_instances_obj = image_instances.get_obj();
            json_spirit::mObject::const_iterator i = image_instances_obj.find("session_buffers");
            if (i != image_instances_obj.end())
            {
                json_spirit::mArray session_buffers = find_value(image_instances_obj, "session_buffers").get_array();
                foreach(json_spirit::mValue buffer, session_buffers)
                {
                    std::string session = find_value_string(buffer.get_obj(), "session");
                    uint32_t buffer_size = find_value_int32(buffer.get_obj(), "buffer_size");
                    _session_buffers[session] = buffer_size;
                }
            }
#if 0
            json_spirit::mArray _instances = find_value(image_instances.get_obj(), "processing_images").get_array();

            foreach(json_spirit::mValue _instance, _instances)
            {
                image_instance::ptr image(new image_instance());

                image->req_id = find_value(_instance.get_obj(), "req_id").get_str();
                image->image_name = find_value(_instance.get_obj(), "image_name").get_str();
                image->base_name = find_value(_instance.get_obj(), "base_name").get_str();
                image->session_id = find_value(_instance.get_obj(), "session_id").get_str();
                image->created_time = boost::posix_time::time_from_string(find_value(_instance.get_obj(), "created_time").get_str());
                //image->updated_time = boost::posix_time::time_from_string(find_value(_instance.get_obj(), "updated_time").get_str());
                image->updated_time = boost::posix_time::microsec_clock::universal_time();

                json_spirit::mArray _conns = find_value(_instance.get_obj(), "connections").get_array();
                foreach(json_spirit::mValue _conn, _conns)
                {
                    std::string conn_id = find_value(_conn.get_obj(), "connection_id").get_str();
                    if (connections_map.count(conn_id))
                    {
                        connection conn = connections_map[conn_id];
                        image->conns.insert(conn);
                    }
                }

                irm_imagex_op::ptr op = irm_imagex_carrier_op::get(image->conns, _working_path, _data_path);
                if (op)
                {
                    macho::windows::registry reg;
                    int memory_buffer_size = IRM_QUEUE_SIZE * 32 * 1024 * 1024;
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)))
                    {
                        if (reg[L"MemoryBufferSize"].exists() && ((DWORD)reg[L"MemoryBufferSize"]) > 0)
                        {
                            memory_buffer_size = (DWORD)reg[L"MemoryBufferSize"];
                        }
                    }
                    irm_transport_image::ptr outfile = irm_transport_image::open(macho::stringutils::convert_utf8_to_unicode(image->base_name), macho::stringutils::convert_utf8_to_unicode(image->image_name), op, memory_buffer_size);
                    image->outfile = outfile;
                    _image_instances.push_back(image);
                }
            }
#endif
        }
        else
        {
            LOG(LOG_LEVEL_WARNING, macho::stringutils::convert_ansi_to_unicode("Running image instances db not found."));
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't load running image instances info.")));
        result = false;
    }
    catch (...)
    {
        result = false;
    }

    return result;
}

std::string carrier_service_handler::get_image_id_by_name(const std::string& image_name)
{
    macho::windows::auto_lock lock(_instance_cs);
    
    std::string image_id = "";

    //find image id
    if (!image_name.empty() && _image_instances.size())
    {
        std::string name = image_name;

        try
        {
            auto it = std::find_if(_image_instances.begin(), _image_instances.end(), [&name](const image_instance::ptr& obj){return obj->image_name == name; });
            if (it != _image_instances.end())
            {
                image_id = (*it)->req_id;
            }
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Query image id error.");
        }
    }

    return image_id;
}

image_instance::ptr carrier_service_handler::get_image_obj_by_id(const std::string& image_id)
{
    macho::windows::auto_lock lock(_instance_cs);

    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    int32_t index = -1;

    //find image id
    if (!image_id.empty() && _image_instances.size())
    {
        std::string id = image_id;

        try
        {
            auto it = std::find_if(_image_instances.begin(), _image_instances.end(), [&id](const image_instance::ptr& obj){return obj->req_id == id; });
            if (it != _image_instances.end())
            {
                index = std::distance(_image_instances.begin(), it);

                boost::posix_time::time_duration duration = update_time - (*it)->updated_time;
                if (duration.total_seconds() > CARRIER_IMAGE_SERVICE_UPDATE_INTERVAL)
                {
                    (*it)->updated_time = update_time;
                    save_contents();
                }

                return _image_instances[index];
            }
            else
                return nullptr;
        }
        catch (...)
        {
            LOG(LOG_LEVEL_ERROR, L"Query image object error.");
            return nullptr;
        }
    }
    else
        return nullptr;
}

bool carrier_service_handler::validate_session_by_name(const std::string& image_name, const std::string& session_id)
{
    bool result = false;

    macho::windows::auto_lock lock(_instance_cs);

    foreach(auto instance, _image_instances)
    {
        if (instance->image_name == image_name && instance->session_id == session_id)
        {
            result = true;
            break;
        }
    }

    return result;
}

bool carrier_service_handler::validate_session_by_id(const std::string& image_id, const std::string& session_id)
{
    bool result = false;

    macho::windows::auto_lock lock(_instance_cs);

    foreach(auto instance, _image_instances)
    {
        if (instance->req_id == image_id && instance->session_id == session_id)
        {
            result = true;
            break;
        }
    }

    return result;
}

void carrier_service_handler::clean_timeout_image_instances()
{
    if (_instance_cs.trylock()){
        int timeout = 0;
        boost::posix_time::ptime current_time = boost::posix_time::microsec_clock::universal_time();

        for (image_instance::vtr::iterator p = _image_instances.begin(); p != _image_instances.end(); p++)
        {
            boost::posix_time::time_duration duration = current_time - (*p)->updated_time;
            if (duration.total_seconds() > CARRIER_IMAGE_SERVICE_TIMEOUT)
            {
                irm_transport_image *image = ((*p)->outfile.get());
                image->flush(true);

                irm_imagex_op::ptr op = irm_imagex_carrier_op::get((*p)->conns, _working_path, _data_path);
                op->release_image_lock(macho::stringutils::convert_utf8_to_unicode((*p)->base_name));

                (*p)->conns.clear();
                timeout++;
            }
        }

        if (timeout)
            LOG(LOG_LEVEL_RECORD, L"Cleaning up stalled image instances...");

        _image_instances.erase(std::remove_if(_image_instances.begin(), _image_instances.end(), [&current_time](const image_instance::ptr &p){ return boost::posix_time::time_duration(current_time - p->updated_time).total_seconds() > CARRIER_IMAGE_SERVICE_TIMEOUT; }), _image_instances.end());
   
        save_contents();
        _instance_cs.unlock();
    }
}

void carrier_service_handler::create(std::string& _return, const std::string& session_id, const std::set<std::string>& connection_ids, const std::string& base_name, const std::string& name, const int64_t size, const int32_t block_size, const bool compressed, const bool checksum, const uint8_t version, const std::string& parent, const bool checksum_verify, const std::string& comment, const bool cdr)
{
    // Your implementation goes here
    VALIDATE;
    FUN_TRACE;
    printf("create\n");

    invalid_operation         err_ex;
    irm_transport_image::ptr  outfile = NULL;
    boost::uuids::uuid        uuid = boost::uuids::random_generator()();
    std::string               image_id = boost::uuids::to_string(uuid);
    std::string               image_name = name; //SAASAME_TRANSPORT_IMAGE_FILE_EXTENSION;
    image_instance::ptr       image_obj = NULL;
    std::set<connection>      conns;

    if (!is_valid_uuid(session_id))
    {
        err_ex.what_op = error_codes::SAASAME_E_FAIL;
        err_ex.why = "Invalid session id";
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
        throw err_ex;
    }

    if (base_name.empty() || name.empty() || connection_ids.empty() || size == 0 || block_size == 0)
    {
        err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err_ex.why = "Invalid arguments";
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
        throw err_ex;
    }

    if (!(_return = get_image_id_by_name(image_name)).empty())
    {
        if (!validate_session_by_id(_return, session_id))
        {
            err_ex.what_op = error_codes::SAASAME_E_FAIL;
            err_ex.why = boost::str(boost::format("Image (%1%) is still in processing by other job.") % image_name);
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
            throw err_ex;
        }
        else
            return;
    }

    foreach(std::string connection_id, connection_ids)
    {
        if (connections_map.count(connection_id))
        {
            connection conn = connections_map[connection_id];
            conns.insert(conn);
        }
        else
        {
            err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
            err_ex.why = boost::str(boost::format("Unknown connection id(%1%).") % connection_id);
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
            throw err_ex;
        }
    }

    try
    {
        irm_imagex_op::ptr op = irm_imagex_carrier_op::get(conns,_working_path,_data_path);
        if (op)
        {
            LOG(LOG_LEVEL_RECORD, L"Accept image (%s/%s:%I64u/%u) create request.", 
                macho::stringutils::convert_utf8_to_unicode(base_name).c_str(),
                macho::stringutils::convert_utf8_to_unicode(image_name).c_str(),
                size, block_size);

            //create
            if (!irm_transport_image::create(
                macho::stringutils::convert_utf8_to_unicode(base_name),
                macho::stringutils::convert_utf8_to_unicode(image_name),
                op,
                size,
                (saasame::ironman::imagex::irm_transport_image_block::block_size)block_size,
                compressed,
                checksum,
                version,
                macho::stringutils::convert_utf8_to_unicode(parent),
                checksum_verify,
                macho::stringutils::convert_utf8_to_unicode(comment),
                cdr))
            {
                err_ex.what_op = error_codes::SAASAME_E_IMAGE_CREATE_FAIL;
                err_ex.why = boost::str(boost::format("Failed to create the virtual disk file (%1%), (error: %2%).") % name % err_ex.what_op);
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                throw err_ex;
            }
            else
            {
                //open
                LOG(LOG_LEVEL_RECORD, L"Accept:open the image (%s) after create successfully.", macho::stringutils::convert_utf8_to_unicode(image_name).c_str());
                macho::windows::registry reg;
                int memory_buffer_size = IRM_QUEUE_SIZE * 32 * 1024 * 1024;
                if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)))
                {
                    if (reg[L"MemoryBufferSize"].exists() && ((DWORD)reg[L"MemoryBufferSize"]) > 0)
                    {
                        memory_buffer_size = (DWORD)reg[L"MemoryBufferSize"];
                    }
                }
                outfile = irm_transport_image::open(macho::stringutils::convert_utf8_to_unicode(base_name), macho::stringutils::convert_utf8_to_unicode(image_name), op, memory_buffer_size);

                if (outfile == NULL)
                {
                    op->remove_file(macho::stringutils::convert_utf8_to_unicode(image_name));

                    err_ex.what_op = error_codes::SAASAME_E_IMAGE_CREATE_FAIL;
                    err_ex.why = boost::str(boost::format("Failed to open the virtual disk file (%1%), (error: %2%).") % name % err_ex.what_op);
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                    op = NULL;
                    throw err_ex;
                }
                else
                {
                    irm_transport_image *image = (outfile.get());
                    //insert new image id and connection id objects
                    image_obj = boost::shared_ptr<image_instance>((new image_instance()));

                    if (image_obj)
                    {
                        image_obj->req_id = image_id;
                        image_obj->image_name = name;
                        image_obj->conns = conns;
                        image_obj->outfile = outfile;
                        image_obj->created_time = boost::posix_time::microsec_clock::universal_time();
                        image_obj->updated_time = image_obj->created_time;
                        image_obj->base_name = base_name;
                        image_obj->session_id = session_id;
                        if (_session_buffers.count(session_id)){
                            image_obj->buffer_size = _session_buffers[session_id];
                            if (image_obj->buffer_size){
                                uint64_t buffer_size_in_bytes = image_obj->buffer_size;
                                buffer_size_in_bytes = buffer_size_in_bytes * 1024 * 1024 * 1024;
                                if (buffer_size_in_bytes < size)
                                    image->queue_size = buffer_size_in_bytes / block_size;
                                else
                                    image->queue_size = 0;
                            }
                        }
                        {
                            macho::windows::auto_lock lock(_instance_cs);

                            _image_instances.push_back(image_obj);

                            if (save_contents())
                                _return = image_id;
                            else
                            {
                                err_ex.what_op = error_codes::SAASAME_E_INTERNAL_FAIL;
                                err_ex.why = boost::str(boost::format("Failed to update image instance db, (error: %1%).") % GetLastError());
                                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                                image->close();
                                outfile = NULL;
                                op = NULL;
                                throw err_ex;
                            }
                        }

                        //initial checksum db
                        image->create_or_load_checksum_db(_working_path);
#ifndef IRM_TRANSPORTER                 
                        repeat_job::vtr jobs = repeat_job::create_jobs(name, parent, conns);
                        if (jobs.size())
                        {
                            macho::windows::auto_lock lock(_lock);
                            foreach(repeat_job::ptr j, jobs)
                            {
                                bool found = false;
                                foreach(macho::removeable_job::map::value_type& _j, _scheduled){
                                    repeat_job* r = dynamic_cast<repeat_job*>(_j.second.get());
                                    if (r->image_name() == name){
                                        foreach(connection c, r->connections()){
                                            foreach(connection _c, j->connections()){
                                                if (c.id == _c.id){
                                                    found = true;
                                                    break;
                                                }
                                            }
                                            if (found)
                                                break;
                                        }
                                    }
                                    if (found)
                                        break;
                                }                               
                                if (!found){
                                    j->register_is_image_uploaded_callback_function(boost::bind(&carrier_service_handler::is_image_uploaded, this, _1));
                                    _scheduler.schedule_job(j, macho::interval_trigger(boost::posix_time::seconds(30)));
                                    _scheduled[j->id()] = j;
                                }
                            }
                        }
#endif
                    }
                    else
                    {
                        err_ex.what_op = error_codes::SAASAME_E_INTERNAL_FAIL;
                        err_ex.why = boost::str(boost::format("Failed to initial internal object instance, (error: %1%).") % GetLastError());
                        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                        image->close();
                        outfile = NULL;
                        op = NULL;
                        throw err_ex;
                    }
                }
            }
        }
        else
        {
            err_ex.what_op = error_codes::SAASAME_E_IMAGE_CREATE_FAIL;
            err_ex.why = "Initial failure";
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
            throw err_ex;
        }

#ifdef __VHDX_DEBUG
        boost::filesystem::path vhdx_output_path = _working_path;
        macho::windows::registry reg;
        if (reg.open(L"Software\\SaaSaMe\\Transport")){
            if (reg[L"VHDXOutputPath"].exists() && reg[L"VHDXOutputPath"].is_string())
                vhdx_output_path = (std::wstring)reg[L"VHDXOutputPath"];
        }
        boost::filesystem::path output_file = vhdx_output_path / name;
        output_file += (".vhdx");
        DWORD err = ERROR_SUCCESS;
        if (parent.length()){
            boost::filesystem::path output_parent_file = vhdx_output_path / parent;
            output_parent_file += (".vhdx");
            vhd_disk_info disk_info;
            if (!(ERROR_SUCCESS == ( err = win_vhdx_mgmt::get_virtual_disk_information(output_parent_file.wstring(), disk_info)) &&
                ERROR_SUCCESS == (err = win_vhdx_mgmt::create_vhdx_file(output_file.wstring(), CREATE_VIRTUAL_DISK_FLAG_NONE, size, disk_info.block_size, disk_info.logical_sector_size, disk_info.physical_sector_size, output_parent_file.wstring())))){
                LOG(LOG_LEVEL_ERROR, L"Faile to create the virtual disk file (%s).", output_file.wstring().c_str()); 
            }                
        }
        else{
            err = win_vhdx_mgmt::create_vhdx_file(output_file.wstring(),
                CREATE_VIRTUAL_DISK_FLAG::CREATE_VIRTUAL_DISK_FLAG_NONE,
                size,
                CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE,
                VIXDISKLIB_SECTOR_SIZE,
                VIXDISKLIB_SECTOR_SIZE);
        }
        if (ERROR_SUCCESS == err){
            universal_disk_rw::ptr rw = win_vhdx_mgmt::open_virtual_disk_for_rw(output_file.wstring());
            if (rw) _vhdx_instances[_return] = rw;
        }
#endif

    }
    catch (...)
    {
        err_ex.what_op = error_codes::SAASAME_E_IMAGE_CREATE_FAIL;
        err_ex.why = "Create failure";
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
        throw err_ex;
    }
}

void carrier_service_handler::create(std::string& _return, const std::string& session_id, const create_image_info& image){
    if (image.version == create_image_option::VERSION_1)
        create(_return, session_id, image.connection_ids, image.base, image.name, image.size, image.block_size, image.compressed, image.checksum, image.mode, image.parent, image.checksum_verify, image.comment, image.cdr);
    else{
        bool compressed = false;
        bool checksum = false;
        foreach(std::string connection_id, image.connection_ids){
            if (connections_map.count(connection_id)){
                connection conn = connections_map[connection_id];
                if (conn.compressed)
                    compressed = true;
                if (conn.checksum)
                    checksum = true;
            }
        }
        create(_return, session_id, image.connection_ids, image.base, image.name, image.size, image.block_size, compressed, checksum, image.mode, image.parent, image.checksum_verify, image.comment, image.cdr);
    }
}

void carrier_service_handler::create_ex(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name, const int64_t size, const int32_t block_size, const std::string& parent, const bool checksum_verify){
    bool compressed = false;
    bool checksum = false;
    foreach(std::string connection_id, connection_ids){
        if (connections_map.count(connection_id)){
            connection conn = connections_map[connection_id];
            if (conn.compressed)
                compressed = true;
            if (conn.checksum)
                checksum = true;
        }
    }
    create(_return, session_id, connection_ids, base_name, name, size, block_size, compressed, checksum, IMAGE_VERSION, parent, checksum_verify, name, false);
}

void carrier_service_handler::open(std::string& _return, const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& base_name, const std::string& name)
{
    // Your implementation goes here
    VALIDATE;
    FUN_TRACE;
    printf("open\n");

    invalid_operation        err_ex;
    irm_transport_image::ptr outfile = nullptr;
    boost::uuids::uuid       uuid = boost::uuids::random_generator()();
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    std::string              image_id = boost::uuids::to_string(uuid);
    std::string              image_name = name; //SAASAME_TRANSPORT_IMAGE_FILE_EXTENSION;
    image_instance::ptr      image_obj = nullptr;
    std::set<connection>     conns;

    if (!is_valid_uuid(session_id))
    {
        err_ex.what_op = error_codes::SAASAME_E_FAIL;
        err_ex.why = "Invalid session id";
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
        throw err_ex;
    }

    if (base_name.empty() || name.empty() || connection_ids.empty())
    {
        err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err_ex.why = "Invalid arguments";
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
        throw err_ex;
    }

    _return = get_image_id_by_name(image_name);
    if(_return.empty())
    {
        LOG(LOG_LEVEL_RECORD, L"Can't find the image from image run db. accept open the image (%s) ...", macho::stringutils::convert_utf8_to_unicode(image_name).c_str());
        foreach(std::string connection_id, connection_ids)
        {
            if (connections_map.count(connection_id))
            {
                connection conn = connections_map[connection_id];
                conns.insert(conn);
            }
            else
            {
                err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
                err_ex.why = boost::str(boost::format("Unknown connection id(%1%).") % connection_id);
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                throw err_ex;
            }
        }
        irm_imagex_op::ptr op = irm_imagex_carrier_op::get(conns, _working_path, _data_path);
        if (!op)
        {
            err_ex.what_op = error_codes::SAASAME_E_IMAGE_OPEN_FAIL;
            err_ex.why = "Initial failure";
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
            throw err_ex;
        }
        else{
            macho::windows::registry reg;
            int memory_buffer_size = IRM_QUEUE_SIZE * 32 * 1024 * 1024;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)))
            {
                if (reg[L"MemoryBufferSize"].exists() && ((DWORD)reg[L"MemoryBufferSize"]) > 0)
                {
                    memory_buffer_size = (DWORD)reg[L"MemoryBufferSize"];
                }
            }
            outfile = irm_transport_image::open(macho::stringutils::convert_utf8_to_unicode(base_name), macho::stringutils::convert_utf8_to_unicode(image_name), op, memory_buffer_size);

            if (outfile == NULL)
            {
                op->remove_file(macho::stringutils::convert_utf8_to_unicode(image_name));
                err_ex.what_op = error_codes::SAASAME_E_IMAGE_OPEN_FAIL;
                err_ex.why = boost::str(boost::format("Failed to open the virtual disk file (%1%), (error: %2%).") % name % err_ex.what_op);
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                op = NULL;
                throw err_ex;
            }
            else
            {
                irm_transport_image *image = (outfile.get());
                //insert new image id and connection id objects
                image_obj = boost::shared_ptr<image_instance>((new image_instance()));

                if (image_obj)
                {
                    image_obj->req_id = image_id;
                    image_obj->image_name = name;
                    image_obj->conns = conns;
                    image_obj->outfile = outfile;
                    image_obj->created_time = boost::posix_time::microsec_clock::universal_time();
                    image_obj->updated_time = image_obj->created_time;
                    image_obj->base_name = base_name;
                    image_obj->session_id = session_id;
                    if (_session_buffers.count(session_id)){
                        image_obj->buffer_size = _session_buffers[session_id];
                        if (image_obj->buffer_size){
                            uint64_t buffer_size_in_bytes = image_obj->buffer_size;
                            buffer_size_in_bytes = buffer_size_in_bytes * 1024 * 1024 * 1024;
                            if (buffer_size_in_bytes < image->total_size)
                                image->queue_size = buffer_size_in_bytes / image->block_size;
                            else
                                image->queue_size = 0;
                        }
                    }
                    {
                        macho::windows::auto_lock lock(_instance_cs);

                        _image_instances.push_back(image_obj);

                        if (save_contents())
                            _return = image_id;
                        else
                        {
                            err_ex.what_op = error_codes::SAASAME_E_INTERNAL_FAIL;
                            err_ex.why = boost::str(boost::format("Failed to update image instance db, (error: %1%).") % GetLastError());
                            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                            image->close();
                            outfile = NULL;
                            op = NULL;
                            throw err_ex;
                        }
                    }

                    //initial checksum db
                    image->create_or_load_checksum_db(_working_path);
                }
                else
                {
                    err_ex.what_op = error_codes::SAASAME_E_INTERNAL_FAIL;
                    err_ex.why = boost::str(boost::format("Failed to initial internal object instance, (error: %1%).") % GetLastError());
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                    image->close();
                    outfile = NULL;
                    op = NULL;
                    throw err_ex;
                }
            }
        }
    }
    else
    {
        if (!validate_session_by_id(_return, session_id))
        {
            err_ex.what_op = error_codes::SAASAME_E_IMAGE_OPEN_FAIL;
            err_ex.why = boost::str(boost::format("Image(%s) not belong to this session id(%s) ") % image_name %session_id);
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
            throw err_ex;
        }

        image_obj = get_image_obj_by_id(_return);

        if (image_obj)
        {
            LOG(LOG_LEVEL_RECORD, L"Accept image (%s) open request.", macho::stringutils::convert_utf8_to_unicode(name).c_str());

            foreach(std::string connection_id, connection_ids)
            {
                auto it = std::find_if(image_obj->conns.begin(), image_obj->conns.end(), [&connection_id](const saasame::transport::connection& obj){return obj.id == connection_id; });
                if (it == image_obj->conns.end())
                {
                    _return = "";
                    err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
                    err_ex.why = boost::str(boost::format("Unknown connection id(%1%).") % connection_id);
                    LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                    throw err_ex;
                }
            }

            image_obj->updated_time = update_time;
            save_instances();

#ifdef __VHDX_DEBUG
            boost::filesystem::path vhdx_output_path = _working_path;
            macho::windows::registry reg;
            if (reg.open(L"Software\\SaaSaMe\\Transport")){
                if (reg[L"VHDXOutputPath"].exists() && reg[L"VHDXOutputPath"].is_string())
                    vhdx_output_path = (std::wstring)reg[L"VHDXOutputPath"];
            }
            boost::filesystem::path output_file = vhdx_output_path / name;
            output_file += (".vhdx");    
            universal_disk_rw::ptr rw = win_vhdx_mgmt::open_virtual_disk_for_rw(output_file.wstring());
            if (rw) _vhdx_instances[_return] = rw;
#endif
        }
        else
        {
            err_ex.what_op = error_codes::SAASAME_E_IMAGE_OPEN_FAIL;
            err_ex.why = "Open failure";
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
            throw err_ex;
        }
    }
}

int32_t carrier_service_handler::write(const std::string& session_id, const std::string& image_id, const int64_t start, const std::string& buffer, const int32_t number_of_bytes_to_write)
{
    // Your implementation goes here
    FUN_TRACE;
    printf("write\n");

    invalid_operation err;
    uint32_t number_of_bytes_written = 0;
    image_instance::ptr image_obj = nullptr;

#ifdef __CARRIER_DEBUG
    LOG(LOG_LEVEL_RECORD, L"Receiving: %s:%I64u:%u", macho::stringutils::convert_utf8_to_unicode(image_id).c_str(), start, number_of_bytes_to_write);
#endif

    image_obj = get_image_obj_by_id(image_id);

    if (!image_obj)
    {
        err.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err.why = boost::str(boost::format("Can't find the image id(%1%)") % image_id);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }

    if (!image_obj->outfile)
    {
        err.what_op = error_codes::SAASAME_E_IMAGE_WRITE;
        err.why = boost::str(boost::format("Failed to initial internal objects for the image id(%1%)") % image_id);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }

    if (!image_obj->outfile->write(start, buffer, number_of_bytes_written))
    {
        err.what_op = error_codes::SAASAME_E_IMAGE_WRITE;
        err.why = boost::str(boost::format("Failed to write contents of the image(%1%), start=(%2%), length=(%3%)") % image_obj->image_name % start % number_of_bytes_to_write);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }
#if 0
    else
        save_instances();
#endif

#ifdef __VHDX_DEBUG
    if (_vhdx_instances.count(image_id) && _vhdx_instances[image_id]){
        uint32_t _number_of_bytes_written;
        _vhdx_instances[image_id]->write(start, buffer, _number_of_bytes_written);
    }
#endif
    return (int32_t)number_of_bytes_written;
}

int32_t carrier_service_handler::write_ex(const std::string& session_id, const std::string& image_id, const int64_t start, const std::string& buffer, const int32_t number_of_bytes_to_write, const bool is_compressed){
    // Your implementation goes here
    FUN_TRACE;
    printf("write\n");

    invalid_operation err;
    uint32_t number_of_bytes_written = 0;
    image_instance::ptr image_obj = nullptr;

#ifdef __CARRIER_DEBUG
    LOG(LOG_LEVEL_RECORD, L"Receiving: %s:%I64u:%u", macho::stringutils::convert_utf8_to_unicode(image_id).c_str(), start, number_of_bytes_to_write);
#endif

    image_obj = get_image_obj_by_id(image_id);

    if (!image_obj)
    {
        err.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err.why = boost::str(boost::format("Can't find the image id(%1%)") % image_id);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }

    if (!image_obj->outfile)
    {
        err.what_op = error_codes::SAASAME_E_IMAGE_WRITE;
        err.why = boost::str(boost::format("Failed to initial internal objects for the image id(%1%)") % image_id);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }

    irm_transport_image *image = (image_obj->outfile.get());
    if ( IRM_IMAGE_TRANSPORT_MODE != image->mode || !image->write_ex(start, buffer, number_of_bytes_to_write, is_compressed))
    {
        err.what_op = error_codes::SAASAME_E_IMAGE_WRITE;
        err.why = boost::str(boost::format("Failed to write contents of the image(%1%), start=(%2%), length=(%3%)") % image_obj->image_name % start % number_of_bytes_to_write);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }
    number_of_bytes_written = number_of_bytes_to_write;
    return (int32_t)number_of_bytes_written;
}

bool carrier_service_handler::close(const std::string& session_id, const std::string& image_id, const bool is_cancel)
{
    // Your implementation goes here
    FUN_TRACE;
    printf("close\n");

    invalid_operation err;
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();

    LOG(LOG_LEVEL_RECORD, L"Accept image id(%s) close request.", macho::stringutils::convert_utf8_to_unicode(image_id).c_str());
    std::string image_name;
    //get image object by image id
    {
        macho::windows::auto_lock lock(_instance_cs);
        auto it = std::find_if(_image_instances.begin(), _image_instances.end(), [&image_id](const image_instance::ptr& obj){return obj->req_id == image_id; });
        if (it != _image_instances.end())
        {
            auto index = std::distance(_image_instances.begin(), it);
            _image_instances[index]->updated_time = update_time;

            if (_image_instances[index]->outfile)
            {
                irm_transport_image *image = (_image_instances[index]->outfile.get());
                image_name = _image_instances[index]->image_name;
                LOG(LOG_LEVEL_RECORD, L"Close image(%s) with cancel flag(%s).", macho::stringutils::convert_utf8_to_unicode(_image_instances[index]->image_name).c_str(), (is_cancel ? L"true" : L"false"));
                image->close(is_cancel);
                image = NULL;
#ifdef __VHDX_DEBUG
                _vhdx_instances.erase(image_id);
#endif
            }

            _image_instances[index]->conns.clear();
            _image_instances.erase(it);
        }
        else
        {
            err.what_op = error_codes::SAASAME_E_INVALID_ARG;
            err.why = boost::str(boost::format("Can't find the image id(%1%) to close") % image_id);
            LOG(LOG_LEVEL_WARNING, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
            throw err;
        }
    }
    if (is_cancel && !image_name.empty()){
        {
            macho::windows::auto_lock lock(_lock);
            _scheduler.remove_group_jobs(macho::stringutils::convert_utf8_to_unicode(image_name));
            macho::removeable_job::map::iterator j = _scheduled.begin();
            while (j != _scheduled.end())
            {
                repeat_job* r = dynamic_cast<repeat_job*>((*j).second.get());
                if (r->image_name() == image_name)
                {
                    r->interrupt();
                    r->cancel();
                    r->remove();
                    _scheduler.remove_job((*j).second->id());
                    j = _scheduled.erase(j);
                }
                else
                {
                    ++j;
                }
            }
        }
    }
    clean_timeout_image_instances();
    save_instances();

    return true;
}

bool carrier_service_handler::is_buffer_free(const std::string& session_id, const std::string& image_id){

    invalid_operation err;
    uint32_t number_of_bytes_written = 0;
    image_instance::ptr image_obj = nullptr;

#ifdef __CARRIER_DEBUG
    LOG(LOG_LEVEL_RECORD, L"Receiving: %s:%I64u:%u", macho::stringutils::convert_utf8_to_unicode(image_id).c_str(), start, number_of_bytes_to_write);
#endif

    image_obj = get_image_obj_by_id(image_id);

    if (!image_obj)
    {
        err.what_op = error_codes::SAASAME_E_INVALID_ARG;
        err.why = boost::str(boost::format("Can't find the image id(%1%)") % image_id);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }

    if (!image_obj->outfile)
    {
        err.what_op = error_codes::SAASAME_E_IMAGE_WRITE;
        err.why = boost::str(boost::format("Failed to initial internal objects for the image id(%1%)") % image_id);
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err.why).c_str());
        throw err;
    }
    irm_transport_image *image = (image_obj->outfile.get());
    return image->mode == IRM_IMAGE_TRANSPORT_MODE || image->is_buffer_free();
}

void carrier_service_handler::stop_all()
{
    FUN_TRACE;
    macho::windows::auto_lock lock(_instance_cs);
    suspend_jobs();
    foreach(image_instance::ptr& image, _image_instances)
    {
        if (image->outfile)
        {
            irm_transport_image *obj = (image->outfile.get());
            if (obj)
            {
                obj->flush(true);
                LOG(LOG_LEVEL_RECORD, L"Flushed (%s) memory data to block file", obj->name.c_str());
            }
            image->outfile = NULL;
        }
    }
    save_contents();
    wait_jobs_completion();
    _scheduler.stop();
}

bool carrier_service_handler::remove_base_image(const std::string& session_id, const std::set<std::string> & base_images)
{
    FUN_TRACE;

    bool result = true;

    if (base_images.size())
    {
        {
            macho::windows::auto_lock lock(_instance_cs);

            foreach(std::string base_img, base_images)
            {
                try
                {
                    LOG(LOG_LEVEL_WARNING, L"Removing snapshot base image (%s)", macho::stringutils::convert_utf8_to_unicode(base_img).c_str());
                    auto it = std::find_if(_image_instances.begin(), _image_instances.end(), [&base_img](const image_instance::ptr& obj){return obj->base_name == base_img; });
                    if (it != _image_instances.end())
                    {
                        LOG(LOG_LEVEL_ERROR, L"Can't remove snapshot base image (%s).", macho::stringutils::convert_utf8_to_unicode(base_img).c_str());
                        result = false;
                        break;
                    }
                }
                catch (...)
                {
                    LOG(LOG_LEVEL_WARNING, L"Encountered an exception. Can't remove snapshot base image (%s).", macho::stringutils::convert_utf8_to_unicode(base_img).c_str());
                    result = false;
                    break;
                }
            }
        }
        if (result)
        {
            foreach(std::string base_img, base_images)
            {
                try
                {
                    boost::filesystem::path r_dir = _working_path / IRM_IMAGE_LOCAL_DB_DIR / base_img;
                    if (boost::filesystem::exists(r_dir))
                        boost::filesystem::remove_all(r_dir);
                    boost::filesystem::path data_dir = _data_path / IRM_IMAGE_LOCAL_DB_DIR / base_img;
                    if (boost::filesystem::exists(data_dir))
                        boost::filesystem::remove_all(data_dir);
                }
                catch (boost::filesystem::filesystem_error &ex)
                {
                    LOG(LOG_LEVEL_ERROR, L"%s, Can't remove snapshot base image (%s).", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str(), macho::stringutils::convert_utf8_to_unicode(base_img).c_str());
                    result = false;
                    break;
                }
            }
        }
    }
    else
    {
        result = false;
        LOG(LOG_LEVEL_ERROR, L"the base image list is empty.");
    }

    return result;
}

bool carrier_service_handler::remove_snapshot_image(const std::string& session_id, const std::map<std::string,image_map_info> & images)
{
    FUN_TRACE;

    bool result = true;
    invalid_operation        err_ex;

    if (images.size())
    {
        std::set<std::string>  base_images;
        typedef std::map<std::string, image_map_info> map_type;
        {
            macho::windows::auto_lock lock(_instance_cs);

            foreach(const map_type::value_type &image_map, images)
            {
                if (image_map.second.image.empty())
                    continue;
                std::string image_name = image_map.second.image;
                base_images.insert(image_map.second.base_image);
                try
                {
                    {
                        macho::windows::auto_lock lock(_lock);
                        _scheduler.remove_group_jobs(macho::stringutils::convert_utf8_to_unicode(image_name));
                        macho::removeable_job::map::iterator j = _scheduled.begin();
                        while (j != _scheduled.end())
                        {
                            repeat_job* r = dynamic_cast<repeat_job*>((*j).second.get());
                            if (r->image_name() == image_name){
                                r->interrupt();
                                r->cancel();
                                r->remove();
                                _scheduler.remove_job((*j).second->id());
                                j = _scheduled.erase(j);
                            }
                            else if (!r->base_name().empty() && r->base_name() == image_map.second.base_image){
                                _scheduler.remove_group_jobs(macho::stringutils::convert_utf8_to_unicode(r->image_name()));
                                r->interrupt();
                                r->cancel();
                                r->remove();
                                _scheduler.remove_job((*j).second->id());
                                j = _scheduled.erase(j);
                            }
                            else{
                                ++j;
                            }
                        }
                    }
                    LOG(LOG_LEVEL_WARNING, L"Removing snapshot image (%s)", macho::stringutils::convert_utf8_to_unicode(image_name).c_str());
                    
                    auto it = std::find_if(_image_instances.begin(), _image_instances.end(), [&image_name](const image_instance::ptr& obj){return obj->image_name == image_name; });
                    if (it != _image_instances.end())
                    {
                        LOG(LOG_LEVEL_ERROR, L"Can't remove snapshot image (%s).", macho::stringutils::convert_utf8_to_unicode(image_name).c_str());
                        result = false;
                        break;
                    }
                }
                catch (...)
                {
                    LOG(LOG_LEVEL_WARNING, L"Encountered an exception. Can't remove snapshot image (%s).", macho::stringutils::convert_utf8_to_unicode(image_name).c_str());
                    result = false;
                    break;
                }
            }
        }

        if (result)
        {
            boost::filesystem::directory_iterator end_itr;

            try
            {
                foreach(const map_type::value_type &image_map, images)
                {
                    if (image_map.second.image.empty())
                        continue;
                    std::set<connection>    conns;
                    foreach(std::string connection_id, image_map.second.connection_ids)
                    {
                        if (connections_map.count(connection_id))
                        {
                            connection conn = connections_map[connection_id];
                            conns.insert(conn);
                        }
                        else
                        {
                            err_ex.what_op = error_codes::SAASAME_E_INVALID_ARG;
                            err_ex.why = boost::str(boost::format("Unknown connection id(%1%).") % connection_id);
                            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(err_ex.why).c_str());
                            throw err_ex;
                        }
                    }

                    irm_imagex_op::ptr op = irm_imagex_carrier_op::get(conns, _working_path, _data_path);
                    if (op){
                        op->release_image_lock(macho::stringutils::convert_utf8_to_unicode(image_map.second.base_image));
                        op->release_image_lock(macho::stringutils::convert_utf8_to_unicode(image_map.second.image));
                        result = op->remove_image(macho::stringutils::convert_utf8_to_unicode(image_map.second.image));
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, L"irm_imagex_op is nullptr. Can't remove snapshot image (%s).", macho::stringutils::convert_utf8_to_unicode(image_map.second.image).c_str());
                        result = false;
                    }
                }
            }
            catch (...)
            {
                LOG(LOG_LEVEL_ERROR, L"Encountered an exception. Can't remove snapshot images.");
                result = false;
            }
        }
        if (result && base_images.size())
            remove_base_image(session_id, base_images);
    }
    else
    {
        result = false;
        LOG(LOG_LEVEL_ERROR, L"the snapshot image list is empty.");
    }

    return result;
}

void carrier_service_handler::load_jobs(){
#ifndef IRM_TRANSPORTER
    macho::windows::auto_lock lock(_lock);
    boost::filesystem::path working_dir = macho::windows::environment::get_working_directory();
    namespace fs = boost::filesystem;
    fs::path job_dir = working_dir / "jobs";
    fs::directory_iterator end_iter;
    typedef std::multimap<std::time_t, fs::path> result_set_t;
    result_set_t result_set;
    if (fs::exists(job_dir) && fs::is_directory(job_dir)){
        for (fs::directory_iterator dir_iter(job_dir); dir_iter != end_iter; ++dir_iter){
            if (fs::is_regular_file(dir_iter->status())){
                if (dir_iter->path().extension() == JOB_EXTENSION){
                    repeat_job::ptr job = repeat_job::load(dir_iter->path(), dir_iter->path().wstring() + L".status");
                    if (job->state() & mgmt_job_state::job_state_finished){
                        job->remove();
                    }
                    else{
                        LOG(LOG_LEVEL_RECORD, L"load job %s.", dir_iter->path().wstring().c_str());
                        job->register_is_image_uploaded_callback_function(boost::bind(&carrier_service_handler::is_image_uploaded, this, _1));
                        if (!_scheduled.count(job->id())){
                            _scheduler.schedule_job(job, macho::interval_trigger(boost::posix_time::minutes(1)));
                            _scheduled[job->id()] = job;
                        }
                    }
                }
            }
        }
    }
#endif
}

void carrier_service_handler::suspend_jobs(){
    macho::windows::auto_lock lock(_lock);
    foreach(macho::removeable_job::map::value_type& j, _scheduled){
        j.second->interrupt();
    }
}

void carrier_service_handler::job_to_be_executed_callback(const macho::trigger::ptr& job, const macho::job_execution_context& ec)
{
    clean_timeout_image_instances();
    save_instances();
    {
#if 0
        macho::windows::auto_lock lock(_lock);
        macho::removeable_job::map::iterator j = _scheduled.begin();
        while (j != _scheduled.end())
        {
            if (!_scheduler.is_running((*j).second->id()))
            {
                repeat_job* r = dynamic_cast<repeat_job*>((*j).second.get());
                if (r->state() & mgmt_job_state::job_state_finished)
                {
                    r->remove();
                    _scheduler.remove_job((*j).second->id());
                    j = _scheduled.erase(j);
                }
                else
                {
                    ++j;
                }
            }
        }
#endif
    }
}

void carrier_service_handler::wait_jobs_completion()
{
    foreach(macho::removeable_job::map::value_type& j, _scheduled)
    {
        while (_scheduler.is_running(j.second->id()))
            boost::this_thread::sleep(boost::posix_time::seconds(2));
    }
}

bool carrier_service_handler::is_image_uploaded(std::string parent)
{
    bool result = true;
    if (parent.length())
    {
#ifndef IRM_TRANSPORTER
        std::wstring wp = macho::stringutils::convert_utf8_to_unicode(parent);
        macho::windows::auto_lock lock(_lock);
        if (_scheduler.has_group_jobs(wp))
        {
            macho::removeable_job::map::iterator j = _scheduled.begin();
            while (j != _scheduled.end())
            {
                repeat_job* r = dynamic_cast<repeat_job*>((*j).second.get());
                if (0 == _wcsicmp(j->second->group().c_str(), wp.c_str()))
                {
                    result = false;
                    if (r->state() & mgmt_job_state::job_state_finished)
                    {
                        result = true;
                    }
                    break;
                }              
                else
                {
                    ++j;
                }
            }
        }
#endif
    }
    return result;
}

bool carrier_service_handler::verify_management(const std::string& management, const int32_t port, const bool is_ssl){
    bool result = false;
    FUN_TRACE;
    mgmt_op op({}, std::string(management), port, is_ssl);
    result = op.open();
    return result;
}

bool carrier_service_handler::set_buffer_size(const std::string& session_id, const int32_t size) {
    macho::windows::auto_lock lock(_instance_cs);
    if (size)
        _session_buffers[session_id] = size;
    else
        _session_buffers.erase(session_id);
    save_contents();
    return true;
}

bool carrier_service_handler::is_image_replicated(const std::string& session_id, const std::set<std::string> & connection_ids, const std::string& image_name){
    return true;
}
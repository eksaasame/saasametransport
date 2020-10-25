#ifdef _AZURE_BLOB
#include "azure_blob_op.h"
#include <was/storage_account.h>
#include <was/blob.h>
#include <cpprest/rawptrstream.h>
#include "..\ntfs_parser\vhdtool.h"
#include <cpprest/filestream.h>  
#include <cpprest/containerstream.h>

using namespace macho;
using namespace macho::windows;

class azure_page_blob_rw : public universal_disk_rw{
public:
    azure_page_blob_rw(std::string connection_string, azure::storage::cloud_page_blob& blob) : _connection_string(connection_string), _blob(blob){
        _out_stream = blob.open_write();
    }
    virtual ~azure_page_blob_rw(){
        _out_stream.flush().wait();
        _out_stream.close().wait();
    }
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written);
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written);
    virtual std::wstring path() const{ return  _blob.uri().primary_uri().to_string(); }
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    virtual universal_disk_rw::ptr clone();
private:
    macho::windows::protected_data        _connection_string;
    azure::storage::cloud_page_blob       _blob;
    concurrency::streams::ostream		  _out_stream;
    concurrency::streams::istream		  _in_stream;
    macho::windows::critical_section      _cs;
};

class azure_page_blob_op_impl : virtual public azure_page_blob_op{
public:
    azure_page_blob_op_impl(std::string connection_string) : _connection_string(connection_string){
    }
    virtual bool                   open();
    virtual bool                   create_vhd_page_blob(std::string container, std::string blob_name, uint64_t size, std::string comment = std::string());
    virtual bool                   delete_vhd_page_blob(std::string container, std::string blob_name);
    virtual bool                   create_vhd_page_blob_snapshot(std::string container, std::string blob_name, std::string snapshot_name);
    virtual bool                   delete_vhd_page_blob_snapshot(std::string container, std::string blob_name, std::string snapshot_name);
    virtual blob_snapshot::vtr     get_vhd_page_blob_snapshots(std::string container, std::string blob_name);
    virtual bool                   create_vhd_page_blob_from_snapshot(std::string container, std::string source_blob_name, std::string snapshot, std::string target_blob_name);
    virtual bool                   change_vhd_blob_id(std::string container, std::string blob_name, macho::guid_ new_id);
    virtual universal_disk_rw::ptr open_vhd_page_blob_for_rw(std::string container, std::string blob_name);
    virtual io_range::vtr		   get_page_ranges(std::string container, std::string blob_name);
    virtual uint64_t               get_vhd_page_blob_size(std::string container, std::string blob_name);
private:
    bool                   _change_vhd_blob_id(azure::storage::cloud_page_blob& blob, uint64_t size, macho::guid_ &id);
    macho::windows::protected_data        _connection_string;
    azure::storage::cloud_storage_account _storage_account;
    azure::storage::cloud_blob_client     _blob_client;
};

azure_page_blob_op::ptr azure_page_blob_op::open(std::string connection_string){
    azure_page_blob_op_impl* op = new azure_page_blob_op_impl(connection_string);
    if (op && op->open())
        return std::move(azure_page_blob_op::ptr(op));
    delete op;
    return NULL;
}

bool                   azure_page_blob_op::create_vhd_page_blob(std::string connection_string, std::string container, std::string blob_name, uint64_t size, std::string comment){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.create_vhd_page_blob(container, blob_name, size, comment);
    return false;
}
bool                   azure_page_blob_op::delete_vhd_page_blob(std::string connection_string, std::string container, std::string blob_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.delete_vhd_page_blob(container, blob_name);
    return false;
}
bool                   azure_page_blob_op::create_vhd_page_blob_snapshot(std::string connection_string, std::string container, std::string blob_name, std::string snapshot_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.create_vhd_page_blob_snapshot(container, blob_name, snapshot_name);
    return false;
}
bool                   azure_page_blob_op::delete_vhd_page_blob_snapshot(std::string connection_string, std::string container, std::string blob_name, std::string snapshot_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.delete_vhd_page_blob_snapshot(container, blob_name, snapshot_name);
    return false;
}

azure_page_blob_op::blob_snapshot::vtr     azure_page_blob_op::get_vhd_page_blob_snapshots(std::string connection_string, std::string container, std::string blob_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.get_vhd_page_blob_snapshots(container, blob_name);
    return blob_snapshot::vtr();
}

bool                   azure_page_blob_op::create_vhd_page_blob_from_snapshot(std::string connection_string, std::string container, std::string source_blob_name, std::string snapshot, std::string target_blob_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.create_vhd_page_blob_from_snapshot(container, source_blob_name, snapshot, target_blob_name);
    return false;
}

bool                   azure_page_blob_op::change_vhd_blob_id(std::string connection_string, std::string container, std::string blob_name, macho::guid_ new_id){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.change_vhd_blob_id(container, blob_name, new_id);
    return false;
}

universal_disk_rw::ptr azure_page_blob_op::open_vhd_page_blob_for_rw(std::string connection_string, std::string container, std::string blob_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.open_vhd_page_blob_for_rw(container, blob_name);
    return NULL;
}

io_range::vtr		  azure_page_blob_op::get_page_ranges(std::string connection_string, std::string container, std::string blob_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.get_page_ranges(container, blob_name);
    return io_range::vtr();
}

uint64_t azure_page_blob_op::get_vhd_page_blob_size(std::string connection_string, std::string container, std::string blob_name){
    azure_page_blob_op_impl op(connection_string);
    if (op.open())
        return op.get_vhd_page_blob_size(container, blob_name);
    return 0;
}

bool azure_page_blob_op_impl::open(){
    bool result = false;
    try{
        _storage_account = azure::storage::cloud_storage_account::parse(macho::stringutils::convert_utf8_to_unicode(_connection_string));
        _blob_client = _storage_account.create_cloud_blob_client();
        result = true;
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool azure_page_blob_op_impl::_change_vhd_blob_id(azure::storage::cloud_page_blob& blob, uint64_t size, macho::guid_ &id){
    bool result = false;
    try{
        std::string footer = vhd_tool::get_fixed_vhd_footer(size, id);
        if (!blob.exists())
            blob.create(size + footer.length());
        else
            blob.resize(size + footer.length());
        blob.metadata()[U("id")] = (std::wstring)id;
        blob.upload_metadata();
        concurrency::streams::ostream out = blob.open_write();
        { // Write VHD footer....
            concurrency::streams::rawptr_buffer<char> buffer(&footer[0], footer.length(), std::ios::in);
            out.seek(size);
            out.write(buffer, footer.length()).wait();
        }
        out.flush().wait();
        out.close().wait();
        result = true;
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool azure_page_blob_op_impl::create_vhd_page_blob(std::string container, std::string blob_name, uint64_t size, std::string comment){
    bool result = false;
    try{
        if (container.empty() || blob_name.empty() || 0 == size){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (!blob_container.exists()){
            blob_container.create();
            blob_container.metadata()[U("comment")] = comment.empty() ? U("N/A") : macho::stringutils::convert_utf8_to_unicode(comment);
            blob_container.upload_metadata();
        }
        azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
        if (!(result = blob.exists())){
            blob.metadata()[U("comment")] = comment.empty() ? U("N/A") : macho::stringutils::convert_utf8_to_unicode(comment);
            blob.metadata()[U("snapshot")] = U("base");
            blob.metadata()[U("id")] = macho::guid_::create();
            result = _change_vhd_blob_id(blob, size, macho::guid_::create());
            if (!result && blob.exists()){
                blob.delete_blob();
            }
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool azure_page_blob_op_impl::delete_vhd_page_blob(std::string container, std::string blob_name){
    bool result = false;
    try{
        if (container.empty() || blob_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if(blob.exists()){
                bool is_empty = true;
                azure::storage::list_blob_item_iterator end_of_results;
                auto it = blob_container.list_blobs(utility::string_t(), true, azure::storage::blob_listing_details::snapshots, 0, azure::storage::blob_request_options(), azure::storage::operation_context());
                for (; it != end_of_results; ++it){
                    if (it->is_blob()){
                        if (it->as_blob().uri().primary_uri() == blob.uri().primary_uri()){
                            if (it->as_blob().is_snapshot()){
                                azure::storage::cloud_page_blob snapshot_blob = blob.container().get_page_blob_reference(it->as_blob().name(), it->as_blob().snapshot_time());
                                snapshot_blob.delete_blob();
                            }
                        }
                        else{
                            is_empty = false;
                        }
                    }
                    else{
                        is_empty = false;
                    }
                }
                blob.delete_blob();
                if (is_empty)
                    blob_container.delete_container();
            }
        }
        result = true;
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool azure_page_blob_op_impl::create_vhd_page_blob_snapshot(std::string container, std::string blob_name, std::string snapshot_name){
    bool result = false;
    try{
        if (container.empty() || blob_name.empty() || snapshot_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        std::wstring _snapshot_name = macho::stringutils::convert_utf8_to_unicode(snapshot_name);
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                azure::storage::list_blob_item_iterator end_of_results;
                auto it = blob.container().list_blobs(utility::string_t(), true, azure::storage::blob_listing_details::snapshots, 0, azure::storage::blob_request_options(), azure::storage::operation_context());
                for (; it != end_of_results; ++it){
                    if (it->is_blob()){
                        if (it->as_blob().is_snapshot() && it->as_blob().uri().primary_uri() == blob.uri().primary_uri()){
                            azure::storage::cloud_page_blob snapshot_blob = blob.container().get_page_blob_reference(it->as_blob().name(), it->as_blob().snapshot_time());
                            snapshot_blob.download_attributes();
                            if ((snapshot_blob.metadata()[U("snapshot")] == _snapshot_name) || (snapshot_blob.metadata()[U("id")] == _snapshot_name)){
                                result = true;
                                break;
                            }
                        }
                    }
                }
                if (!result){
                    blob.download_attributes();
                    azure::storage::cloud_metadata metadata = blob.metadata();
                    metadata[U("snapshot")] = _snapshot_name;
                    metadata[U("id")] = macho::guid_::create(); 
                    azure::storage::cloud_blob snapshot = blob.create_snapshot(metadata, azure::storage::access_condition(), azure::storage::blob_request_options(), azure::storage::operation_context());
                    result = snapshot.exists();
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool azure_page_blob_op_impl::delete_vhd_page_blob_snapshot(std::string container, std::string blob_name, std::string snapshot_name){
    bool result = false;
    try{
        if (container.empty() || blob_name.empty() || snapshot_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        std::wstring _snapshot_name = macho::stringutils::convert_utf8_to_unicode(snapshot_name);
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                result = true;
                azure::storage::list_blob_item_iterator end_of_results;
                auto it = blob.container().list_blobs(utility::string_t(), true, azure::storage::blob_listing_details::snapshots, 0, azure::storage::blob_request_options(), azure::storage::operation_context());
                for (; it != end_of_results; ++it){
                    if (it->is_blob()){
                        if (it->as_blob().is_snapshot() && it->as_blob().uri().primary_uri() == blob.uri().primary_uri()){
                            azure::storage::cloud_page_blob snapshot_blob = blob.container().get_page_blob_reference(it->as_blob().name(), it->as_blob().snapshot_time());
                            snapshot_blob.download_attributes();
                            if (snapshot_blob.metadata()[U("snapshot")] == _snapshot_name || 
                                snapshot_blob.metadata()[U("id")] == _snapshot_name ||
                                snapshot_blob.snapshot_time() == _snapshot_name){
                                result = false;
                                snapshot_blob.delete_blob();
                                result = true;
                                break;
                            }
                        }
                    }
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

azure_page_blob_op::blob_snapshot::vtr    azure_page_blob_op_impl::get_vhd_page_blob_snapshots(std::string container, std::string blob_name){
    blob_snapshot::vtr result;
    try{
        if (container.empty() || blob_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                azure::storage::list_blob_item_iterator end_of_results;
                auto it = blob.container().list_blobs(utility::string_t(), true, azure::storage::blob_listing_details::snapshots, 0, azure::storage::blob_request_options(), azure::storage::operation_context());
                for (; it != end_of_results; ++it){
                    if (it->is_blob()){
                        if (it->as_blob().is_snapshot() && it->as_blob().uri().primary_uri() == blob.uri().primary_uri()){
                            azure::storage::cloud_page_blob snapshot_blob = blob.container().get_page_blob_reference(it->as_blob().name(), it->as_blob().snapshot_time());
                            snapshot_blob.download_attributes();                           
                            blob_snapshot s;
                            s.name = macho::stringutils::convert_unicode_to_utf8(snapshot_blob.metadata()[U("snapshot")]);
                            s.id = macho::stringutils::convert_unicode_to_utf8(snapshot_blob.metadata()[U("id")]);
                            s.date_time = macho::stringutils::convert_unicode_to_utf8(it->as_blob().snapshot_time());
                            result.push_back(s);
                        }
                    }
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return result;
}

bool  azure_page_blob_op_impl::create_vhd_page_blob_from_snapshot(std::string container, std::string source_blob_name, std::string snapshot, std::string target_blob_name){
    bool result = false;
    try{
        if (container.empty() || source_blob_name.empty() || snapshot.empty() || target_blob_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        std::wstring _snapshot_name = macho::stringutils::convert_utf8_to_unicode(snapshot);
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      source_blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(source_blob_name));
            azure::storage::cloud_page_blob      target_blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(target_blob_name));
            if (source_blob.exists()){
                azure::storage::list_blob_item_iterator end_of_results;
                auto it = source_blob.container().list_blobs(utility::string_t(), true, azure::storage::blob_listing_details::snapshots, 0, azure::storage::blob_request_options(), azure::storage::operation_context());
                for (; it != end_of_results; ++it){
                    if (it->is_blob()){
                        if (it->as_blob().is_snapshot() && it->as_blob().uri().primary_uri() == source_blob.uri().primary_uri()){
                            azure::storage::cloud_page_blob snapshot_blob = source_blob.container().get_page_blob_reference(it->as_blob().name(), it->as_blob().snapshot_time());
                            snapshot_blob.download_attributes();
                            if (snapshot_blob.metadata()[U("snapshot")] == _snapshot_name || 
                                snapshot_blob.metadata()[U("id")] == _snapshot_name ||
                                snapshot_blob.snapshot_time() == _snapshot_name){
                                if (!target_blob.exists()){
                                    target_blob.create(snapshot_blob.properties().size());
                                }
                                else{
                                    target_blob.resize(snapshot_blob.properties().size());
                                }
                                utility::string_t copy_id = target_blob.start_copy_from_blob(snapshot_blob);
                                do{
                                    boost::this_thread::sleep(boost::posix_time::seconds(1));
                                } while (target_blob.copy_state().status() == azure::storage::copy_status::pending);
                                result = target_blob.copy_state().status() == azure::storage::copy_status::success;
                                target_blob.download_attributes();
                                _change_vhd_blob_id(target_blob, target_blob.properties().size() - 512, macho::guid_::create());
                                break;
                            }
                        }
                    }
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"source_blob_name: %s does not exit.", macho::stringutils::convert_utf8_to_unicode(source_blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool  azure_page_blob_op_impl::change_vhd_blob_id(std::string container, std::string blob_name, macho::guid_ new_id){
    bool result = false;
    try{
        if (container.empty() || blob_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                blob.download_attributes();
                result = _change_vhd_blob_id(blob, blob.properties().size() - 512, new_id);
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

universal_disk_rw::ptr azure_page_blob_op_impl::open_vhd_page_blob_for_rw(std::string container, std::string blob_name){
    universal_disk_rw::ptr rw;
    try{
        if (container.empty() || blob_name.empty() ){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return rw;
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                rw = universal_disk_rw::ptr(new azure_page_blob_rw(_connection_string, blob));
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return rw;
}

io_range::vtr azure_page_blob_op_impl::get_page_ranges(std::string container, std::string blob_name)  {
    io_range::vtr results;
    try{		
        if (container.empty() || blob_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                std::vector<azure::storage::page_range> ranges = blob.download_page_ranges();
                foreach(azure::storage::page_range &r, ranges){
                    results.push_back(io_range(r.start_offset(), (uint64_t)r.end_offset() - (uint64_t)r.start_offset() + 1));
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return results;
}

uint64_t azure_page_blob_op_impl::get_vhd_page_blob_size(std::string container, std::string blob_name)  {
    uint64_t size = 0;
    try{
        if (container.empty() || blob_name.empty()){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
        }
        azure::storage::cloud_blob_container blob_container = _blob_client.get_container_reference(macho::stringutils::convert_utf8_to_unicode(container));
        if (blob_container.exists()){
            azure::storage::cloud_page_blob      blob = blob_container.get_page_blob_reference(macho::stringutils::convert_utf8_to_unicode(blob_name));
            if (blob.exists()){
                size = blob.properties().size() - 512;
            }
            else{
                LOG(LOG_LEVEL_ERROR, L"blob (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(blob_name).c_str());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"blob_container (%s) does not exit.", macho::stringutils::convert_utf8_to_unicode(container).c_str());
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return size;
}

bool azure_page_blob_rw::write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
    return write(start, buf.c_str(), buf.length(), number_of_bytes_written);
}

bool azure_page_blob_rw::write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written){
    bool result = false;
    try{
        if (NULL == buffer || 0 == number_of_bytes_to_write){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        macho::windows::auto_lock lck(_cs);
        _out_stream.seek(start);
        number_of_bytes_written = 0;
        do{
            uint64_t length = number_of_bytes_to_write - number_of_bytes_written;
            concurrency::streams::rawptr_buffer<char> _buffer(&((char*)buffer)[number_of_bytes_written], length, std::ios::in);
            pplx::task<size_t> task = _out_stream.write(_buffer, length);
            task.wait();
            number_of_bytes_written += task.get();
            _out_stream.flush().wait();
        } while (number_of_bytes_written != number_of_bytes_to_write);
        result = number_of_bytes_written == number_of_bytes_to_write;
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return result;
}

bool azure_page_blob_rw::sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){
    uint32_t number_of_bytes_written = 0;
    if (write(start_sector << 9, buffer, number_of_sectors_to_write << 9, number_of_bytes_written)){
        number_of_sectors_written = number_of_bytes_written >> 9;
        return true;
    }
    return false;
}

bool azure_page_blob_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output){
    output.resize(number_of_bytes_to_read);
    uint32_t number_of_bytes_read = 0;
    if (read(start, number_of_bytes_to_read, (void *)&output[0], number_of_bytes_read)){
        output.resize(number_of_bytes_read);
        return true;
    }
    return false;
}

bool azure_page_blob_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read){
    bool result = false;
    try{
        if (NULL == buffer || 0 == number_of_bytes_to_read){
            LOG(LOG_LEVEL_ERROR, L"Invalid parameter(s)");
            return result;
        }
        macho::windows::auto_lock lck(_cs);
        memset(buffer, 0, number_of_bytes_to_read);
        _in_stream = _blob.open_read();
        _in_stream.seek(start);
        number_of_bytes_read = 0;
        do{
            uint64_t length = number_of_bytes_to_read - number_of_bytes_read;
            concurrency::streams::rawptr_buffer<char> _buffer(&((char*)buffer)[number_of_bytes_read], length, std::ios::out);
            pplx::task<size_t> task = _in_stream.read(_buffer, length);
            task.wait();
            number_of_bytes_read += task.get();
        } while (number_of_bytes_read != number_of_bytes_to_read);
        result = number_of_bytes_read == number_of_bytes_to_read;
        _in_stream.close();
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return result;
}

bool azure_page_blob_rw::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
    uint32_t number_of_bytes_read = 0;
    if (read(start_sector << 9, number_of_sectors_to_read << 9, buffer, number_of_bytes_read)){
        number_of_sectors_read = number_of_bytes_read >> 9;
        return true;
    }
    return false;
}

universal_disk_rw::ptr azure_page_blob_rw::clone(){
    return azure_page_blob_op::open_vhd_page_blob_for_rw(_connection_string,
        macho::stringutils::convert_unicode_to_utf8(_blob.container().name()), 
        macho::stringutils::convert_unicode_to_utf8(_blob.name()));
}

/*
bool azure_upload::erase_free_ranges(const io_range::vtr& frees){
    bool result = true;
    try{
        io_range::vtr _frees = frees;
        uint64_t start = 0;
        std::vector<azure::storage::page_range> ranges = _blob.download_page_ranges();
        std::sort(ranges.begin(), ranges.end(), [](azure::storage::page_range const& lhs, azure::storage::page_range const& rhs){ return lhs.start_offset() < rhs.start_offset(); });
        std::sort(_frees.begin(), _frees.end(), [](io_range const& lhs, io_range const& rhs){ return lhs.start < rhs.start; });
        io_range::vtr::iterator free = _frees.begin();
        foreach(azure::storage::page_range &p, ranges){
            start = p.start_offset();
            if (free != _frees.end()){
                while (free->end < start){
                    free = _frees.erase(free);
                    if (free == _frees.end())
                        break;
                }
                if (free != _frees.end()){
                    if (p.end_offset() > free->end){
                        while (start < p.end_offset()){
                            if (free->end <= p.end_offset()){
                                _blob.clear_pages(free->start, free->end - free->start + 1);
                                LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", free->start, free->end);
                                start = free->end;
                                free++;
                                if (free == _frees.end() || free->start > p.end_offset()){
                                    break;
                                }
                            }
                            else
                                break;
                        } 
                    }
                    else if ( free->start < p.end_offset()){
                        _blob.clear_pages(free->start, free->end - free->start + 1);
                        LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", free->start, free->end);
                    }
                }
            }
        }
    }
    catch (const std::exception &e){
        LOG(LOG_LEVEL_ERROR, L"exception: %s", macho::stringutils::convert_utf8_to_unicode(e.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

*/

#endif
#include "loader_job.h"
#include "common_service_handler.h"
#include "loader_service_handler.h"
#include "management_service.h"
#include <codecvt>
#include "universal_disk_rw.h"
#include "..\irm_imagex\irm_imagex.h"
#include "loader_service.h"
#include "common_service_handler.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <boost/crc.hpp>
#include <openssl/evp.h>
#include "..\gen-cpp\aws_s3.h"
#include "mgmt_op.h"
#include "service_op.h"
#include <thrift/transport/TSSLSocket.h>
#include <boost/algorithm/string/predicate.hpp>
#include "..\irm_host_mgmt\vhdx.h"
#include "..\ntfs_parser\azure_blob_op.h"
#ifdef _VMWARE_VADP
#include "vmware.h"
#include "vmware_ex.h"
#pragma comment(lib, "irm_hypervisor_ex.lib")
using namespace mwdc::ironman::hypervisor_ex;
#endif
using boost::shared_ptr;

using namespace  ::saasame::transport;
using namespace json_spirit;
using namespace macho::windows;
using namespace macho;
#ifndef string_map
typedef std::map<std::string, std::string> string_map;
#endif

#ifndef string_int64_map
typedef std::map<std::string, int64_t> string_int64_map;
#endif

#ifndef string_map_map
typedef std::map<std::string, std::map<std::string, std::string>> string_map_map;
#endif

#ifndef string_set_map
typedef std::map<std::string, std::set<std::string>> string_set_map;
#endif

#ifndef VIXDISKLIB_SECTOR_SIZE
#define VIXDISKLIB_SECTOR_SIZE 512
#endif

#define RetryCount                3
#define JournalRetryCount         100
#define PurggingSize              20
#define TransportModePurggingSize 2
#define NumberOfWorkers           4
#define CONNECTION_POOL           1
#pragma comment(lib, "irm_imagex.lib")

class continues_data_replication_trigger : virtual public interval_trigger{
public:
    continues_data_replication_trigger(boost::posix_time::time_duration repeat_interval = boost::posix_time::seconds(15)) : interval_trigger(repeat_interval){
        _name = _T("continues_data_replication_trigger");
    }
};

struct data_block {
    typedef boost::shared_ptr<data_block> ptr;
    typedef std::vector<ptr> vtr;
    data_block(uint64_t _start = 0) : start(_start), duplicated(false), length(0){}
    uint64_t                start;
    std::vector<uint8_t>    data;
    bool                    duplicated;
    uint64_t                length;
};

class read_imagex{

private:
    static int _decompress(__in const char* source, __inout char* dest, __in int compressed_size, __in int max_decompressed_size){
        return LZ4_decompress_safe(source, dest, compressed_size, max_decompressed_size);
    }

public:
    read_imagex(connection_op::ptr conn, std::string filename, connection_op::ptr cascading, std::string machine_id, std::vector<std::wstring>& parallels, std::vector<std::wstring>& branches) : _conn(conn), _cascading(cascading), _parallels(parallels), _branches(branches){
        _filename = macho::stringutils::convert_ansi_to_unicode(filename);
        _machine_id = macho::stringutils::convert_ansi_to_unicode(machine_id);
        _sz_machine_id = machine_id;
    }
    read_imagex(connection_op::ptr conn, std::wstring filename, connection_op::ptr cascading, const std::wstring& machine_id, std::vector<std::wstring>& parallels, std::vector<std::wstring>& branches) : _conn(conn), _filename(filename), _cascading(cascading), _machine_id(machine_id), _parallels(parallels), _branches(branches){
        _sz_machine_id = macho::stringutils::convert_unicode_to_utf8(machine_id);
    }

    bool clear(){
        if (!_conn->remove_file(_filename))
            return false;
        else if (!_conn->remove_file(_filename + (IRM_IMAGE_JOURNAL_EXT)))
            return false;
        else if (!_conn->remove_file(_filename + L"." + _machine_id))
            return false;
        _conn->remove_file(_filename + L".close." + _machine_id);
        foreach(std::wstring _c, _parallels){
            _conn->remove_file(_filename + L"." + _c);
            _conn->remove_file(_filename + L".close." + _c);
        }
        if (!_cascading){ // p -> c1 -> c2 mode or  p -> t(https) -> c1 -> c2 
            foreach(std::wstring _c, _branches){
                _conn->remove_file(_filename + L"." + _c);
                _conn->remove_file(_filename + L".close." + _c);
            }
        }
        return true;
    }

    saasame::ironman::imagex::irm_transport_image::ptr get_image_info(){
        FUN_TRACE;
        try{
            std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
            macho::windows::lock_able::ptr lock_obj = _conn->get_lock_object(_filename, _sz_machine_id);
            macho::windows::auto_lock lock(*lock_obj);
            uint32_t size;
            saasame::ironman::imagex::irm_transport_image::ptr p = saasame::ironman::imagex::irm_transport_image::ptr(new saasame::ironman::imagex::irm_transport_image());
            if (p && _conn->read_file(_filename, *p.get(),size)){
                return p;
            }
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return NULL;
    }

    saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr get_journal_info(){
        FUN_TRACE;
        try{
            std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
            macho::windows::lock_able::ptr lock_obj = _conn->get_lock_object(_filename + (IRM_IMAGE_JOURNAL_EXT), _sz_machine_id);
            macho::windows::auto_lock lock(*lock_obj);
            uint32_t size;
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr p = saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr(new saasame::ironman::imagex::irm_transport_image_blks_chg_journal());
            if (p && _conn->read_file(_filename + (IRM_IMAGE_JOURNAL_EXT), *p.get(), size)){
                return p;
            }
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return NULL;
    }

    bool is_source_journal_existing(){
        return _conn->is_file_ready(_filename + IRM_IMAGE_JOURNAL_EXT);
    }

    macho::windows::lock_able::ptr get_source_journal_lock(){
        return _conn->get_lock_object(_filename + IRM_IMAGE_JOURNAL_EXT, _sz_machine_id);
    }
    
    bool replicate_image(saasame::ironman::imagex::irm_transport_image::ptr image){
        FUN_TRACE;
        if (_cascading){
            try{
                macho::windows::lock_able::ptr lock_obj = _cascading->get_lock_object(_filename, "c");
                macho::windows::auto_lock lock(*lock_obj);
                return _cascading->write_file(_filename, *image);
            }
            catch (macho::windows::lock_able::exception &ex){
                LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
            }
            catch (const std::exception& ex){
                LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            }
        }
        return true;
    }

    bool replicate_journal(saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal){
        FUN_TRACE;
        bool result = true;      
        try{
            if (journal){
                if ((!_parallels.empty()) || (!_cascading && !_branches.empty())){
                    std::wstring journal_file = _filename + L"." + _machine_id;
                    macho::windows::lock_able::ptr lock_obj = _conn->get_lock_object(journal_file, _sz_machine_id);
                    macho::windows::auto_lock lock(*lock_obj);
                    int count = 100;
                    while (count && (!(result = _conn->write_file(journal_file, *journal))) && (!_conn->is_canceled())){
                        boost::this_thread::sleep(boost::posix_time::seconds(1));
                        count--;
                    }
                }
                if (_cascading){
                    if (result){
                        std::wstring journal_file = _filename + (IRM_IMAGE_JOURNAL_EXT);
                        macho::windows::lock_able::ptr lock_obj = _cascading->get_lock_object(journal_file, "c");
                        macho::windows::auto_lock lock(*lock_obj);
                        int count = JournalRetryCount;
                        while (count && (!(result = _cascading->write_file(journal_file, *journal))) && (!_cascading->is_canceled())){
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                            count--;
                        }
                    }
                    if (result && _cascading->type() == connection_op_type::local){
                        std::wstring journal_file = _filename + L"." + _machine_id;
                        macho::windows::lock_able::ptr lock_obj = _cascading->get_lock_object(journal_file, "c");
                        macho::windows::auto_lock lock(*lock_obj);                    
                        int count = JournalRetryCount;
                        while (count && (!(result = _cascading->write_file(journal_file, *journal))) && (!_cascading->is_canceled())){
                            boost::this_thread::sleep(boost::posix_time::seconds(1));
                            count--;
                        }
                    }
                }
            }
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return result;
    }

    bool close_journal(saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal){
        FUN_TRACE;
        bool result = true;
        try{
            if (journal){
                std::wstring journal_file = _filename + L".close." + _machine_id;
                saasame::ironman::imagex::irm_transport_image_blks_chg_journal _journal = *journal.get();
                _journal.extension.append(IRM_IMAGE_CLOSE_EXT);
                if ((!_parallels.empty()) || (!_cascading && !_branches.empty())){
                    macho::windows::lock_able::ptr lock_obj = _conn->get_lock_object(journal_file, _sz_machine_id);
                    macho::windows::auto_lock lock(*lock_obj);
                    int count = JournalRetryCount;
                    while (count && (!(result = _conn->write_file(journal_file, _journal))) && (!_conn->is_canceled())){
                        boost::this_thread::sleep(boost::posix_time::seconds(1));
                        count--;
                    }
                }
                if (result && _cascading && _cascading->type() == connection_op_type::local){
                    macho::windows::lock_able::ptr lock_obj = _cascading->get_lock_object(journal_file, "c");
                    macho::windows::auto_lock lock(*lock_obj);
                    int count = JournalRetryCount;
                    while (count && (!(result = _cascading->write_file(journal_file, _journal))) && (!_cascading->is_canceled())){
                        boost::this_thread::sleep(boost::posix_time::seconds(1));
                        count--;
                    }
                }
            }
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return result;
    }

    saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr get_cascadings_journal_info(bool is_closed = false){
        FUN_TRACE;
        try{
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr result;
            if (_conn){
                foreach(std::wstring _c, _parallels){
                    std::wstring journal_file = _filename + (is_closed ? L".close." : L".") + _c;
                    macho::windows::lock_able::ptr lock_obj = _conn->get_lock_object(journal_file, _sz_machine_id);
                    macho::windows::auto_lock lock(*lock_obj);
                    uint32_t size;
                    saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr p = saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr(new saasame::ironman::imagex::irm_transport_image_blks_chg_journal());
                    if (p && _conn->read_file(journal_file, *p.get(), size)){
                        result.push_back(p);
                    }
                    else{
                        break;
                    }
                }
                if (_cascading){
                    if (result.size() == _parallels.size())
                        return result;
                }
                else{
                    foreach(std::wstring _c, _branches){
                        std::wstring journal_file = _filename + (is_closed ? L".close." : L".") + _c;
                        macho::windows::lock_able::ptr lock_obj = _conn->get_lock_object(journal_file, _sz_machine_id);
                        macho::windows::auto_lock lock(*lock_obj);
                        uint32_t size;
                        saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr p = saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr(new saasame::ironman::imagex::irm_transport_image_blks_chg_journal());
                        if (p && _conn->read_file(journal_file, *p.get(), size)){
                            result.push_back(p);
                        }
                        else{
                            break;
                        }
                    }
                    if (result.size() == (_parallels.size() + _branches.size()))
                        return result;
                }
            }
            
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr();
    }

    uint64_t get_cascadings_journal_next_block_index(){
        uint64_t index = -1LL;
        if (!_parallels.empty() || (!_cascading && !_branches.empty())){
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr journals = get_cascadings_journal_info();
            if (journals.empty())
                return 0;
            foreach(saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal, journals){
                if (journal->next_block_index < index)
                    index = journal->next_block_index;
            }
            return index;
        }
        return index;
    }

    bool are_cascadings_journals_closed(){
        bool result = true;
        if (!_parallels.empty() || (!_cascading && !_branches.empty())){
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr journals = get_cascadings_journal_info(true);
            if (journals.empty())
                return false;
            foreach(saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal, journals){
                if (!journal->is_closed())
                    result = false;
            }
        }
        return result;
    }

    saasame::ironman::imagex::irm_transport_image_block::ptr get_transport_image_block(std::wstring transport_image_block_name, uint32_t block_size, uint32_t& transport_size, uint32_t version = 1){
        FUN_TRACE;
        try{
            saasame::ironman::imagex::irm_transport_image_block::ptr p = saasame::ironman::imagex::irm_transport_image_block::ptr(new saasame::ironman::imagex::irm_transport_image_block(0, block_size, version));
            if (p && _conn->read_file(transport_image_block_name, *p.get(), transport_size, _cascading)){
                return p;
            }
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return NULL;
    }

    bool replicate_transport_image_block(std::wstring transport_image_block_name, uint32_t& transport_size){
        FUN_TRACE;
        bool result = false;
        std::ifstream ifs;
        if (result = _conn->read_file(transport_image_block_name, ifs)){
            ifs.seekg(0, std::ios::end);
            transport_size = ifs.tellg();
            ifs.seekg(0, std::ios::beg);
            result = _cascading->write_file(transport_image_block_name, ifs);
        }
        return result;
    }

    static bool get_data_blocks(saasame::ironman::imagex::irm_transport_image_block& image_block, uint32_t block_size, data_block::vtr& blocks){
        FUN_TRACE;
        try{
            if (IRM_IMAGE_TRANSPORT_MODE == image_block.mode){
                if (image_block.compressed){
                    uint32_t decompressed_length = image_block.length;
                    std::vector<uint8_t> decompressed_buf;
                    decompressed_buf.resize(decompressed_length);
                    decompressed_length = _decompress((const char *)&image_block.data[0], (char *)&decompressed_buf[0], image_block.data.size(), decompressed_length);
                    data_block::ptr p = data_block::ptr(new data_block(image_block.start));
                    p->length = image_block.length;
                    p->data = std::move(decompressed_buf);
                    blocks.push_back(p);
                }
                else{
                    data_block::ptr p = data_block::ptr(new data_block(image_block.start));
                    p->length = image_block.length;
                    p->data = std::move(image_block.data); 
                    blocks.push_back(p);
                }
            }
            else{
                if (!image_block.duplicated){
                    if (image_block.compressed){
                        uint32_t decompressed_length = block_size;
                        std::vector<uint8_t> decompressed_buf;
                        decompressed_buf.resize(decompressed_length);
                        decompressed_length = _decompress((const char *)&image_block.data[0], (char *)&decompressed_buf[0], image_block.data.size(), block_size);
                        image_block.data = std::move(decompressed_buf);
                        image_block.length = decompressed_length;
                        image_block.compressed = false;
                    }

                    if (strlen((char*)image_block.md5) != 0){
                        uint32_t crc = image_block.crc;
                        uint8_t  md5[16];
                        memcpy(md5, image_block.md5, sizeof(md5));
                        std::stringstream buf(std::ios_base::binary | std::ios_base::out | std::ios_base::in);

                        //reset crc&md5 before calculate crc&md5
                        image_block.crc = 0;
                        memset(image_block.md5, 0, sizeof(image_block.md5));

                        //crc32
                        image_block.save(buf, true);
                        std::string _buf = buf.str();
                        uint32_t new_crc = 0;
                        saasame::ironman::imagex::irm_transport_image_block::calc_crc32_val(_buf, new_crc);
                        image_block.crc = new_crc;

                        //md5
                        std::string digest;
                        saasame::ironman::imagex::irm_transport_image_block::calc_md5_val(_buf, digest);

                        if (!digest.empty())
                            memcpy(image_block.md5, digest.c_str(), sizeof(image_block.md5));
                        else
                            LOG(LOG_LEVEL_ERROR, _T("Failed to calculate md5 value for block(start=%I64u)."), image_block.start);

                        if ((image_block.crc != crc) || (0 != memcmp(md5, image_block.md5, sizeof(md5)))){
                            LOG(LOG_LEVEL_ERROR, _T("The data block file is corrupted."));
                            return false;
                        }
                    }
                }
                uint64_t newBit, oldBit = 0;
                bool     dirty_range = false;
                for (newBit = 0; newBit < (block_size / IRM_IMAGE_SECTOR_SIZE); newBit++){
                    if (image_block.bitmap[newBit >> 3] & (1 << (newBit & 7))){
                        if (!dirty_range){
                            dirty_range = true;
                            oldBit = newBit;
                        }
                    }
                    else {
                        if (dirty_range){
                            uint32_t start = oldBit * IRM_IMAGE_SECTOR_SIZE;
                            uint32_t length = (newBit - oldBit) * IRM_IMAGE_SECTOR_SIZE;
                            data_block::ptr p = data_block::ptr(new data_block(image_block.start + (oldBit * IRM_IMAGE_SECTOR_SIZE)));
                            p->length = length;
                            if (!image_block.duplicated){
                                p->data.resize(length);
                                memcpy(&p->data[0], &image_block.data[start], length);
                            }
                            else{
                                p->duplicated = true;
                            }
                            blocks.push_back(p);
                            dirty_range = false;
                        }
                    }
                }
                if (dirty_range){
                    uint32_t start = oldBit * IRM_IMAGE_SECTOR_SIZE;
                    uint32_t length = (newBit - oldBit) * IRM_IMAGE_SECTOR_SIZE;
                    data_block::ptr p = data_block::ptr(new data_block(image_block.start + (oldBit * IRM_IMAGE_SECTOR_SIZE)));
                    p->length = length;
                    if (!image_block.duplicated){
                        p->data.resize(length);
                        memcpy(&p->data[0], &image_block.data[start], length);
                    }
                    else{
                        p->duplicated = true;
                    }
                    blocks.push_back(p);
                }
            }
            return true;
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
        return false;
    }

private:   
    std::wstring                   _filename;
    connection_op::ptr             _conn;
    connection_op::ptr             _cascading;
    std::wstring                   _machine_id;
    std::string                    _sz_machine_id;
    std::vector<std::wstring>&     _parallels;
    std::vector<std::wstring>&     _branches;
};

struct replication_repository{
    typedef boost::shared_ptr<replication_repository> ptr;
    replication_repository(saasame::ironman::imagex::irm_transport_image& _image, loader_disk& _progress, const std::wstring& _machine_id, std::vector<std::wstring>& _parallels, std::vector<std::wstring>& _branches)
        : terminated(false)
        , purge_data(false)
        , progress(_progress)
        , image(_image)
        , block_size(_image.block_size)
        , has_customized_id(false)
        , machine_id(_machine_id)
        , update_journal_error(false)
        , branches(_branches)
        , parallels(_parallels){}
    macho::windows::critical_section                                    cs;
    block_file::queue                                                   waitting;
    block_file::vtr                                                     processing;
    block_file::queue                                                   error;
    boost::mutex                                                        completion_lock;
    boost::condition_variable                                           completion_cond;
    loader_disk&                                                        progress;
    saasame::ironman::imagex::irm_transport_image&                      image;
    uint32_t                                                            block_size;
    bool                                                                terminated;
    bool                                                                purge_data;
    bool                                                                has_customized_id;
    std::vector<std::wstring>&										    parallels;
    std::vector<std::wstring>&											branches;
    std::wstring                                                        machine_id;
    saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal_db;
    bool                                                                update_journal_error;
};

class replication_task{
public:
    typedef boost::shared_ptr<replication_task> ptr;
    typedef std::vector<ptr> vtr;
    typedef boost::signals2::signal<bool(), macho::_or<bool>> job_is_canceled;
    typedef boost::signals2::signal<void()> job_save_statue;
    typedef boost::signals2::signal<void(loader_disk& progress)> job_save_progress;
    typedef boost::signals2::signal<void(saasame::transport::job_state::type state, int error, record_format& format)> job_record;
    inline void register_job_is_canceled_function(job_is_canceled::slot_type slot){
        is_canceled.connect(slot);
    }
    inline void register_job_save_callback_function(job_save_statue::slot_type slot){
        save_status.connect(slot);
    }
    inline void register_job_save_progress_callback_function(job_save_progress::slot_type slot){
        save_progress.connect(slot);
    }
    inline void register_job_record_callback_function(job_record::slot_type slot){
        record.connect(slot);
    }
    replication_task(connection_op::ptr _in, universal_disk_rw::ptr _out, replication_repository& _repository, uint64_t _size, connection_op::ptr _cascading) : in(_in), out(_out), cascading(_cascading), repository(_repository), size(_size){}
    virtual void                                               replicate();
    virtual bool                                               purge(read_imagex& readx);
protected:
    replication_repository&                                    repository;
    connection_op::ptr                                         in;
    connection_op::ptr                                         cascading;
    universal_disk_rw::ptr                                     out;
    uint64_t                                                   size;
    job_is_canceled                                            is_canceled;
    job_save_statue                                            save_status;
    job_save_progress                                          save_progress;
    job_record                                                 record;
    int                                                        purgging_size;
};

class cascading_task : virtual public replication_task{
public:
    typedef boost::shared_ptr<cascading_task> ptr;
    typedef std::vector<ptr> vtr;
    cascading_task(connection_op::ptr _in, universal_disk_rw::ptr _out, replication_repository& _repository, uint64_t _size, connection_op::ptr _cascading) : replication_task(_in, universal_disk_rw::ptr(), _repository, 0, _cascading){}
    virtual void replicate();
};

bool replication_task::purge(read_imagex& readx){
    bool is_updated = false;
    block_file::ptr bk_file;
#if 0
    std::sort(repository.progress.completed_blocks.begin(), repository.progress.completed_blocks.end(), block_file::compare());
    block_file::queue completed_blocks;
    foreach(block_file::ptr b, repository.progress.completed_blocks){
        bool found = false;
        foreach(block_file::ptr _b, completed_blocks){
            if (b->index == _b->index){
                found = true;
                break;
            }
        }
        if (!found)
            completed_blocks.push_back(b);
    }
    repository.progress.completed_blocks = completed_blocks;
#endif
    std::sort(repository.progress.completed_blocks.begin(), repository.progress.completed_blocks.end(), block_file::compare());
    while (repository.progress.completed_blocks.size() && repository.error.size() == 0){
        if (0 == repository.processing.size() || repository.progress.completed_blocks.front()->index < repository.processing[0]->index){
            bk_file = repository.progress.completed_blocks.front();
            if (IRM_IMAGE_TRANSPORT_MODE == repository.image.mode && (repository.progress.journal_index + 1) != bk_file->index)
                break;
            repository.progress.completed_blocks.pop_front();
            if (repository.purge_data){
                if (IRM_IMAGE_TRANSPORT_MODE == repository.image.mode){
                    repository.progress.purgging.push_back(bk_file);
                }
                else{
                    std::wstring _file = bk_file->name;
                    auto it = std::find_if(repository.progress.purgging.begin(), repository.progress.purgging.end(), [&_file](const block_file::ptr& obj){return obj->name == _file; });
                    if (it != repository.progress.purgging.end()){
                        if (bk_file->index > (*it)->index)
                            (*it)->index = bk_file->index;
                    }
                    else{
                        repository.progress.purgging.push_back(bk_file);
                    }
                }
                std::sort(repository.progress.purgging.begin(), repository.progress.purgging.end(), block_file::compare());
                uint64_t next_block_index = readx.get_cascadings_journal_next_block_index();
                for (block_file::vtr::iterator b = repository.progress.purgging.begin(); b != repository.progress.purgging.end() && repository.progress.purgging.size() > purgging_size;){
                    if (IRM_IMAGE_TRANSPORT_MODE == repository.image.mode){
                        if (next_block_index > (*b)->index && !in->remove_file((*b)->name)){
                            LOG(LOG_LEVEL_ERROR, _T("Failed to remove the data block file (%s)."), (*b)->name.c_str());
                            break;
                        }
                    }
                    else{
                        if (NULL == (*b)->lock)
                            (*b)->lock = in->get_lock_object((*b)->name);
                        try{
                            macho::windows::auto_lock lock(*(*b)->lock);
                            if ((*b)->duplicated)
                                in->remove_file((*b)->name + IRM_IMAGE_DEDUPE_TMP_EXT);
                            if (!in->remove_file((*b)->name)){
                                LOG(LOG_LEVEL_ERROR, _T("Failed to remove the data block file (%s)."), (*b)->name.c_str());
                                break;
                            }
                        }
                        catch (macho::windows::lock_able::exception &ex){
                            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                            break;
                        }
                    }
                    b = repository.progress.purgging.erase(b);
                }
            }
            // Only implement cascading for IRM_IMAGE_TRANSPORT_MODE
            if (IRM_IMAGE_TRANSPORT_MODE == repository.image.mode && repository.journal_db)
                repository.journal_db->next_block_index = bk_file->index + 1;
            if (repository.progress.progress < bk_file->data_end)
                repository.progress.progress = bk_file->data_end;
            repository.progress.journal_index = bk_file->index;
            repository.progress.data += bk_file->data;
            repository.progress.transport_data += bk_file->transport_size;
            if (bk_file->duplicated)
                repository.progress.duplicated_data += bk_file->data;
            is_updated = true;
            save_progress(repository.progress);
            if (bk_file->lock)
                bk_file->lock->unlock();
        }
        else
            break;
    }
    return is_updated;
}

void replication_task::replicate(){ 
    FUN_TRACE;
    bool            result = true;
    block_file::ptr bk_file;
    purgging_size = repository.image.mode == IRM_IMAGE_TRANSPORT_MODE ? TransportModePurggingSize : PurggingSize;
    registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"PurggingSize"].exists() && (DWORD)reg[L"PurggingSize"] > 0){
            purgging_size = (DWORD)reg[L"PurggingSize"];
        }
    }

    read_imagex readx(in, repository.image.name, cascading, repository.machine_id, repository.parallels, repository.branches);
    while (true)
    {
        {
            macho::windows::auto_lock lock(repository.cs);
            if (bk_file){
                if (!result){
                    repository.terminated = true;
                    repository.error.push_back(bk_file);
                    break;
                }
                else{
                    for (block_file::vtr::iterator b = repository.processing.begin(); b != repository.processing.end(); b++){
                        if ((*b)->name == bk_file->name){
                            repository.processing.erase(b);
                            break;
                        }
                    }
                    bool is_updated = false;
                    {
                        macho::windows::auto_lock lock(repository.progress.cs);
                        repository.progress.completed_blocks.push_back(bk_file);
                        is_updated = purge(readx);
                    }
                    if (is_updated){
                        save_status();
                        if (!readx.replicate_journal(repository.journal_db))
                        {
                            LOG(LOG_LEVEL_ERROR, _T("Cannot replicate journal file."));
                            record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate journal file.")));
                            result = false;
                            repository.update_journal_error = true;
                            save_status();
                            break;
                        }
                    }
                }
            }

            if (0 == repository.waitting.size() || repository.terminated || is_canceled())
                break;
            bk_file = repository.waitting.front();
            repository.waitting.pop_front();
            if (IRM_IMAGE_TRANSPORT_MODE == repository.image.mode){
                macho::windows::auto_lock lock(repository.progress.cs);
                foreach(block_file::ptr &b, repository.progress.completed_blocks){
                    if (b->index == bk_file->index){
                        LOG(LOG_LEVEL_INFO, _T("Skip completed block( %I64u)"), bk_file->index);
                        if (0 == repository.waitting.size()){
                            bk_file = NULL;
                            break;
                        }
                        bk_file = repository.waitting.front();
                        repository.waitting.pop_front();
                    }
                    else if (b->index > bk_file->index){
                        LOG(LOG_LEVEL_INFO, _T("b->index > bk_file->index(%I64u > %I64u)"), b->index, bk_file->index);
                        break;
                    }
                }              
                if (!bk_file){
                    if (0 == repository.waitting.size())
                        purge(readx);
                    break;
                }
            }
            repository.processing.push_back(bk_file);
            std::sort(repository.processing.begin(), repository.processing.end(), block_file::compare());
        }
        if (repository.terminated)
            break;
        uint32_t  transport_size = 0;
        saasame::ironman::imagex::irm_transport_image_block::ptr blk = readx.get_transport_image_block(bk_file->name, repository.image.block_size, transport_size, repository.image.mode);
        data_block::vtr blocks;
        if (blk && readx.get_data_blocks(*blk.get(), repository.image.block_size, blocks)){
            bk_file->duplicated = blk->duplicated;
            bk_file->transport_size = transport_size;
            if (blk->duplicated){
                foreach(data_block::ptr &_data, blocks){
                    bk_file->data += _data->length;
                    bk_file->data_end = _data->start + _data->length;
                }
            }
            else{
                foreach(data_block::ptr &_data, blocks){
                    try{
                        uint32_t number_of_bytes_written = 0;
                        if (repository.has_customized_id && size){
                            uint64_t customize_id_offset = size - 20480;
                            if (_data->start < customize_id_offset && (_data->start + _data->data.size()) >= customize_id_offset){
                                uint32_t r = 0;
                                std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                                memset(data.get(), 0, 512);
                                if (out->sector_read((size >> 9) - 40, 1, data.get(), r)){
                                    GUID guid;
                                    memcpy(&guid, &(data.get()[512 - 16]), 16);

                                    memcpy(&(_data->data[customize_id_offset - _data->start - 16]), (char*)&(data.get()[512 - 16]), 16);
                                    LOG(LOG_LEVEL_RECORD, _T("Protected customized id (%s) for disk (%s)."), ((std::wstring)macho::guid_(guid)).c_str(), out->path().c_str());
                                }
                                else{
                                    LOG(LOG_LEVEL_ERROR, _T("Cannot read data from disk (%s). (sector: %I64u)"), out->path().c_str(), (size >> 9) - 40);
                                }
                            }
                        }
                        if (out->write(_data->start, &(_data->data[0]), _data->data.size(), number_of_bytes_written)){
                            bk_file->data += _data->data.size();
                            bk_file->data_end = _data->start + _data->data.size();
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("Cannot replicate data to disk (%s). (start: %I64u, length: %I64u)"), out->path().c_str(), _data->start, _data->data.size());
                            record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate data to disk %1%.") % macho::stringutils::convert_unicode_to_ansi(out->path())));
                            result = false;
                            save_status();
                            break;
                        }
                    }
                    catch (const std::exception& ex){
                        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
                        LOG(LOG_LEVEL_ERROR, _T("Cannot replicate data to disk (%s). (start: %I64u, length: %I64u)"), out->path().c_str(), _data->start, _data->data.size());
                        record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate data to disk %1%.") % macho::stringutils::convert_unicode_to_ansi(out->path())));
                        result = false;
                        save_status();
                        break;
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                        LOG(LOG_LEVEL_ERROR, _T("Cannot replicate data to disk (%s). (start: %I64u, length: %I64u)"), out->path().c_str(), _data->start, _data->data.size());
                        record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate data to disk %1%.") % macho::stringutils::convert_unicode_to_ansi(out->path())));
                        result = false;
                        save_status();
                        break;
                    }
                }
            }
        }
        else{
            result = false;
            LOG(LOG_LEVEL_ERROR, _T("Cannot import data block file (%s)."), bk_file->name.c_str());
            record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot import data block file %1%.") % macho::stringutils::convert_unicode_to_ansi(bk_file->name)));
            save_status();
        }
    }
    repository.completion_cond.notify_one();
}

void cascading_task::replicate(){
    FUN_TRACE;
    bool            result = true;
    block_file::ptr bk_file;
    purgging_size = repository.image.mode == IRM_IMAGE_TRANSPORT_MODE ? TransportModePurggingSize : PurggingSize;
    registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"PurggingSize"].exists() && (DWORD)reg[L"PurggingSize"] > 0){
            purgging_size = (DWORD)reg[L"PurggingSize"];
        }
    }
    read_imagex readx(in, repository.image.name, cascading, repository.machine_id, repository.parallels, repository.branches);
    while (true)
    {
        {
            macho::windows::auto_lock lock(repository.cs);
            if (bk_file){
                if (!result){
                    repository.terminated = true;
                    repository.error.push_back(bk_file);
                    break;
                }
                else{
                    for (block_file::vtr::iterator b = repository.processing.begin(); b != repository.processing.end(); b++){
                        if ((*b)->name == bk_file->name){
                            repository.processing.erase(b);
                            break;
                        }
                    }
                    bool is_updated = false;
                    {
                        macho::windows::auto_lock lock(repository.progress.cs);
                        repository.progress.completed_blocks.push_back(bk_file);
                        is_updated = purge(readx);
                    }
                    if (is_updated){
                        save_status();
                        if (!readx.replicate_journal(repository.journal_db))
                        {
                            LOG(LOG_LEVEL_ERROR, _T("Cannot replicate journal file."));
                            record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate journal file.")));
                            result = false;
                            repository.update_journal_error = true;
                            save_status();
                            break;
                        }
                    }
                }
            }

            if (0 == repository.waitting.size() || repository.terminated || is_canceled())
                break;
            bk_file = repository.waitting.front();
            repository.waitting.pop_front();
            if (IRM_IMAGE_TRANSPORT_MODE == repository.image.mode){
                macho::windows::auto_lock lock(repository.progress.cs);
                foreach(block_file::ptr &b, repository.progress.completed_blocks){
                    if (b->index == bk_file->index){
                        LOG(LOG_LEVEL_INFO, _T("Skip completed block( %I64u)"), bk_file->index);
                        if (0 == repository.waitting.size()){
                            bk_file = NULL;
                            break;
                        }
                        bk_file = repository.waitting.front();
                        repository.waitting.pop_front();
                    }
                    else if (b->index > bk_file->index){
                        LOG(LOG_LEVEL_INFO, _T("b->index > bk_file->index(%I64u > %I64u)"), b->index, bk_file->index);
                        break;
                    }
                }
                if (!bk_file){
                    if (0 == repository.waitting.size())
                        purge(readx);
                    break;
                }
            }
            repository.processing.push_back(bk_file);
            std::sort(repository.processing.begin(), repository.processing.end(), block_file::compare());
        }
        if (repository.terminated)
            break;
        uint32_t  transport_size = 0;
        if (result = readx.replicate_transport_image_block(bk_file->name, transport_size)){
            bk_file->transport_size = transport_size;
        }
        else{
            LOG(LOG_LEVEL_ERROR, _T("Cannot replicate data block file (%s)."), bk_file->name.c_str());
            record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate data block file %1%.") % macho::stringutils::convert_unicode_to_ansi(bk_file->name)));
            save_status();
        }
    }
    repository.completion_cond.notify_one();
}

loader_job::loader_job(loader_service_handler& handler, std::wstring id, saasame::transport::create_job_detail detail) :_handler(handler), _get_loader_job_create_detail_error_count(0), _is_cdr(false), _is_replicating(false), _state(job_state_none), _create_job_detail(detail), _is_initialized(false), _is_interrupted(false), _is_canceled(false), removeable_job(id, macho::guid_(GUID_NULL)), _is_continuous_data_replication(false), _enable_cdr_trigger(false){
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _progress_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.progress") % _id % JOB_EXTENSION);
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"Machine"].exists() && reg[L"Machine"].is_string())
            _machine_id = reg[L"Machine"].string();
    }    
}

loader_job::loader_job(loader_service_handler& handler, saasame::transport::create_job_detail detail) :_handler(handler), _get_loader_job_create_detail_error_count(0), _is_cdr(false), _is_replicating(false), _state(job_state_none), _create_job_detail(detail), _is_initialized(false), _is_interrupted(false), _is_canceled(false), removeable_job(macho::guid_::create(), macho::guid_(GUID_NULL)), _is_continuous_data_replication(false), _enable_cdr_trigger(false){
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _progress_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.progress") % _id % JOB_EXTENSION);
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"Machine"].exists() && reg[L"Machine"].is_string())
            _machine_id = reg[L"Machine"].string();
    }
}

void loader_job::update_create_detail(const saasame::transport::create_job_detail& detail){
    {
        macho::windows::auto_lock lock(_config_lock);
        _create_job_detail = detail;
    }
    save_config();
}

loader_job::ptr loader_job::create(loader_service_handler& handler, std::string id, saasame::transport::create_job_detail detail){
    loader_job::ptr job;
    if (detail.type == saasame::transport::job_type::loader_job_type){
        job = loader_job::ptr(new loader_job(handler, macho::guid_(id), detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
        job->_latest_update_time = job->_created_time;
    }
    return job;
}

loader_job::ptr loader_job::create(loader_service_handler& handler, saasame::transport::create_job_detail detail){
    loader_job::ptr job;
    if (detail.type == saasame::transport::job_type::loader_job_type){
        job = loader_job::ptr(new loader_job(handler, detail));
        job->_created_time = boost::posix_time::microsec_clock::universal_time();
        job->_latest_update_time = job->_created_time;
    }
    return job;
}

loader_job::ptr loader_job::load(loader_service_handler& handler, boost::filesystem::path config_file, boost::filesystem::path status_file){
    loader_job::ptr job;
    std::wstring id;
    saasame::transport::create_job_detail _create_job_detail = load_config(config_file.wstring(), id);
    if (_create_job_detail.type == saasame::transport::job_type::loader_job_type){
        job = loader_job::ptr(new loader_job(handler, id, _create_job_detail));
        job->load_create_job_detail(config_file.wstring());
        job->load_status(status_file.wstring());
    }
    return job;
}

void loader_job::remove(){
    _is_removing = true;
    LOG(LOG_LEVEL_RECORD, L"Remove job : %s", _id.c_str());
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        boost::filesystem::remove(_progress_file);
        boost::filesystem::remove(_config_file.string() + ".cmd");
        if (_loader_job_create_detail.detect_type == disk_detect_type::type::AZURE_BLOB){
#ifdef _AZURE_BLOB
            azure_page_blob_op::ptr op_ptr = azure_page_blob_op::open(_loader_job_create_detail.azure_storage_connection_string);
            if (op_ptr){
                foreach(string_map::value_type &d, _loader_job_create_detail.disks_lun_mapping){
                    op_ptr->delete_vhd_page_blob(_loader_job_create_detail.export_path, d.second);
                }
            }
#endif
        }
        if (_is_initialized) 
            connection_op::remove(_loader_job_create_detail.connection_id);
        _running.unlock();
    }
}

saasame::transport::create_job_detail loader_job::load_config(std::wstring config_file, std::wstring &job_id){
    saasame::transport::create_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        mValue _create_job_detail = find_value(job_config.get_obj(), "create_job_detail").get_obj();
        job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(_create_job_detail.get_obj(), "id").get_str());
        detail.management_id = find_value(_create_job_detail.get_obj(), "management_id").get_str();
        detail.mgmt_port = find_value_int32(_create_job_detail.get_obj(), "mgmt_port", 80);
        detail.is_ssl = find_value_bool(_create_job_detail.get_obj(), "is_ssl");

        mArray addr = find_value(_create_job_detail.get_obj(), "mgmt_addr").get_array();
        foreach(mValue a, addr){
            detail.mgmt_addr.insert(a.get_str());
        }
        detail.type = (saasame::transport::job_type::type)find_value(_create_job_detail.get_obj(), "type").get_int();
        mArray triggers = find_value(_create_job_detail.get_obj(), "triggers").get_array();
        foreach(mValue t, triggers){
            saasame::transport::job_trigger trigger;
            trigger.type = (saasame::transport::job_trigger_type::type)find_value(t.get_obj(), "type").get_int();
            trigger.start = find_value(t.get_obj(), "start").get_str();
            trigger.finish = find_value(t.get_obj(), "finish").get_str();
            trigger.interval = find_value(t.get_obj(), "interval").get_int();
            trigger.duration = find_value(t.get_obj(), "duration").get_int();
            trigger.id = find_value_string(t.get_obj(), "id");
            detail.triggers.push_back(trigger);
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
    return detail;
}

void loader_job::load_create_job_detail(std::wstring config_file){
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        const mObject job_config_obj = job_config.get_obj();
        mObject::const_iterator i = job_config_obj.find("loader_job_create_detail");
        if (i == job_config_obj.end()){
            _is_initialized = false;
        }
        else{
            _is_initialized = true;
            mValue _job_create_detail = i->second;
            _loader_job_create_detail = loader_job_create_detail();
            _loader_job_create_detail.replica_id = find_value_string(_job_create_detail.get_obj(), "replica_id");
            _loader_job_create_detail.connection_id = find_value_string(_job_create_detail.get_obj(), "connection_id");
            _loader_job_create_detail.is_paused = find_value_bool(_job_create_detail.get_obj(), "is_paused");
            mArray disks_lun_mapping = find_value(_job_create_detail.get_obj(), "disks_lun_mapping").get_array();
            foreach(mValue d, disks_lun_mapping){
                _loader_job_create_detail.disks_lun_mapping[find_value_string(d.get_obj(), "disk")] = find_value_string(d.get_obj(), "lun");
            }

            mArray snapshots = find_value(_job_create_detail.get_obj(), "snapshots").get_array();
            foreach(mValue s, snapshots){
                _loader_job_create_detail.snapshots.push_back(s.get_str());
            }

            mArray disks_order = find_value_array(_job_create_detail.get_obj(), "disks_order");
            foreach(mValue s, disks_order){
                _loader_job_create_detail.disks_order.push_back(s.get_str());
            }

            mArray disks_snapshot_mapping = find_value(_job_create_detail.get_obj(), "disks_snapshot_mapping").get_array();
            foreach(mValue s, disks_snapshot_mapping){
                mArray disks = find_value(s.get_obj(), "disks").get_array();
                std::string snapshot_id = find_value_string(s.get_obj(), "snapshot_id");
                foreach(mValue d, disks)
                    _loader_job_create_detail.disks_snapshot_mapping[snapshot_id][find_value_string(d.get_obj(), "disk")] = find_value_string(d.get_obj(), "snapshot");
            }
            _loader_job_create_detail.block_mode_enable = find_value_bool(_job_create_detail.get_obj(), "block_mode_enable");
            _loader_job_create_detail.purge_data = find_value(_job_create_detail.get_obj(), "purge_data").get_bool();
            _loader_job_create_detail.remap = find_value(_job_create_detail.get_obj(), "remap").get_bool();
            _loader_job_create_detail.detect_type = (disk_detect_type::type)find_value_int32(_job_create_detail.get_obj(), "detect_type");
            _loader_job_create_detail.worker_thread_number = find_value_int32(_job_create_detail.get_obj(), "worker_thread_number");
            _loader_job_create_detail.host_name = find_value_string(_job_create_detail.get_obj(), "host_name");
            _loader_job_create_detail.export_disk_type = (virtual_disk_type::type)find_value_int32(_job_create_detail.get_obj(), "export_disk_type", virtual_disk_type::VHD);
            _loader_job_create_detail.export_path = find_value_string(_job_create_detail.get_obj(), "export_path");

            if (_loader_job_create_detail.detect_type == saasame::transport::disk_detect_type::AZURE_BLOB){
                //_loader_job_create_detail.azure_storage_connection_string = find_value_string(_job_create_detail.get_obj(), "azure_storage_connection_string");
                std::string azure_storage_connection_string = find_value_string(_job_create_detail.get_obj(), "ez_azure_storage_connection_string");
                if (!azure_storage_connection_string.empty()){
                    macho::bytes ez_azure_storage_connection_string;
                    ez_azure_storage_connection_string.set(macho::stringutils::convert_utf8_to_unicode(azure_storage_connection_string));
                    macho::bytes u_ez_azure_storage_connection_string = macho::windows::protected_data::unprotect(ez_azure_storage_connection_string, true);
                    if (u_ez_azure_storage_connection_string.length() && u_ez_azure_storage_connection_string.ptr()){
                        _loader_job_create_detail.azure_storage_connection_string = std::string(reinterpret_cast<char const*>(u_ez_azure_storage_connection_string.ptr()), u_ez_azure_storage_connection_string.length());
                    }
                    else{
                        _loader_job_create_detail.azure_storage_connection_string = "";
                    }
                }
            }
            const mObject job_create_detail_obj = _job_create_detail.get_obj();
            mObject::const_iterator x = job_create_detail_obj.find("disks_size_mapping");
            if (x != job_create_detail_obj.end()){
                mArray disks_size_mapping = find_value(_job_create_detail.get_obj(), "disks_size_mapping").get_array();
                foreach(mValue d, disks_size_mapping){
                    _loader_job_create_detail.disks_size_mapping[find_value_string(d.get_obj(), "disk")] = find_value_int32(d.get_obj(), "size");
                }
            }
            _loader_job_create_detail.keep_alive = find_value_bool(_job_create_detail.get_obj(), "keep_alive", true);
            _loader_job_create_detail.is_continuous_data_replication = find_value_bool(_job_create_detail.get_obj(), "is_continuous_data_replication", false);  

            if (_loader_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
                macho::bytes user, password;
                user.set(macho::stringutils::convert_utf8_to_unicode(find_value(_job_create_detail.get_obj(), "u1").get_str()));
                password.set(macho::stringutils::convert_utf8_to_unicode(find_value(_job_create_detail.get_obj(), "u2").get_str()));
                macho::bytes u1 = macho::windows::protected_data::unprotect(user, true);
                macho::bytes u2 = macho::windows::protected_data::unprotect(password, true);

                if (u1.length() && u1.ptr()){
                    _loader_job_create_detail.vmware_connection.username = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
                }
                else{
                    _loader_job_create_detail.vmware_connection.username = "";
                }
                if (u2.length() && u2.ptr()){
                    _loader_job_create_detail.vmware_connection.password = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
                }
                else{
                    _loader_job_create_detail.vmware_connection.password = "";
                }
                _loader_job_create_detail.vmware_connection.host      = find_value_string(_job_create_detail.get_obj(), "host");
                _loader_job_create_detail.vmware_connection.esx       = find_value_string(_job_create_detail.get_obj(), "esx");
                _loader_job_create_detail.vmware_connection.datastore = find_value_string(_job_create_detail.get_obj(), "datastore");
                _loader_job_create_detail.vmware_connection.folder_path = find_value_string(_job_create_detail.get_obj(), "folder_path");
                _loader_job_create_detail.thin_provisioned = find_value_bool(_job_create_detail.get_obj(), "thin_provisioned");
            }

            if (job_create_detail_obj.end() != job_create_detail_obj.find("cascadings")){
                mValue  cascadings = find_value(job_create_detail_obj, "cascadings");
                _loader_job_create_detail.cascadings = load_cascadings(cascadings.get_obj());
            }
            if (_machine_id.empty()){
                mValue _create_job_detail = find_value(job_config.get_obj(), "create_job_detail").get_obj();
                _machine_id = find_value_string(_create_job_detail.get_obj(), "machine_id");
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
}

void loader_job::save_config(json_spirit::mObject& job_config){
    mObject job_detail;
    job_detail["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
    job_detail["type"] = _create_job_detail.type;
    job_detail["management_id"] = _create_job_detail.management_id;
    job_detail["mgmt_port"] = _create_job_detail.mgmt_port;
    job_detail["is_ssl"] = _create_job_detail.is_ssl;
    job_detail["machine_id"] = _machine_id;
    job_detail["group"] = macho::stringutils::convert_unicode_to_utf8(_group);
    mArray  mgmt_addr(_create_job_detail.mgmt_addr.begin(), _create_job_detail.mgmt_addr.end());
    job_detail["mgmt_addr"] = mgmt_addr;
    mArray triggers;
    foreach(saasame::transport::job_trigger &t, _create_job_detail.triggers){
        mObject trigger;
        if (0 == t.start.length())
            t.start = boost::posix_time::to_simple_string(_created_time);
        trigger["type"] = (int)t.type;
        trigger["start"] = t.start;
        trigger["finish"] = t.finish;
        trigger["interval"] = t.interval;
        trigger["duration"] = t.duration;
        trigger["id"] = t.id;
        triggers.push_back(trigger);
    }
    job_detail["triggers"] = triggers;
    job_config["create_job_detail"] = job_detail;

    if (_is_initialized){
        mObject _job_create_detail;
        _job_create_detail["replica_id"] = _loader_job_create_detail.replica_id;
        _job_create_detail["connection_id"] = _loader_job_create_detail.connection_id;
        _job_create_detail["is_paused"] = _loader_job_create_detail.is_paused;
        mArray disks_lun_mapping;
        foreach(string_map::value_type &d, _loader_job_create_detail.disks_lun_mapping){
            mObject disk_lun;
            disk_lun["disk"] = d.first;
            disk_lun["lun"] = d.second;
            disks_lun_mapping.push_back(disk_lun);
        }
        _job_create_detail["disks_lun_mapping"] = disks_lun_mapping;

        mArray  snapshots(_loader_job_create_detail.snapshots.begin(), _loader_job_create_detail.snapshots.end());
        _job_create_detail["snapshots"] = snapshots;

        mArray  disks_order(_loader_job_create_detail.disks_order.begin(), _loader_job_create_detail.disks_order.end());
        _job_create_detail["disks_order"] = disks_order;

        mArray disks_snapshot_mapping;
        foreach(string_map_map::value_type &s, _loader_job_create_detail.disks_snapshot_mapping){
            mObject snapshot;
            mArray disks;
            foreach(string_map::value_type &d, s.second){
                mObject disk;
                disk["disk"] = d.first;
                disk["snapshot"] = d.second;
                disks.push_back(disk);
            }
            snapshot["snapshot_id"] = s.first;
            snapshot["disks"] = disks;
            disks_snapshot_mapping.push_back(snapshot);
        }
        _job_create_detail["disks_snapshot_mapping"] = disks_snapshot_mapping;
        _job_create_detail["block_mode_enable"] = _loader_job_create_detail.block_mode_enable;
        _job_create_detail["purge_data"] = _loader_job_create_detail.purge_data;
        _job_create_detail["remap"] = _loader_job_create_detail.remap;
        _job_create_detail["detect_type"] = _loader_job_create_detail.detect_type;
        _job_create_detail["worker_thread_number"] = _loader_job_create_detail.worker_thread_number;
        _job_create_detail["host_name"] = _loader_job_create_detail.host_name;
        _job_create_detail["export_disk_type"] = _loader_job_create_detail.export_disk_type;
        _job_create_detail["export_path"] = _loader_job_create_detail.export_path;
        //_job_create_detail["azure_storage_connection_string"] = _loader_job_create_detail.azure_storage_connection_string;
        if (!_loader_job_create_detail.azure_storage_connection_string.empty()){
            macho::bytes azure_storage_connection_string;
            azure_storage_connection_string.set((LPBYTE)_loader_job_create_detail.azure_storage_connection_string.c_str(), _loader_job_create_detail.azure_storage_connection_string.length());
            macho::bytes ez_azure_storage_connection_string = macho::windows::protected_data::protect(azure_storage_connection_string, true);
            _job_create_detail["ez_azure_storage_connection_string"] = macho::stringutils::convert_unicode_to_utf8(ez_azure_storage_connection_string.get());;
        }
        mArray disks_size_mapping;
        foreach(string_int64_map::value_type &d, _loader_job_create_detail.disks_size_mapping){
            mObject disk_size;
            disk_size["disk"] = d.first;
            disk_size["size"] = d.second;
            disks_size_mapping.push_back(disk_size);
        }
        _job_create_detail["disks_size_mapping"] = disks_size_mapping;
        _job_create_detail["keep_alive"] = _loader_job_create_detail.keep_alive;
        _job_create_detail["is_continuous_data_replication"] = _loader_job_create_detail.is_continuous_data_replication;
        if (_loader_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
            _job_create_detail["host"] = _loader_job_create_detail.vmware_connection.host;
            _job_create_detail["esx"] = _loader_job_create_detail.vmware_connection.esx;
            _job_create_detail["datastore"] = _loader_job_create_detail.vmware_connection.datastore;
            _job_create_detail["folder_path"] = _loader_job_create_detail.vmware_connection.folder_path;
            macho::bytes user, password;
            user.set((LPBYTE)_loader_job_create_detail.vmware_connection.username.c_str(), _loader_job_create_detail.vmware_connection.username.length());
            password.set((LPBYTE)_loader_job_create_detail.vmware_connection.password.c_str(), _loader_job_create_detail.vmware_connection.password.length());
            macho::bytes u1 = macho::windows::protected_data::protect(user, true);
            macho::bytes u2 = macho::windows::protected_data::protect(password, true);
            _job_create_detail["u1"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
            _job_create_detail["u2"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
            _job_create_detail["thin_provisioned"] = _loader_job_create_detail.thin_provisioned;
        }
        if (!_loader_job_create_detail.cascadings.branches.empty()){
            _job_create_detail["cascadings"] = save_cascadings(_loader_job_create_detail.cascadings);
        }
        if (!_loader_job_create_detail.post_snapshot_script.empty()){
            std::string output_file = _config_file.string() + ".cmd";
            std::ofstream output(output_file, std::ios::out | std::ios::trunc);
            output.write(_loader_job_create_detail.post_snapshot_script.c_str(), _loader_job_create_detail.post_snapshot_script.length());
            output.close();
        }
        else{
            boost::filesystem::remove(_config_file.string() + ".cmd");
        }
        job_config["loader_job_create_detail"] = _job_create_detail;
    }
}

void loader_job::save_config(){
    try{
        using namespace json_spirit;
        mObject job_config;
        save_config(job_config);
        boost::filesystem::path temp = _config_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _config_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _config_file.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info.")).c_str());
    }
    catch (...){
    }
}

void loader_job::record(saasame::transport::job_state::type state, int error, record_format& format){
    macho::windows::auto_lock lock(_cs);
    if (_histories.size() == 0 || _histories[_histories.size() - 1]->description != format.str()){
        _histories.push_back(history_record::ptr(new history_record(state, error, format, error ? true : !_is_cdr)));
        LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), L"(%s)%s", _id.c_str(), macho::stringutils::convert_utf8_to_unicode(format.str()).c_str());
    }
    while (_histories.size() > _MAX_HISTORY_RECORDS)
        _histories.erase(_histories.begin());
}

bool loader_job::get_loader_job_create_detail(saasame::transport::loader_job_create_detail &detail){
    bool result = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    int retry = 3;
    while (!result){
        mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
        if (op.open()){
            try{
                int retry_count = 3;
                do{
                    FUN_TRACE_MSG(boost::str(boost::wformat(L"get_loader_job_create_detail(%s)") % _id));
                    saasame::transport::loader_job_create_detail _detail;
                    op.client()->get_loader_job_create_detail(_detail, _machine_id, macho::stringutils::convert_unicode_to_ansi(_id));
                    result = true;
                    foreach(const std::string& snap, _detail.snapshots){
                        if (_detail.disks_lun_mapping.size() != _detail.disks_snapshot_mapping[snap].size()){
                            result = false;
                            retry_count--;
                            LOG(LOG_LEVEL_ERROR, L"Invalid job spec. Number of disks is not equal to number of disk snapshots (%s).", macho::stringutils::convert_utf8_to_unicode(snap).c_str());
                            boost::this_thread::sleep(boost::posix_time::seconds(3));
                            break;
                        }
                    }
                    if (result)
                        detail = _detail;
                } while (!result && retry_count > 0);
#ifdef _DEBUG
                /* 
                detail.detect_type = saasame::transport::disk_detect_type::AZURE_BLOB;
                detail.export_path = macho::stringutils::convert_unicode_to_utf8(_id);
                detail.azure_storage_connection_string = "UseDevelopmentStorage=true;";
                foreach(string_map::value_type &d, detail.disks_lun_mapping){
                    d.second = d.first + ".vhd";
                }
                */
                /*
                detail.detect_type = saasame::transport::disk_detect_type::VMWARE_VADP;
                detail.vmware_connection.host = "192.168.31.124";
                detail.vmware_connection.esx = "esx124";
                detail.vmware_connection.username = "root";
                detail.vmware_connection.password = "abc@123";
                detail.vmware_connection.datastore = "datastore1";
                foreach(string_map::value_type &d, detail.disks_lun_mapping){
                    d.second = d.first + ".vmdk";
                }
                */
#endif
                break;
            }
            catch (saasame::transport::invalid_operation& op){
                LOG(LOG_LEVEL_ERROR, L"Invalid operation (0x%08x): %s", op.what_op, macho::stringutils::convert_ansi_to_unicode(op.why).c_str());
                if (op.what_op == saasame::transport::error_codes::SAASAME_E_JOB_NOTFOUND){
                    _is_removing = true;
                    break;
                }
            }
            catch (apache::thrift::TException & tx){
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
            }
            catch (std::exception &ex){
                LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
            }
        }
        retry--;
        if (result || 0 == retry)
            break;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
    return result;
}

bool loader_job::update_state(const saasame::transport::loader_job_detail &detail){
    bool result = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    LOG(LOG_LEVEL_INFO, _T("Update status for snapshot(%s) to management...."), macho::stringutils::convert_ansi_to_unicode(_current_snapshot).c_str());
    int retry = 3;
    while (!result){
        mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
        if (op.open()){
            try{
                op.client()->update_loader_job_state(_machine_id, detail);
                result = true;
                break;
            }
            catch (apache::thrift::TException & tx){
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
            }
        }
        retry--;
        if (0 == retry)
            break;
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
    return result;
}

bool loader_job::take_snapshot(std::string snapshot_id){
    bool result = false;
    bool is_connected = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (is_connected = op.open()){
        try{
            result = op.client()->take_snapshots(_machine_id, snapshot_id);
        }
        catch (apache::thrift::TException & tx){
            is_connected = false;
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    if (!is_connected){
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot connect to the management server to take snapshot."));
        BOOST_THROW_LOADER_EXCEPTION(L"Cannot connect to the management server to take snapshot.");
    }
    return result;
}

bool loader_job::discard_snapshot(std::string snapshot_id){
    bool result = false;
    bool is_connected = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (is_connected = op.open()){
        try{
            result = op.client()->discard_snapshots(_machine_id, snapshot_id);
        }
        catch (apache::thrift::TException & tx){
            is_connected = false;
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
 
    if (!is_connected){
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot connect to the management server to discard snapshot."));
        BOOST_THROW_LOADER_EXCEPTION(L"Cannot connect to the management server to discard snapshot.");
    }
    return result;
}

bool loader_job::check_snapshot(std::string snapshot_id){
    bool result = false;
    bool is_connected = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (is_connected = op.open()){
        try{
            result = op.client()->check_snapshots(_machine_id, snapshot_id);
        }
        catch (apache::thrift::TException & tx){
            is_connected = false;
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }

    if (!is_connected){
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot connect to the management server to check snapshot."));
        BOOST_THROW_LOADER_EXCEPTION(L"Cannot connect to the management server to check snapshot.");
    }
    return result;
}

bool loader_job::mount_devices(){
    bool result = false;
    bool is_connected = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (is_connected = op.open()){
        try{
            result = op.client()->mount_loader_job_devices(_machine_id, macho::stringutils::convert_unicode_to_ansi(_id));
        }
        catch (apache::thrift::TException & tx){
            is_connected = false;
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    if (!is_connected){
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot connect to the management server to mount devices."));
        BOOST_THROW_LOADER_EXCEPTION(L"Cannot connect to the management server to mount devices.");
    }
    return result;
}

bool loader_job::dismount_devices(){
    bool result = false;
    bool is_connected = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (is_connected = op.open()){
        try{
            result = op.client()->dismount_loader_job_devices(_machine_id, macho::stringutils::convert_unicode_to_ansi(_id));
        }
        catch (apache::thrift::TException & tx){
            is_connected = false;
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    if (!is_connected){
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot connect to the management server to dismount devices."));
        BOOST_THROW_LOADER_EXCEPTION(L"Cannot connect to the management server to dismount devices.");
    }
    return result;
}

bool loader_job::is_devices_ready(){
    bool result = false;
    bool is_connected = false;
    FUN_TRACE;
    int port;
    std::set<std::string> mgmt_addrs;
    {
        macho::windows::auto_lock lock(_config_lock);
        port = _create_job_detail.mgmt_port;
        mgmt_addrs = _create_job_detail.mgmt_addr;
    }
    mgmt_op op(mgmt_addrs, _mgmt_addr, port, _create_job_detail.is_ssl);
    if (is_connected = op.open()){
        try{
            result = op.client()->is_loader_job_devices_ready(_machine_id, macho::stringutils::convert_unicode_to_ansi(_id));
        }
        catch (apache::thrift::TException & tx){
            is_connected = false;
            LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        }
    }
    if (!is_connected){
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot connect to the management server to check device status."));
        BOOST_THROW_LOADER_EXCEPTION(L"Cannot connect to the management server to check device status.");
    }
    return result;
}

void loader_job:: save_status(){
    try{
        macho::windows::auto_lock lock(_cs);
        mObject job_status;
        job_status["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["is_initialized"] = _is_initialized;
        job_status["state"] = _state;
        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);
        job_status["latest_enter_time"] = boost::posix_time::to_simple_string(_latest_enter_time);
        job_status["latest_leave_time"] = boost::posix_time::to_simple_string(_latest_leave_time);
        job_status["mgmt_addr"] = _mgmt_addr;
        mArray histories;
        foreach(history_record::ptr &h, _histories){
            mObject o;
            o["time"] = boost::posix_time::to_simple_string(h->time);
            o["state"] = (int)h->state;
            o["error"] = h->error;
            o["description"] = h->description;
            o["format"] = h->format;
            o["is_display"] = h->is_display;
            mArray  args(h->args.begin(), h->args.end());
            o["arguments"] = args;
            histories.push_back(o);
        }
        job_status["histories"] = histories;

        mArray progress_map;
        foreach(loader_disk::map::value_type& l, _progress_map){
            macho::windows::auto_lock _lock(l.second->cs);
            mObject o;
            o["key"] = l.first;
            o["completed"] = l.second->completed;
            o["canceled"] = l.second->canceled;
            o["journal_index"] = l.second->journal_index;
            o["progress"] = l.second->progress;         
            o["data"] = l.second->data;
            o["duplicated_data"] = l.second->duplicated_data;
            o["transport_data"] = l.second->transport_data;
            o["cdr"] = l.second->cdr;

            mArray purgging;
            for (block_file::vtr::iterator b = l.second->purgging.begin(); b != l.second->purgging.end(); b++){
                mObject _b;
                _b["name"] = macho::stringutils::convert_unicode_to_utf8((*b)->name);
                _b["data"] = (int)(*b)->data;
                _b["duplicated"] = (*b)->duplicated;
                _b["data_end"] = (uint64_t)(*b)->data_end;
                _b["index"] = (*b)->index;
                _b["transport_size"] = (int)(*b)->transport_size;
                purgging.push_back(_b);
            }
            o["purgging"] = purgging;

            mArray completed_blocks;
            foreach (block_file::ptr &b ,l.second->completed_blocks){
                mObject _b;
                _b["name"] = macho::stringutils::convert_unicode_to_utf8(b->name);
                _b["data"] = (int)b->data;
                _b["duplicated"] = b->duplicated;
                _b["data_end"] = (uint64_t)b->data_end;
                _b["index"] = b->index;
                _b["transport_size"] = (int)b->transport_size;
                completed_blocks.push_back(_b);
            }
            o["completed_blocks"] = completed_blocks;
            progress_map.push_back(o);
        }
        job_status["progress_map"] = progress_map;
        job_status["current_snapshot"] = _current_snapshot;
        job_status["latest_update_time"] = boost::posix_time::to_simple_string(_latest_update_time);

        boost::filesystem::path temp = _status_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_status, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _status_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _status_file.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output status info.")));
    }
    catch (...){
    }
}

void loader_job::save_progress(loader_disk& progress){
    try{
        macho::windows::auto_lock lock(progress.cs);
        mObject o;
        o["key"] = progress.key;
        o["completed"] = progress.completed;
        o["canceled"] = progress.canceled;
        o["journal_index"] = progress.journal_index;
        o["progress"] = progress.progress;
        o["data"] = progress.data;
        o["duplicated_data"] = progress.duplicated_data;
        o["transport_data"] = progress.transport_data;
        o["cdr"] = progress.cdr;

        mArray purgging;
        for (block_file::vtr::iterator b = progress.purgging.begin(); b != progress.purgging.end(); b++){
            mObject _b;
            _b["name"] = macho::stringutils::convert_unicode_to_utf8((*b)->name);
            _b["data"] = (int)(*b)->data;
            _b["duplicated"] = (*b)->duplicated;
            _b["data_end"] = (uint64_t)(*b)->data_end;
            _b["index"] = (*b)->index;
            _b["transport_size"] = (int)(*b)->transport_size;
            purgging.push_back(_b);
        }
        o["purgging"] = purgging;

        mArray completed_blocks;
        foreach(block_file::ptr &b, progress.completed_blocks){
            mObject _b;
            _b["name"] = macho::stringutils::convert_unicode_to_utf8(b->name);
            _b["data"] = (int)b->data;
            _b["duplicated"] = b->duplicated;
            _b["data_end"] = (uint64_t)b->data_end;
            _b["index"] = b->index;
            _b["transport_size"] = (int)b->transport_size;
            completed_blocks.push_back(_b);
        }
        o["completed_blocks"] = completed_blocks;

        boost::filesystem::path temp = _progress_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(o, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _progress_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _progress_file.wstring().c_str(), GetLastError());
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output progress info.")));
    }
    catch (...){
    }
}

bool loader_job::load_status(std::wstring status_file){
    FUN_TRACE;
    try{
        if (boost::filesystem::exists(status_file)){
            std::ifstream is(status_file);
            mValue job_status;
            read(is, job_status);
            //_is_interrupted = find_value(job_status.get_obj(), "is_interrupted").get_bool();
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
            if (job_status.get_obj().count("_is_initialized"))
                _is_initialized = find_value(job_status.get_obj(), "_is_initialized").get_bool();
            _state = find_value(job_status.get_obj(), "state").get_int();
            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            _latest_enter_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "latest_enter_time").get_str());
            std::string latest_leave_time_str = find_value(job_status.get_obj(), "latest_leave_time").get_str();
            if (0 == latest_leave_time_str.length() || latest_leave_time_str == "not-a-date-time")
                _latest_leave_time = boost::date_time::not_a_date_time;
            else
                _latest_leave_time = boost::posix_time::time_from_string(latest_leave_time_str);
            _latest_update_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "latest_update_time").get_str());
            mArray histories = find_value(job_status.get_obj(), "histories").get_array();
            foreach(mValue &h, histories){
                std::vector<std::string> arguments;
                mArray args = find_value(h.get_obj(), "arguments").get_array();
                foreach(mValue a, args){
                    arguments.push_back(a.get_str());
                }
                _histories.push_back(history_record::ptr(new history_record(
                    find_value_string(h.get_obj(), "time"),
                    find_value_string(h.get_obj(), "original_time"),
                    (saasame::transport::job_state::type)find_value_int32(h.get_obj(), "state"),
                    find_value_int32(h.get_obj(), "error"),
                    find_value_string(h.get_obj(), "description"),
                    find_value_string(h.get_obj(), "format"),
                    arguments,
                    find_value_bool(h.get_obj(), "is_display")
                    )));
            }
            mArray progress_map = find_value(job_status.get_obj(), "progress_map").get_array();
            foreach(mValue &p, progress_map){
                loader_disk::ptr disk_progress = loader_disk::ptr(new loader_disk(find_value_string(p.get_obj(), "key")));
                disk_progress->completed = find_value(p.get_obj(), "completed").get_bool();
                disk_progress->canceled = find_value_bool(p.get_obj(), "canceled");
                disk_progress->journal_index = find_value(p.get_obj(), "journal_index").get_int64();
                disk_progress->progress = find_value(p.get_obj(), "progress").get_int64();
                disk_progress->data = find_value(p.get_obj(), "data").get_int64();
                disk_progress->duplicated_data = find_value(p.get_obj(), "duplicated_data").get_int64();
                disk_progress->transport_data = find_value(p.get_obj(), "transport_data").get_int64();
                disk_progress->cdr = find_value_bool(p.get_obj(), "cdr");

                mArray purgging = find_value(p.get_obj(), "purgging").get_array();
                foreach(mValue &b, purgging){
                    block_file::ptr _b = block_file::ptr(new block_file());
                    _b->name = macho::stringutils::convert_utf8_to_unicode(find_value(b.get_obj(), "name").get_str());
                    _b->data = find_value(b.get_obj(), "data").get_int();
                    _b->duplicated = find_value_bool(b.get_obj(), "duplicated");
                    _b->data_end = find_value(b.get_obj(), "data_end").get_int64();
                    _b->index = find_value(b.get_obj(), "index").get_int();
                    _b->transport_size = find_value_int32(b.get_obj(), "transport_size");
                    disk_progress->purgging.push_back(_b);
                }

                mArray completed_blocks = find_value_array(p.get_obj(), "completed_blocks");
                foreach(mValue &b, completed_blocks){
                    block_file::ptr _b = block_file::ptr(new block_file());
                    _b->name = macho::stringutils::convert_utf8_to_unicode(find_value(b.get_obj(), "name").get_str());
                    _b->data = find_value(b.get_obj(), "data").get_int();
                    _b->duplicated = find_value_bool(b.get_obj(), "duplicated");
                    _b->data_end = find_value(b.get_obj(), "data_end").get_int64();
                    _b->index = find_value(b.get_obj(), "index").get_int();
                    _b->transport_size = find_value_int32(b.get_obj(), "transport_size");
                    disk_progress->completed_blocks.push_back(_b);
                }
                _progress_map[disk_progress->key] = disk_progress;
            }
            _current_snapshot = find_value_string(job_status.get_obj(), "current_snapshot");
            _mgmt_addr = find_value_string(job_status.get_obj(), "mgmt_addr");
            if (boost::filesystem::exists(_progress_file)){
                return load_progress(_progress_file.wstring());
            }
            return true;
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot load status info.")));
    }
    catch (...){
    }
    return false;
}

bool loader_job::load_progress(std::wstring progress_file){
    FUN_TRACE;
    try{
        if (boost::filesystem::exists(progress_file)){
            std::ifstream is(progress_file);
            mValue p;
            read(is, p);
            std::string key = find_value_string(p.get_obj(), "key");
            loader_disk::ptr disk_progress; 
            if (_progress_map.count(key))
                disk_progress = _progress_map[key];
            if (disk_progress){
                disk_progress->journal_index = find_value(p.get_obj(), "journal_index").get_int64();
                disk_progress->progress = find_value(p.get_obj(), "progress").get_int64();
                disk_progress->data = find_value(p.get_obj(), "data").get_int64();
                disk_progress->duplicated_data = find_value(p.get_obj(), "duplicated_data").get_int64();
                disk_progress->transport_data = find_value(p.get_obj(), "transport_data").get_int64();
                disk_progress->purgging.clear();
                mArray purgging = find_value(p.get_obj(), "purgging").get_array();
                foreach(mValue &b, purgging){
                    block_file::ptr _b = block_file::ptr(new block_file());
                    _b->name = macho::stringutils::convert_utf8_to_unicode(find_value(b.get_obj(), "name").get_str());
                    _b->data = find_value(b.get_obj(), "data").get_int();
                    _b->duplicated = find_value_bool(b.get_obj(), "duplicated");
                    _b->data_end = find_value(b.get_obj(), "data_end").get_int64();
                    _b->index = find_value(b.get_obj(), "index").get_int();
                    _b->transport_size = find_value_int32(b.get_obj(), "transport_size");
                    disk_progress->purgging.push_back(_b);
                }
                disk_progress->completed_blocks.clear();
                mArray completed_blocks = find_value_array(p.get_obj(), "completed_blocks");
                foreach(mValue &b, completed_blocks){
                    block_file::ptr _b = block_file::ptr(new block_file());
                    _b->name = macho::stringutils::convert_utf8_to_unicode(find_value(b.get_obj(), "name").get_str());
                    _b->data = find_value(b.get_obj(), "data").get_int();
                    _b->duplicated = find_value_bool(b.get_obj(), "duplicated");
                    _b->data_end = find_value(b.get_obj(), "data_end").get_int64();
                    _b->index = find_value(b.get_obj(), "index").get_int();
                    _b->transport_size = find_value_int32(b.get_obj(), "transport_size");
                    disk_progress->completed_blocks.push_back(_b);
                }
                return true;
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot load progress info.")));
    }
    catch (...){
    }
    return false;
}

saasame::transport::loader_job_detail loader_job::get_job_detail(bool complete) {
    macho::windows::auto_lock lock(_cs);
    FUN_TRACE;
    saasame::transport::loader_job_detail update_detail;
    update_detail.created_time = boost::posix_time::to_simple_string(_created_time);
    update_detail.id = macho::guid_(_id);
    update_detail.replica_id = _loader_job_create_detail.replica_id;
    update_detail.__set_replica_id(update_detail.replica_id);
    update_detail.connection_id = _loader_job_create_detail.connection_id;
    update_detail.__set_connection_id(update_detail.connection_id);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    update_detail.updated_time = boost::posix_time::to_simple_string(update_time);
    update_detail.state = (saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error);
    if (complete){
        foreach(history_record::ptr &h, _histories){
            saasame::transport::job_history _h;
            _h.state = h->state;
            _h.error = h->error;
            _h.description = h->description;
            _h.time = boost::posix_time::to_simple_string(h->time);
            _h.format = h->format;
            _h.is_display = h->is_display;
            _h.arguments = h->args;
            _h.__set_arguments(_h.arguments);
            update_detail.histories.push_back(_h);
        }
        update_detail.__set_histories(update_detail.histories);

        foreach(loader_disk::map::value_type& l, _progress_map){
            update_detail.progress[l.first] = l.second->progress;
            update_detail.data[l.first] = l.second->data;
            update_detail.transport_data[l.first] = l.second->transport_data;
        }
        update_detail.__set_progress(update_detail.progress);
        update_detail.__set_data(update_detail.data);
        update_detail.__set_transport_data(update_detail.transport_data);
        update_detail.snapshot_id = _current_snapshot;
        update_detail.__set_snapshot_id(update_detail.snapshot_id);
    }
    return update_detail;
}

void loader_job::interrupt(){
    FUN_TRACE;
    _is_interrupted = true;
    _wait_cond.notify_one();
    if (_running.try_lock()){
        save_status();
        _running.unlock();
    }
}

void loader_job::resume(){
    FUN_TRACE;
    _is_canceled = _is_interrupted = false;
    if (_running.try_lock()){
        save_status();
        _running.unlock();
    }
}

bool loader_job::_update(bool force){
    int  retry_count = RetryCount;
    bool result = true;
    while (retry_count > 0 ){
        if (result = __update(force))
            break;
        else if (_terminated)
            break;
        else
            boost::this_thread::sleep(boost::posix_time::seconds(10));
        retry_count--;
    }
    return result;
}

bool loader_job::__update(bool force){
    FUN_TRACE;
    bool              result = true;    
    loader_job_detail update_detail;
    update_detail.created_time = boost::posix_time::to_simple_string(_created_time);
    update_detail.id = macho::guid_(_id);
    macho::windows::auto_lock lock(_cs);
    update_detail.state = (saasame::transport::job_state::type)(_state & ~mgmt_job_state::job_state_error);
    update_detail.replica_id = _loader_job_create_detail.replica_id;
    update_detail.connection_id = _loader_job_create_detail.connection_id;
    update_detail.snapshot_id = _current_snapshot;
    update_detail.__set_created_time(update_detail.created_time);
    update_detail.__set_id(update_detail.id);
    update_detail.__set_state(update_detail.state);
    update_detail.__set_replica_id(update_detail.replica_id);
    update_detail.__set_connection_id(update_detail.connection_id);
    update_detail.__set_snapshot_id(update_detail.snapshot_id);
    foreach(loader_disk::map::value_type& l, _progress_map){
        update_detail.progress[l.first] = l.second->progress;
        update_detail.data[l.first] = l.second->data;
        update_detail.transport_data[l.first] = l.second->transport_data;
        //update_detail.duplicated_data[l.first] = l.second->duplicated_data;
    }
    update_detail.__set_progress(update_detail.progress);
    update_detail.__set_data(update_detail.data);
    update_detail.__set_transport_data(update_detail.transport_data);
    update_detail.__set_duplicated_data(update_detail.duplicated_data);
    update_detail.__set_snapshot_id(update_detail.snapshot_id);
    boost::posix_time::ptime update_time = boost::posix_time::microsec_clock::universal_time();
    update_detail.updated_time = boost::posix_time::to_simple_string(update_time);
    update_detail.__set_updated_time(update_detail.updated_time);
    for (history_record::vtr::iterator h = _histories.begin(); h != _histories.end(); h++){
        if ((*h)->time > _latest_update_time){
            saasame::transport::job_history _h;
            _h.state = (*h)->state;
            _h.error = (*h)->error;
            _h.description = (*h)->description;
            _h.time = boost::posix_time::to_simple_string((*h)->time);
            _h.format = (*h)->format;
            _h.is_display = (*h)->is_display;
            _h.arguments = (*h)->args;
            _h.__set_arguments(_h.arguments);
            update_time = (*h)->time;
            update_detail.histories.push_back(_h);
            LOG(LOG_LEVEL_DEBUG, _T("Update histories for snapshot(%s) to management. Time: (%s), Message (%s)"), macho::stringutils::convert_ansi_to_unicode(_current_snapshot).c_str(),
                macho::stringutils::convert_ansi_to_unicode(_h.time).c_str(),
                macho::stringutils::convert_ansi_to_unicode(_h.description).c_str());
        }
    }
    update_detail.__set_histories(update_detail.histories);
    if (force || update_detail.histories.size()){
        if (result = update_state(update_detail)){
            _latest_update_time = update_time;
        }
    }
    return result;
}

void loader_job::update(){
    while (!_terminated){
        boost::this_thread::sleep(boost::posix_time::seconds(15));       
        _update(true);
    }
}

template <class JOB, class TASK>
void loader_job::_execute(std::string name, connection_op::ptr conn_ptr, loader_disk::ptr disk_progress, universal_disk_rw::ptr rw, bool is_canceled_snapshot, uint64_t size, connection_op::ptr cascading, std::vector<std::wstring>& parallels, std::vector<std::wstring>& branches, bool is_cascading_job){
    FUN_TRACE;
    using namespace saasame::ironman::imagex;
    irm_transport_image_blks_chg_journal::ptr target_journal_db;
    read_imagex readx(conn_ptr, name, cascading, _machine_id, parallels, branches);
    LOG(LOG_LEVEL_INFO, _T("Replicating data from disk (%s)."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
    while (true){
        irm_transport_image::ptr image = readx.get_image_info();
        irm_transport_image_blks_chg_journal::ptr journal = readx.get_journal_info();
        if (image && journal){
            if (!target_journal_db){
                target_journal_db = irm_transport_image_blks_chg_journal::ptr(new irm_transport_image_blks_chg_journal());
                target_journal_db->parent_path = journal->parent_path;
                target_journal_db->comment = journal->comment;
                target_journal_db->name = journal->name;
                target_journal_db->extension = journal->extension;
                if (-1 != disk_progress->journal_index){
                    target_journal_db->next_block_index = disk_progress->journal_index + 1;
                }
            }
            std::string comment = macho::stringutils::convert_unicode_to_utf8(journal->comment);
            _is_cdr = disk_progress->cdr = journal->is_cdr();
            _state = mgmt_job_state::job_state_replicating;
            if (image->canceled &&!image->checksum){
                LOG(LOG_LEVEL_INFO, _T("Abort replicating all data from disk (%s)."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Abort replicating all data from disk %1%.") % comment));
                disk_progress->completed = false;
                disk_progress->canceled = true; 
                for (block_file::vtr::iterator b = disk_progress->purgging.begin(); b != disk_progress->purgging.end();){
                    //macho::windows::auto_lock lock(*(*b)->lock);
                    if ((*b)->duplicated)
                        conn_ptr->remove_file((*b)->name + IRM_IMAGE_DEDUPE_TMP_EXT);
                    if (!conn_ptr->remove_file((*b)->name)){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot remove data block file (%s)."), (*b)->name.c_str());
                    }
                    b = disk_progress->purgging.erase(b);
                }
                save_progress(*disk_progress);
                if (journal->is_version_2()){
                    for (uint64_t index = 0; index < journal->next_block_index; index++){
                        std::wstring _file = image->get_block_name(index);
                        conn_ptr->remove_file(_file);
                    }
                    target_journal_db->next_block_index = journal->next_block_index;
                    if (!readx.replicate_journal(target_journal_db)){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s journal file."), image->name.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot update disk %1% journal file.") % name));
                        break;
                    }
                    else if (!readx.replicate_image(image)){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s image file."), image->name.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot update disk %1% image file.") % name));
                        break;
                    }
                    uint64_t next_block_index = readx.get_cascadings_journal_next_block_index();
                    if ((conn_ptr->type() == connection_op_type::local && parallels.empty() && branches.empty()) ||
                        (conn_ptr->type() == connection_op_type::remote && parallels.empty())){
                        readx.close_journal(journal);
                        readx.clear();                        
                    }
                    else{
                        macho::windows::lock_able::ptr lock_obj = readx.get_source_journal_lock();
                        macho::windows::auto_lock lock(*lock_obj);
                        if (cascading && cascading->type() == connection_op_type::local){
                            if (readx.close_journal(journal) && readx.are_cascadings_journals_closed())
                                readx.clear();
                        }
                        else{
                            if (readx.are_cascadings_journals_closed())
                                readx.clear();
                            else if (readx.is_source_journal_existing())
                                readx.close_journal(journal);
                        }
                    }
                }
                else{
                    for (uint64_t index = 0; index < journal->dirty_blocks_list.size(); index++){
                        std::wstring _file = image->get_block_name(journal->dirty_blocks_list[index]);
                        conn_ptr->remove_file(_file + IRM_IMAGE_DEDUPE_TMP_EXT);
                        conn_ptr->remove_file(_file);
                    }
                    readx.clear();
                }
                _sent_messages.erase(name);
                break;
            }
            else if (!_loader_job_create_detail.block_mode_enable && !image->completed && journal->is_version_2() && disk_progress->journal_index == (journal->next_block_index - 1)){
                LOG(LOG_LEVEL_WARNING, _T("Non-Block Mode: no more data for (%s). Break thread first."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
                break;
            }
            else if (!_loader_job_create_detail.block_mode_enable && !image->completed && !journal->is_version_2() && disk_progress->journal_index == (journal->dirty_blocks_list.size() - 1)){
                LOG(LOG_LEVEL_WARNING, _T("Non-Block Mode: no more data for (%s). Break thread first."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
                break;
            }
            else if (cascading && !image->completed && journal->is_version_2() && !readx.replicate_image(image)){
                LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s image file."), image->name.c_str());
                break;
            }
            if (0 == _sent_messages.count(name)){
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Replicating snapshot data from disk %1%.") % comment));
                _sent_messages[name] = true;
            }
            _is_replicating = true;
            
            block_file::vtr  block_files;
            try{
                uint64_t index = disk_progress->journal_index + 1;
                if (journal->is_version_2()){
                    for (index; index < journal->next_block_index; index++){
                        block_file::ptr b = block_file::ptr(new block_file());
                        b->name = image->get_block_name(index);
                        b->index = index;
                        block_files.push_back(b);
                    }
                }
                else{
                    disk_progress->completed_blocks.clear();
                    save_progress(*disk_progress);
                    for (index; index < journal->dirty_blocks_list.size(); index++){
                        std::wstring _file = image->get_block_name(journal->dirty_blocks_list[index]);
                        auto it = std::find_if(block_files.begin(), block_files.end(), [&_file](const block_file::ptr& obj){return obj->name == _file; });
                        if (it != block_files.end())
                            (*it)->index = index;
                        else{
                            block_file::ptr b = block_file::ptr(new block_file());
                            b->name = _file;
                            b->index = index;
                            b->lock = conn_ptr->get_lock_object(_file);
                            b->lock->lock();
                            block_files.push_back(b);
                        }
                    }
                    std::sort(block_files.begin(), block_files.end(), block_file::compare());
                }
                replication_repository repository(*image, *disk_progress, macho::stringutils::convert_ansi_to_unicode(_machine_id), parallels, branches);
                repository.purge_data = _loader_job_create_detail.purge_data;
                repository.has_customized_id = (_loader_job_create_detail.detect_type == disk_detect_type::type::CUSTOMIZED_ID);
                repository.journal_db = target_journal_db;

                foreach(block_file::ptr &_bk_file, block_files)
                    repository.waitting.push_back(_bk_file);
                if (repository.waitting.size()){
                    int worker_thread_number = _loader_job_create_detail.worker_thread_number == 0 ? NumberOfWorkers : _loader_job_create_detail.worker_thread_number;
                    int max_worker_thread_number = worker_thread_number;
                    int resume_thread_count = RESUME_THREAD_COUNT;
                    int resume_thread_time = RESUME_THREAD_TIME;
                    registry reg;
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (reg[L"ResumeThreadCount"].exists() && (DWORD)reg[L"ResumeThreadCount"] > 0){
                            resume_thread_count = (DWORD)reg[L"ResumeThreadCount"];
                        }
                        if (reg[L"ResumeThreadTime"].exists() && (DWORD)reg[L"ResumeThreadTime"] > 0){
                            resume_thread_time = (DWORD)reg[L"ResumeThreadTime"];
                        }
                    }
                    while (worker_thread_number > 0){
                        _state &= ~mgmt_job_state::job_state_error;
                        repository.terminated = false;
                        TASK::vtr  tasks;
                        if (worker_thread_number > repository.waitting.size())
                            worker_thread_number = repository.waitting.size();

                        for (int i = 0; i < worker_thread_number; i++){
#ifdef CONNECTION_POOL
                            connection_op::ptr in = conn_ptr;
                            universal_disk_rw::ptr out =  rw ;
#else
                            connection_op::ptr in = conn_ptr->clone();
                            universal_disk_rw::ptr out = i == 0 ? rw : rw->clone();
#endif
                            if (is_cascading_job && in && cascading || in && out){
                                TASK::ptr task(new TASK(in, out, repository, size, cascading));
                                task->register_job_is_canceled_function(boost::bind(&JOB::is_canceled, this));
                                task->register_job_save_callback_function(boost::bind(&JOB::save_status, this));
                                task->register_job_save_progress_callback_function(boost::bind(&JOB::save_progress, this, _1));
                                task->register_job_record_callback_function(boost::bind(&JOB::record, this, _1, _2, _3));
                                tasks.push_back(task);
                            }
                        }

                        boost::thread_group  thread_pool;
                        LOG(LOG_LEVEL_RECORD, L"(%s) Number of running threads is %d. Waitting list size (%d)", _id.c_str(), tasks.size(), repository.waitting.size());
                        if (tasks.size())
                        {
                            for (int i = 0; i < tasks.size(); i++)
                                thread_pool.create_thread(boost::bind(&TASK::replicate, &(*tasks[i])));
                            boost::unique_lock<boost::mutex> _lock(repository.completion_lock);
                            int waitting_count = 0;
                            while (true){
                                repository.completion_cond.timed_wait(_lock, boost::posix_time::milliseconds(60000));
                                {
                                    macho::windows::auto_lock lock(repository.cs);
                                    if (repository.error.size() != 0 || repository.update_journal_error) {
                                        _state |= mgmt_job_state::job_state_error;
                                        repository.terminated = true;
                                        resume_thread_count--;
                                        break;
                                    }
                                    else if ((repository.processing.size() == 0 && repository.waitting.size() == 0) || is_canceled()){
                                        repository.terminated = true;
                                        break;
                                    }
                                    else if (resume_thread_count && max_worker_thread_number > worker_thread_number && repository.waitting.size() > worker_thread_number){
                                        if (waitting_count >= resume_thread_time){
#ifdef CONNECTION_POOL
                                            connection_op::ptr in = conn_ptr;
                                            universal_disk_rw::ptr out = rw;
#else
                                            connection_op::ptr in = conn_ptr->clone();
                                            universal_disk_rw::ptr out = i == 0 ? rw : rw->clone();
#endif
                                            if (is_cascading_job && in && cascading || in && out){
                                                TASK::ptr task(new TASK(in, out, repository, size, cascading));
                                                task->register_job_is_canceled_function(boost::bind(&JOB::is_canceled, this));
                                                task->register_job_save_callback_function(boost::bind(&JOB::save_status, this));
                                                task->register_job_save_progress_callback_function(boost::bind(&JOB::save_progress, this, _1));
                                                task->register_job_record_callback_function(boost::bind(&JOB::record, this, _1, _2, _3));
                                                tasks.push_back(task);
                                                thread_pool.create_thread(boost::bind(&TASK::replicate, &(*tasks[tasks.size() - 1])));
                                                waitting_count = 0;
                                                worker_thread_number++;
                                            }
                                        }
                                        waitting_count++;
                                    }
                                }
                            }
                            thread_pool.join_all();
                            worker_thread_number--;
                            if ((repository.error.size() == 0 && repository.processing.size() == 0 && repository.waitting.size() == 0) || is_canceled()){
                                break;
                            }
                            else if (worker_thread_number){
                                foreach(block_file::ptr& b, repository.processing){
                                    repository.waitting.push_front(b);
                                }
                                repository.processing.clear();
                                repository.error.clear();
                                repository.update_journal_error = false;
                                std::sort(repository.waitting.begin(), repository.waitting.end(), block_file::compare());
                                boost::this_thread::sleep(boost::posix_time::seconds(5));
                            }
                            else {
                                break;
                            }
                        }
                    }
                }
            }
            catch (macho::windows::lock_able::exception &ex){
                _state |= mgmt_job_state::job_state_error;
                LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
            }

            if (image->mode != IRM_IMAGE_TRANSPORT_MODE){
                foreach(block_file::ptr &bk_file, block_files){
                    bk_file->lock->unlock();
                }
            }
            if (_state & mgmt_job_state::job_state_error){
                save_status();
                break;
            }
            if (!readx.replicate_journal(target_journal_db)){
                LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s journal file."), image->name.c_str());
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot update disk %1% journal file.") % name));
                break;
            }
            if (image->canceled && image->checksum && 
                ((!journal->is_version_2() && disk_progress->journal_index == (journal->dirty_blocks_list.size() - 1)) ||
                (journal->is_version_2() && disk_progress->journal_index == (journal->next_block_index - 1)))){
                disk_progress->canceled = true;
                for (block_file::vtr::iterator b = disk_progress->purgging.begin(); b != disk_progress->purgging.end();){
                    //macho::windows::auto_lock lock(*(*b)->lock);
                    if ((*b)->duplicated)
                        conn_ptr->remove_file((*b)->name + IRM_IMAGE_DEDUPE_TMP_EXT);
                    if (!conn_ptr->remove_file((*b)->name)){
                        LOG(LOG_LEVEL_ERROR, _T("Failed to remove the data block file (%s)."), (*b)->name.c_str());
                    }
                    b = disk_progress->purgging.erase(b);
                }
                if (_loader_job_create_detail.purge_data){
                    if (!journal->is_version_2()){
                        readx.clear();
                    }
                    else{
                        if ((conn_ptr->type() == connection_op_type::local && parallels.empty() && branches.empty()) ||
                            (conn_ptr->type() == connection_op_type::remote && parallels.empty())){
                            readx.close_journal(journal);
                            readx.clear();
                        }
                        else{
                            macho::windows::lock_able::ptr lock_obj = readx.get_source_journal_lock();
                            macho::windows::auto_lock lock(*lock_obj);
                            if (cascading && cascading->type() == connection_op_type::local){
                                if (readx.close_journal(journal) && readx.are_cascadings_journals_closed())
                                    readx.clear();
                            }
                            else{
                                if (readx.are_cascadings_journals_closed())
                                    readx.clear();
                                else if (readx.is_source_journal_existing())
                                    readx.close_journal(journal);
                            }
                        }
                    }
                }
                _sent_messages.erase(name);
                LOG(LOG_LEVEL_INFO, _T("Abort replicating snapshot data from disk (%s)."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Abort replicating snapshot data from disk %1%.") % comment));
                break;
            }
            else if (image->completed){
                _sent_messages.erase(name);
                if (!journal->is_version_2() && disk_progress->journal_index == (journal->dirty_blocks_list.size() - 1)){
                    disk_progress->progress = image->total_size;
                    disk_progress->completed = true;
                    for (block_file::vtr::iterator b = disk_progress->purgging.begin(); b != disk_progress->purgging.end();){
                        //macho::windows::auto_lock lock(*(*b)->lock);
                        if ((*b)->duplicated)
                            conn_ptr->remove_file((*b)->name + IRM_IMAGE_DEDUPE_TMP_EXT);
                        if (!conn_ptr->remove_file((*b)->name)){
                            LOG(LOG_LEVEL_ERROR, _T("Cannot remove data block file (%s)."), (*b)->name.c_str());
                        }
                        b = disk_progress->purgging.erase(b);
                    }
                    if (_loader_job_create_detail.purge_data)
                        readx.clear();
                    LOG(LOG_LEVEL_INFO, _T("Data replication from disk (%s) completed with size : %I64u."), macho::stringutils::convert_utf8_to_unicode(name).c_str(), disk_progress->transport_data);
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Data replication from disk %1% completed with size : %2%") % comment %disk_progress->transport_data));
                    break;
                }
                else if (journal->is_version_2() && disk_progress->journal_index == (journal->next_block_index - 1)){
                    if (!readx.replicate_image(image)){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s image file."), image->name.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot update disk %1% image file.") % name));
                        break;
                    }
                    uint64_t next_block_index = readx.get_cascadings_journal_next_block_index();
                    disk_progress->progress = image->total_size;
                    disk_progress->completed = true;
                    for (block_file::vtr::iterator b = disk_progress->purgging.begin(); b != disk_progress->purgging.end();){
                        if (next_block_index > (*b)->index && !conn_ptr->remove_file((*b)->name)){
                            LOG(LOG_LEVEL_ERROR, _T("Cannot remove data block file (%s)."), (*b)->name.c_str());
                        }
                        b = disk_progress->purgging.erase(b);
                    }
                    if (_loader_job_create_detail.purge_data){
                        if ((conn_ptr->type() == connection_op_type::local && parallels.empty() && branches.empty()) ||
                            (conn_ptr->type() == connection_op_type::remote && parallels.empty())){
                            readx.close_journal(journal);
                            readx.clear();
                        }
                        else{
                            macho::windows::lock_able::ptr lock_obj = readx.get_source_journal_lock();
                            macho::windows::auto_lock lock(*lock_obj);
                            if (cascading && cascading->type() == connection_op_type::local){
                                if (readx.close_journal(journal) && readx.are_cascadings_journals_closed())
                                    readx.clear();
                            }
                            else{
                                if (readx.are_cascadings_journals_closed())
                                    readx.clear();
                                else if (readx.is_source_journal_existing())
                                    readx.close_journal(journal);
                            }
                        }
                    }
                    LOG(LOG_LEVEL_INFO, _T("Data replication from disk (%s) completed with size : %I64u."), macho::stringutils::convert_utf8_to_unicode(name).c_str(), disk_progress->transport_data);
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Data replication from disk %1% completed with size : %2%") % comment %disk_progress->transport_data));
                    break;
                }
            }          
        }
        else if (!image){
            if (is_canceled_snapshot)
                disk_progress->canceled = true;
            else
                LOG(LOG_LEVEL_ERROR, _T("Cannot open meta file (%s)."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
        }
        else if (!journal){
            LOG(LOG_LEVEL_ERROR, _T("Cannot open journal file (%s%s)"), macho::stringutils::convert_utf8_to_unicode(name).c_str(), IRM_IMAGE_JOURNAL_EXT);
        }
        save_progress(*disk_progress);
        save_status();
        if (_is_interrupted || _is_canceled)
            break;
        else if (_loader_job_create_detail.block_mode_enable)
            boost::this_thread::sleep(boost::posix_time::seconds(10));
        else if (!image || !journal){
            LOG(LOG_LEVEL_WARNING, _T("Non-Block Mode: no more data for (%s). Break thread first."), macho::stringutils::convert_utf8_to_unicode(name).c_str());
            break;
        }
    }
    save_progress(*disk_progress);
    save_status();
}

void loader_job::execute(){
    VALIDATE;
    if (/*(_state & mgmt_job_state::job_state_finished) ||*/ _is_canceled || _is_removing){
        return;
    }
    FUN_TRACE;

    _state &= ~mgmt_job_state::job_state_error;
    bool is_get_loader_job_create_detail = false;
    _is_cdr = false;
    if (!_is_initialized){
        is_get_loader_job_create_detail = _is_initialized = get_loader_job_create_detail(_loader_job_create_detail);
        LOG(LOG_LEVEL_INFO, _T("Initializing Loader job."));
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Initializing Loader job."));
    }
    else{
        is_get_loader_job_create_detail = get_loader_job_create_detail(_loader_job_create_detail);
    }
    save_config();
    
    connection_op::ptr conn_ptr;
    if (!_is_initialized){
        LOG(LOG_LEVEL_ERROR, _T("Cannot initialize Loader job."));
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, record_format("Cannot initialize Loader job."));
    }
    else if (!is_get_loader_job_create_detail){
        _get_loader_job_create_detail_error_count++;
        if (_get_loader_job_create_detail_error_count > 3){
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot get the loader job detail"));
            _state |= mgmt_job_state::job_state_error;
            save_status();
        }
    }
    else if (_loader_job_create_detail.is_paused){
        LOG(LOG_LEVEL_WARNING, _T("Loader job(%s) is paused."), _id.c_str());
    }
    else if (!_loader_job_create_detail.snapshots.size()){
        intrrupt_unused_cascading_jobs();
        LOG(LOG_LEVEL_WARNING, _T("Doesn't have any snapshot for Loader job(%s)."), _id.c_str());
        if (_loader_job_create_detail.remap)
            dismount_devices();        
        if (!_loader_job_create_detail.keep_alive && !_loader_job_create_detail.is_continuous_data_replication)
            modify_job_scheduler_interval(15);
    }
    else{
        disable_auto_mount_disk_devices();
        modify_job_scheduler_interval(1);
        uint32_t concurrent_count = boost::thread::hardware_concurrency();
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"ConcurrentLoaderJobNumber"].exists() && (DWORD)reg[L"ConcurrentLoaderJobNumber"] > 0){
                concurrent_count = ((DWORD)reg[L"ConcurrentLoaderJobNumber"]);
            }
        }
        macho::windows::semaphore  loader(L"Global\\Loader", concurrent_count);
        if (!loader.trylock()){
            LOG(LOG_LEVEL_WARNING, L"Resource limited. Job %s will be resumed later.", _id.c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_QUEUE_FULL,
                record_format("Pending for available job resource."));
        }
        else if (!(conn_ptr = connection_op::get(_loader_job_create_detail.connection_id,
#ifdef CONNECTION_POOL      
            _loader_job_create_detail.worker_thread_number
#else
            1
#endif
            ))){
            LOG(LOG_LEVEL_ERROR, _T("Loader cannot create connection."));
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, record_format("Loader cannot create connection."));
        }
        else{
            _get_loader_job_create_detail_error_count = 0;
            if (_is_continuous_data_replication != _loader_job_create_detail.is_continuous_data_replication){
                if (_loader_job_create_detail.is_continuous_data_replication){
                    _is_continuous_data_replication = true;
                }
                else{
                    _is_continuous_data_replication = false;
                }
            }
            conn_ptr->register_is_cancelled_function(boost::bind(&loader_job::is_canceled, this));
            _is_interrupted = _terminated = false;
            boost::thread  thread = boost::thread(&loader_job::update, this);
            _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
            _latest_enter_time = boost::posix_time::microsec_clock::universal_time();
            bool mounted = true;
            if (_loader_job_create_detail.remap){
                bool try_mount = false;
                if (!(mounted = is_devices_ready()))
                    try_mount = mount_devices();
                int count = 100;
                while (try_mount&& !_is_interrupted && count > 0 && !(mounted = is_devices_ready())){
                    boost::unique_lock<boost::mutex> wait(_wait_lock);
                    _wait_cond.timed_wait(wait, boost::posix_time::seconds(5));
                    count--;
                }
                get_loader_job_create_detail(_loader_job_create_detail);
            }
            std::map<std::string, std::wstring>   diskio_map;
            std::map<std::string, uint64_t>       disksize_map;
#ifdef _VMWARE_VADP
            vmware_portal_::ptr                            portal;
            mwdc::ironman::hypervisor_ex::vmware_portal_ex portal_ex;
            vmware_virtual_machine::ptr                    vm;
            vmware_vixdisk_connection_ptr                  conn;
#endif
            if (mounted){
#ifdef _VMWARE_VADP
                if (_loader_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
                    vmware_portal_ex::set_log(get_log_file(), get_log_level());
                    LOG(LOG_LEVEL_DEBUG, _T("Try to connect the ESX host (%s)."), macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.host).c_str());
                    std::wstring uri = boost::str(boost::wformat(L"https://%s/sdk") % macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.host));
                    portal = vmware_portal_::connect(uri, macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.username), macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.password));
                    if (portal){
                        vm = portal->get_virtual_machine(_id);
                        if (!vm){
                            vmware_virtual_machine_config_spec config_spec;
                            config_spec.name = macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.host_name) + std::wstring(L"-") + _id.substr(0, 8);
                            config_spec.config_path = macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.datastore);
                            config_spec.host_key = macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.esx);
                            config_spec.folder_path = macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.folder_path);
                            config_spec.memory_mb = 64;
                            config_spec.number_of_cpu = 1;
                            config_spec.uuid = macho::guid_(_id);
                            //config_spec.nics.push_back(vmware_virtual_network_adapter::ptr(new vmware_virtual_network_adapter()));
                            //config_spec.nics[0]->network = hosts[0]->networks.begin()->second;
                            //config_spec.nics[0]->type = L"E1000";
                            if (_loader_job_create_detail.disks_order.empty()){
                                foreach(string_map::value_type &d, _loader_job_create_detail.disks_lun_mapping){
                                    vmware_disk_info::ptr vmware_disk(new vmware_disk_info());
                                    vmware_disk->controller = vmware_virtual_scsi_controller::ptr(new vmware_virtual_scsi_controller());
                                    vmware_disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC_SAS;
                                    vmware_disk->name = macho::stringutils::convert_utf8_to_unicode(d.second);
                                    vmware_disk->uuid = macho::stringutils::convert_utf8_to_unicode(d.first.substr(0, 36));
                                    vmware_disk->size = _loader_job_create_detail.disks_size_mapping[d.first];
                                    vmware_disk->thin_provisioned = _loader_job_create_detail.thin_provisioned;
                                    config_spec.disks.push_back(vmware_disk);
                                }
                            }
                            else{
                                foreach(std::string d, _loader_job_create_detail.disks_order){
                                    vmware_disk_info::ptr vmware_disk(new vmware_disk_info());
                                    vmware_disk->controller = vmware_virtual_scsi_controller::ptr(new vmware_virtual_scsi_controller());
                                    vmware_disk->controller->type = mwdc::ironman::hypervisor::hv_controller_type::HV_CTRL_LSI_LOGIC_SAS;
                                    vmware_disk->name = macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.disks_lun_mapping[d]);
                                    vmware_disk->uuid = macho::stringutils::convert_utf8_to_unicode(d.substr(0, 36));
                                    vmware_disk->size = _loader_job_create_detail.disks_size_mapping[d];
                                    vmware_disk->thin_provisioned = _loader_job_create_detail.thin_provisioned;
                                    config_spec.disks.push_back(vmware_disk);
                                }
                            }
                            vm = portal->create_virtual_machine(config_spec);
                        }
                        if (vm){
                            portal_ex.set_portal(portal, _id);
                            conn = portal_ex.get_vixdisk_connection(
                                macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.host),
                                macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.username),
                                macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.password),
                                vm->key, L"", false);
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("Failed to create a virtual machine for replications."));
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, record_format("Failed to create a virtual machine for replications."));
                        }
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, _T("Cannot connect the ESX host (%s)."), macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.vmware_connection.host).c_str());
                    }
                }
#endif
                com_init com;
                storage::ptr stg = storage::get();
                storage::disk::vtr disks = stg->get_disks();
                int retry_count = 1;
                while (diskio_map.size() != _loader_job_create_detail.disks_lun_mapping.size() &&
                    retry_count >= 0){
                    storage::ptr stg = retry_count > 0 ? storage::get() : storage::local();
                    storage::disk::vtr disks = stg->get_disks();
                    foreach(string_map::value_type &d, _loader_job_create_detail.disks_lun_mapping){
                        if (_is_interrupted || _is_canceled)
                            break;
                        if (diskio_map.count(d.first))
                            continue;
                        storage::disk::ptr _p;
                        if (_loader_job_create_detail.detect_type == disk_detect_type::type::SCSI_ADDRESS){
                            DWORD scsi_port = 0, scsi_bus = 0, scsi_target_id = 0, scsi_lun = 0;
                            if (4 == sscanf_s(d.second.c_str(), "%d:%d:%d:%d", &scsi_port, &scsi_bus, &scsi_target_id, &scsi_lun)){
                                foreach(storage::disk::ptr p, disks){
                                    if (p->scsi_port() == scsi_port &&
                                        p->scsi_bus() == scsi_bus &&
                                        p->scsi_target_id() == scsi_target_id &&
                                        p->scsi_logical_unit() == scsi_lun){
                                        _p = p;
                                        break;
                                    }
                                }
                            }
                            else{
                                LOG(LOG_LEVEL_ERROR, _T("Invalid SCSI address (%s) for disk (%s)."), macho::stringutils::convert_ansi_to_unicode(d.second).c_str(), macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Invalid SCSI address %1% for disk %2%.") % d.second % d.first));
                                _state |= mgmt_job_state::job_state_error;
                                save_status();
                                break;
                            }
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::SERIAL_NUMBER){
                            std::wstring serial_number = stringutils::convert_ansi_to_unicode(d.second);
                            foreach(storage::disk::ptr p, disks){
                                if (p->serial_number().length() > 0 &&
                                    boost::starts_with(serial_number, p->serial_number())){
                                    _p = p;
                                    break;
                                }
                            }
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::UNIQUE_ID){
                            std::wstring unique_id = stringutils::convert_ansi_to_unicode(d.second);
                            foreach(storage::disk::ptr p, disks){
                                if (p->unique_id().length() > 0 &&
                                    (unique_id == p->unique_id())){
                                    _p = p;
                                    break;
                                }
                            }
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::AZURE_BLOB){
#ifdef _AZURE_BLOB
                            if (azure_page_blob_op::create_vhd_page_blob(_loader_job_create_detail.azure_storage_connection_string,
                                _loader_job_create_detail.export_path, d.second, _loader_job_create_detail.disks_size_mapping[d.first], _loader_job_create_detail.host_name)){
                                diskio_map[d.first] = macho::stringutils::convert_utf8_to_unicode(d.second);
                                disksize_map[d.first] = _loader_job_create_detail.disks_size_mapping[d.first];
                            }
#endif
                        }
                        else if (_loader_job_create_detail.detect_type == saasame::transport::disk_detect_type::VMWARE_VADP){
#ifdef _VMWARE_VADP
                            if (vm){
                                std::wstring disk_id = macho::guid_(d.first.substr(0, 36));
                                if (vm->disks_map.count(disk_id)){
                                    diskio_map[d.first] = vm->disks_map[disk_id]->name;
                                    disksize_map[d.first] = _loader_job_create_detail.disks_size_mapping[d.first];
                                }
                            }
                            else{
                                retry_count = 0;
                                break;
                            }
#endif
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE){
                            boost::filesystem::path vhd_file = boost::filesystem::path(macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.export_path)) / macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.host_name) / macho::stringutils::convert_utf8_to_unicode(d.second);
                            switch (_loader_job_create_detail.export_disk_type){
                            case virtual_disk_type::type::VHDX:
                                vhd_file = vhd_file.wstring() + (L".vhdx");
                                break;
                            case virtual_disk_type::type::VHD:
                            default:
                                vhd_file = vhd_file.wstring() + (L".vhd");
                                break;
                            }
                            if (boost::filesystem::exists(vhd_file)){
                                //diskio_map[d.first] = vhd_file.wstring();
                                disksize_map[d.first] = _loader_job_create_detail.disks_size_mapping[d.first];
                                boost::filesystem::path temp_vhd_file = vhd_file.parent_path() / boost::str(boost::wformat(L"%s.delta%s") % vhd_file.filename().stem().wstring() % vhd_file.extension().wstring());
                                vhd_disk_info disk_info;
                                if (boost::filesystem::exists(temp_vhd_file)){
                                    diskio_map[d.first] = temp_vhd_file.wstring();
                                }
                                else{
                                    if (!(ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_information(vhd_file.wstring(), disk_info) &&
                                        ERROR_SUCCESS == win_vhdx_mgmt::create_vhdx_file(temp_vhd_file.wstring(), CREATE_VIRTUAL_DISK_FLAG_NONE, disk_info.virtual_size, disk_info.block_size, disk_info.logical_sector_size, disk_info.physical_sector_size, vhd_file.wstring()))){
                                        LOG(LOG_LEVEL_ERROR, L"Failed to create the detal virtual disk file %s", temp_vhd_file.wstring().c_str());
                                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Failed to create the detal virtual disk file %1%") % macho::stringutils::convert_unicode_to_utf8(temp_vhd_file.wstring())));
                                    }
                                    else{
                                        diskio_map[d.first] = temp_vhd_file.wstring();
                                    }
                                }
                            }
                            else{
                                boost::filesystem::create_directories(vhd_file.parent_path());
                                DWORD err = win_vhdx_mgmt::create_vhdx_file(vhd_file.wstring(),
                                    CREATE_VIRTUAL_DISK_FLAG::CREATE_VIRTUAL_DISK_FLAG_NONE,
                                    _loader_job_create_detail.disks_size_mapping[d.first],
                                    CREATE_VIRTUAL_DISK_PARAMETERS_DEFAULT_BLOCK_SIZE,
                                    VIXDISKLIB_SECTOR_SIZE,
                                    VIXDISKLIB_SECTOR_SIZE);
                                if (ERROR_SUCCESS == err){
                                    diskio_map[d.first] = vhd_file.wstring();
                                    disksize_map[d.first] = _loader_job_create_detail.disks_size_mapping[d.first];
                                }
                                else{
                                    LOG(LOG_LEVEL_ERROR, _T("Cannot create virtual disk (%s)."), vhd_file.wstring().c_str());
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Cannot create virtual disk %1%.") % vhd_file.string()));
                                }
                            }
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::CUSTOMIZED_ID){
                            foreach(storage::disk::ptr p, disks){
                                general_io_rw::ptr rd = general_io_rw::open(boost::str(boost::format("\\\\.\\PhysicalDrive%d") % p->number()));
                                if (rd){
                                    uint32_t r = 0;
                                    std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                                    memset(data.get(), 0, 512);
                                    if (rd->sector_read((p->size() >> 9) - 40, 1, data.get(), r)){
                                        GUID guid;
                                        memcpy(&guid, &(data.get()[512 - 16]), 16);
                                        try{
                                            if (macho::guid_(guid) == macho::guid_(d.second)){
                                                _p = p;
                                                break;
                                            }
                                        }
                                        catch (...){
                                        }
                                    }
                                }
                            }
                        }

                        if ((_loader_job_create_detail.detect_type != disk_detect_type::type::EXPORT_IMAGE) &&
                            (_loader_job_create_detail.detect_type != disk_detect_type::type::AZURE_BLOB) &&
                            (_loader_job_create_detail.detect_type != disk_detect_type::VMWARE_VADP) &&
                            _p){
                            std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % _p->number());
                            LOG(LOG_LEVEL_INFO, _T("Found the disk (%s) -> (%s)."), macho::stringutils::convert_ansi_to_unicode(d.first).c_str(), path.c_str());
                            if (_loader_job_create_detail.detect_type != disk_detect_type::type::CUSTOMIZED_ID){
                                if (_p->is_read_only()){
                                    if (_p->clear_read_only_flag()){}
                                    else if (_p->online()){
                                        if (!_p->clear_read_only_flag()){
                                            LOG(LOG_LEVEL_ERROR, _T("Cannot clear the read only flag of disk (%s)."), path.c_str());
                                        }
                                    }
                                    else{
                                        LOG(LOG_LEVEL_ERROR, _T("Cannot bring disk (%s) online."), path.c_str());
                                    }
                                }
                            }
                            if (_p->is_offline()){}
                            else if (!_p->offline()){
                                LOG(LOG_LEVEL_ERROR, _T("Cannot bring disk (%s) offline."), path.c_str());
                            }

                            if (_p->is_boot() || _p->is_system()){
                                LOG(LOG_LEVEL_ERROR, _T("Cannot use boot or system disk (%s, %s) for data replication."), macho::stringutils::convert_ansi_to_unicode(d.first).c_str(), path.c_str());
                            }
                            else{
                                diskio_map[d.first] = path;
                                disksize_map[d.first] = _p->size();
                            }
                        }

                        if (!diskio_map.count(d.first) && 0 == retry_count){
                            LOG(LOG_LEVEL_ERROR, _T("Cannot find disk (%s)."), macho::stringutils::convert_ansi_to_unicode(d.first).c_str());
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Cannot find disk %1%.") % d.first));
                            _state |= mgmt_job_state::job_state_error;
                            save_status();
                            break;
                        }
                    }
                    retry_count--;
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, _T("Job devices are not ready."));
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Job devices are not ready.")));
                _state |= mgmt_job_state::job_state_error;
                save_status();
            }

            if (_state & mgmt_job_state::job_state_none && !(_state & mgmt_job_state::job_state_error)){
                _state = job_state::type::job_state_initialed;
                LOG(LOG_LEVEL_INFO, _T("Initialization of Loader job completed."));
                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Initialization of Loader job completed."));
                save_status();
            }

            try{
                while (!(_is_canceled || _is_interrupted || _state & mgmt_job_state::job_state_error)){
                    get_loader_job_create_detail(_loader_job_create_detail);
                    save_config();
                    if (!_loader_job_create_detail.snapshots.size()){
                        intrrupt_unused_cascading_jobs();
                        LOG(LOG_LEVEL_WARNING, _T("Doesn't have any snapshot for loader job(%s)."), _id.c_str());
                        break;
                    }

                    if (_loader_job_create_detail.block_mode_enable){
                        LOG(LOG_LEVEL_WARNING, _T("It's running in block mode."));
                    }

                    if (_state & mgmt_job_state::job_state_finished){
                        bool is_snapshoted = _current_snapshot.empty();
                        while (!(_is_canceled || _is_interrupted || is_snapshoted)){
                            if (is_snapshoted = check_snapshot(_current_snapshot)){
                                break;
                            }
                            boost::unique_lock<boost::mutex> wait(_wait_lock);
                            _wait_cond.timed_wait(wait, boost::posix_time::seconds(20));
                        }
                        if (is_snapshoted){
                            size_t next_snapshot_index = 0;
                            std::vector<std::string> snapshots = _loader_job_create_detail.snapshots;
                            snapshots.erase(std::unique(snapshots.begin(), snapshots.end()), snapshots.end());
                            for (size_t i = 0; i < snapshots.size(); i++){
                                if (_current_snapshot == snapshots[i]){
                                    next_snapshot_index = i + 1;
                                    break;
                                }
                            }
                            if (next_snapshot_index == snapshots.size()){
                                save_status();
                                break;
                            }
                            else{
                                _state = job_state::type::job_state_initialed;
                                _current_snapshot = snapshots[next_snapshot_index];
                                _progress_map.clear();

                                if (!_loader_job_create_detail.disks_snapshot_mapping.count(_current_snapshot)){
                                    LOG(LOG_LEVEL_WARNING, _T("Incorrect job spec."));
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Incorrect job spec."));
                                    break;
                                }
                                foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[_current_snapshot]){
                                    loader_disk::ptr disk_progress;
                                    if (!_progress_map.count(d.first)){
                                        disk_progress = loader_disk::ptr(new loader_disk(d.first));
                                        _progress_map[d.first] = disk_progress;
                                    }
                                }
                                save_status();
                            }
                        }
                        else{
                            break;
                        }
                    }
                    else{
                        if (!_current_snapshot.empty()){
                            if (std::find(_loader_job_create_detail.snapshots.begin(), _loader_job_create_detail.snapshots.end(), _current_snapshot) == _loader_job_create_detail.snapshots.end()){
                                if (check_snapshot(_current_snapshot)){
                                    _current_snapshot.clear();
                                }
                            }
                        }
                        if (_current_snapshot.empty()){
                            foreach(std::string snapshot_id, _loader_job_create_detail.snapshots){
                                if (!check_snapshot(snapshot_id)){
                                    _current_snapshot = snapshot_id;
                                    _state = mgmt_job_state::job_state_initialed;
                                    _progress_map.clear();
                                    foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[_current_snapshot]){
                                        loader_disk::ptr disk_progress;
                                        if (!_progress_map.count(d.first)){
                                            disk_progress = loader_disk::ptr(new loader_disk(d.first));
                                            _progress_map[d.first] = disk_progress;
                                        }
                                    }
                                    save_status();
                                    break;
                                }
                            }
                        }
                    }

                    if (_current_snapshot.empty()){
                        LOG(LOG_LEVEL_WARNING, _T("Incorrect job spec."));
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Incorrect job spec."));
                        break;
                    }
                    else if (_current_snapshot.length() && !_is_interrupted && !_is_canceled && (_state & mgmt_job_state::job_state_initialed) || (_state & mgmt_job_state::job_state_replicating)){
                        using namespace saasame::ironman::imagex;
                        _is_replicating = false;
                        connection_op::ptr cascading;
                        std::vector<std::wstring> parallels;
                        std::vector<std::wstring> branches;
                        std::map<std::wstring, std::wstring> jobs;
                        get_cascadings_info(_loader_job_create_detail.cascadings, cascading, parallels, branches, jobs);
                        if (cascading)
                            cascading->register_is_cancelled_function(boost::bind(&loader_job::is_canceled, this));
                        intrrupt_unused_cascading_jobs(jobs);

                        foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[_current_snapshot]){
                            loader_disk::ptr disk_progress;
                            if (_progress_map.count(d.first)){
                                disk_progress = _progress_map[d.first];
                            }
                            else{
                                disk_progress = loader_disk::ptr(new loader_disk(d.first));
                                _progress_map[d.first] = disk_progress;
                                save_status();
                            }
                            if (disk_progress->completed || disk_progress->canceled)
                                continue;

                            LOG(LOG_LEVEL_INFO, _T("Replicating snapshot (%s) data from (%s) to disk (%s)"), macho::stringutils::convert_utf8_to_unicode(_current_snapshot).c_str(), macho::stringutils::convert_utf8_to_unicode(d.second).c_str(), macho::stringutils::convert_utf8_to_unicode(d.first).c_str());
                            std::wstring path = diskio_map[d.first];
                            universal_disk_rw::ptr rw;
                            if (_loader_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE){
                                if (!boost::filesystem::exists(path)){
                                    boost::filesystem::path vhd_file = boost::filesystem::path(macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.export_path)) / macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.host_name) / macho::stringutils::convert_utf8_to_unicode(d.second);
                                    switch (_loader_job_create_detail.export_disk_type){
                                    case virtual_disk_type::type::VHDX:
                                        vhd_file = vhd_file.wstring() + (L".vhdx");
                                        break;
                                    case virtual_disk_type::type::VHD:
                                    default:
                                        vhd_file = vhd_file.wstring() + (L".vhd");
                                        break;
                                    }
                                    if (boost::filesystem::exists(vhd_file)){
                                        boost::filesystem::path temp_vhd_file = vhd_file.parent_path() / boost::str(boost::wformat(L"%s.delta%s") % vhd_file.filename().stem().wstring() % vhd_file.extension().wstring());
                                        vhd_disk_info disk_info;
                                        if (!(ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_information(vhd_file.wstring(), disk_info) &&
                                            ERROR_SUCCESS == win_vhdx_mgmt::create_vhdx_file(temp_vhd_file.wstring(), CREATE_VIRTUAL_DISK_FLAG_NONE, disk_info.virtual_size, disk_info.block_size, disk_info.logical_sector_size, disk_info.physical_sector_size, vhd_file.wstring()))){
                                            LOG(LOG_LEVEL_ERROR, L"Failed to create the detal virtual disk file %s", temp_vhd_file.wstring().c_str());
                                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Failed to create the detal virtual disk file %1%") % macho::stringutils::convert_unicode_to_utf8(temp_vhd_file.wstring())));
                                        }
                                        else{
                                            path = temp_vhd_file.wstring();
                                        }
                                    }
                                }
                                rw = win_vhdx_mgmt::open_virtual_disk_for_rw(path, false);
                            }
#ifdef _AZURE_BLOB
                            else if (_loader_job_create_detail.detect_type == disk_detect_type::type::AZURE_BLOB)
                                rw = azure_page_blob_op::open_vhd_page_blob_for_rw(_loader_job_create_detail.azure_storage_connection_string, _loader_job_create_detail.export_path, _loader_job_create_detail.disks_lun_mapping[d.first]);
#endif 
#ifdef _VMWARE_VADP
                            else if (_loader_job_create_detail.detect_type == disk_detect_type::type::VMWARE_VADP){
                                if (conn) {
                                    rw = portal_ex.open_vmdk_for_rw(conn, path, false);
                                }
                            }
#endif
                            else
                                rw = general_io_rw::open(path, false);
                            if (rw){
                                bool is_canceled_snapshot = false;
                                foreach(loader_disk::map::value_type& l, _progress_map){
                                    if (l.second->canceled){
                                        is_canceled_snapshot = true;
                                        break;
                                    }
                                }
                                _execute<loader_job, replication_task>(d.second, conn_ptr, disk_progress, rw, is_canceled_snapshot, disksize_map.count(d.first) ? disksize_map[d.first] : 0, cascading, parallels, branches, false);
                            }
                            else{
                                LOG(LOG_LEVEL_ERROR, _T("Cannot open disk (%s, %s) for data replication."), macho::stringutils::convert_utf8_to_unicode(d.first).c_str(), path.c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_JOB_REPLICATE_FAIL, (record_format("Cannot open disk %1% for data replication.") % d.first));
                                _state |= mgmt_job_state::job_state_error;
                                save_status();
                                break;
                            }
                            save_status();
                            if (_is_interrupted || _is_canceled)
                                break;
                        }
                        bool is_finished, is_completed = true;
                        bool is_cdr = false;
                        foreach(loader_disk::map::value_type& l, _progress_map){
                            if (!(l.second->completed || l.second->canceled)){
                                if (!(l.second->completed))
                                    is_completed = false;
                                is_finished = false;
                            }
                            else if (l.second->canceled)
                                is_completed = false;
                            if (l.second->cdr)
                                is_cdr = true;
                        }
                        if (is_completed){
                            _state = is_cdr ? mgmt_job_state::job_state_cdr_replicated : mgmt_job_state::job_state_replicated;
                            save_status();
                            LOG(LOG_LEVEL_INFO, _T("Replicated all %s (%s) data to disk(s)"), is_cdr ? "changed" : "snapshot", macho::stringutils::convert_ansi_to_unicode(_current_snapshot).c_str());
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Replication of %1% data completed.") % (is_cdr ? "changed" : "snapshot")));
                        }
                        else if (is_finished){
                            _state = mgmt_job_state::job_state_discard;
                            save_status();
                        }
                        else if (!_is_replicating){
                            _is_interrupted = true;
                            break;
                        }
                    }

                    if (_state & mgmt_job_state::job_state_replicated || _state & mgmt_job_state::job_state_snapshot_created){
                        if (!_update(true)){
                            save_status();
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::EXPORT_IMAGE){
                            bool is_merge = true;
                            boost::filesystem::path lock_file = boost::filesystem::path(macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.export_path)) / macho::stringutils::convert_utf8_to_unicode(_loader_job_create_detail.host_name) / "export";
                            macho::windows::file_lock_ex lck(lock_file, "l");
                            macho::windows::auto_lock lock(lck);
                            typedef std::map<std::string, std::wstring> _disk_io_map_type;
                            foreach(_disk_io_map_type::value_type p, diskio_map){
                                if (p.second.find(L".delta") != std::wstring::npos){
                                    if (ERROR_SUCCESS != win_vhdx_mgmt::merge_virtual_disk(p.second)){
                                        is_merge = false;
                                        break;
                                    }
                                    else{
                                        boost::filesystem::remove(p.second);
                                    }
                                }
                            }
                            if (is_merge){
                                if (take_snapshot(_current_snapshot)){
                                    _state = mgmt_job_state::job_state_finished;
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Snapshot creation completed."));
                                    save_status();
                                    LOG(LOG_LEVEL_INFO, _T("Snapshot creation completed."));
                                    execute_snapshot_post_script();
                                }
                            }
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::AZURE_BLOB){
                            bool is_snapshot = false;
#ifdef _AZURE_BLOB
                            if (_state & mgmt_job_state::job_state_snapshot_created){
                                is_snapshot = true;
                            }
                            else{
                                azure_page_blob_op::ptr op_ptr = azure_page_blob_op::open(_loader_job_create_detail.azure_storage_connection_string);
                                if (op_ptr){
                                    foreach(string_map::value_type &d, _loader_job_create_detail.disks_lun_mapping){
                                        if (!(is_snapshot = op_ptr->create_vhd_page_blob_snapshot(_loader_job_create_detail.export_path, d.second, _current_snapshot))){
                                            break;
                                        }
                                    }
                                }
                            }
#endif
                            if (is_snapshot){
                                _state = mgmt_job_state::job_state_snapshot_created;
                                save_status();
                                if (take_snapshot(_current_snapshot)){
                                    _state = mgmt_job_state::job_state_finished;
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Snapshot creation completed."));
                                    save_status();
                                    LOG(LOG_LEVEL_INFO, _T("Snapshot creation completed."));
                                    execute_snapshot_post_script();
                                }
                            }
                        }
                        else if (_loader_job_create_detail.detect_type == disk_detect_type::type::VMWARE_VADP){
                            bool is_snapshot = false;
#ifdef _VMWARE_VADP
                            if (_state & mgmt_job_state::job_state_snapshot_created){
                                is_snapshot = true;
                            }
                            else{
                                is_snapshot = S_OK == portal_ex.create_temp_snapshot(vm, macho::stringutils::convert_utf8_to_unicode(_current_snapshot));
                            }
#endif
                            if (is_snapshot){
                                _state = mgmt_job_state::job_state_snapshot_created;
                                save_status();
                                if (take_snapshot(_current_snapshot)){
                                    _state = mgmt_job_state::job_state_finished;
                                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Snapshot creation completed."));
                                    save_status();
                                    LOG(LOG_LEVEL_INFO, _T("Snapshot creation completed."));
                                    execute_snapshot_post_script();
                                }
                            }
                        }
                        else if (take_snapshot(_current_snapshot)){
                            _state = mgmt_job_state::job_state_finished;
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Snapshot creation completed."));
                            save_status();
                            LOG(LOG_LEVEL_INFO, _T("Snapshot creation completed."));
                            execute_snapshot_post_script();
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("Cannot create snapshot."));
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot create snapshot."));
                            _state |= mgmt_job_state::job_state_error;
                            save_status();
                        }
                    }
                    else if (_state & mgmt_job_state::job_state_discard){
                        if (!_update(true)){
                            save_status();
                        }
                        else if (discard_snapshot(_current_snapshot)){
                            _state = mgmt_job_state::job_state_finished;
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Snapshot discarded."));
                            save_status();
                            LOG(LOG_LEVEL_INFO, _T("Snapshot discarded."));
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("Cannot discard snapshot."));
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_SNAPSHOT_CREATE_FAIL, record_format("Cannot discard snapshot."));
                            _state |= mgmt_job_state::job_state_error;
                            save_status();
                        }
                    }
                    else if (_state & mgmt_job_state::job_state_cdr_replicated){
                        if (!_update(true)){
                            save_status();
                        }
                        else if (discard_snapshot(_current_snapshot)){
                            _state = mgmt_job_state::job_state_finished;
                            save_status();
                            LOG(LOG_LEVEL_INFO, _T("Commit cdr data ."));
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, _T("Cannot commit cdr data."));
                            _state |= mgmt_job_state::job_state_error;
                            save_status();
                        }
                    }
                {
                    boost::unique_lock<boost::mutex> wait(_wait_lock);
                    _wait_cond.timed_wait(wait, boost::posix_time::seconds(30));
                }
                //int count = 10;
                //while (count > 0 && (!(_is_interrupted || _is_canceled))){
                //    boost::this_thread::sleep(boost::posix_time::seconds(3));
                //    count--;
                //}
                save_status();
                }
            }
            catch (loader_job::exception &ex){

            }
            catch (...){

            }
            _terminated = true;
            if (thread.joinable())
                thread.join();
        }
    }
    /*    
    if (_state & mgmt_job_state::job_state_finished){
        LOG(LOG_LEVEL_INFO, _T("Job completed."));
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, "Job completed.");
    }
    else 
    */
    if (_is_canceled){
        LOG(LOG_LEVEL_WARNING, _T("Job cancelled."));
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_JOB_CANCELLED, record_format("Job cancelled."));
    }
    else if (_is_interrupted){
        LOG(LOG_LEVEL_WARNING, _T("Job interrupted."));
        //record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_JOB_INTERRUPTED,  record_format("Job interrupted."));
    }
    _latest_leave_time = boost::posix_time::microsec_clock::universal_time();
    save_status();
    _update(false);
}

trigger::vtr loader_job::get_triggers(){
    trigger::vtr triggers;
    FUN_TRACE;
    foreach(saasame::transport::job_trigger t, _create_job_detail.triggers){
        trigger::ptr _t;
        switch (t.type){
            case saasame::transport::job_trigger_type::interval_trigger:{
                if (t.start.length() && t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish), _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                else if (t.start.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::time_from_string(t.start), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time, _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                else if (t.finish.length()){
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::posix_time::time_from_string(t.finish), _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                else{
                    _t = trigger::ptr(new interval_trigger_ex(boost::posix_time::minutes(t.interval), 0, boost::posix_time::microsec_clock::universal_time(), boost::posix_time::minutes(t.duration), boost::date_time::not_a_date_time, _latest_leave_time == boost::date_time::not_a_date_time ? _latest_enter_time : _latest_leave_time));
                }
                break;
            }
            case saasame::transport::job_trigger_type::runonce_trigger:{
                if (t.start.length()){
                    _t = trigger::ptr(new run_once_trigger(boost::posix_time::time_from_string(t.start)));
                }
                else{
                    _t = trigger::ptr(new run_once_trigger());
                }
                break;
            }
        }
        triggers.push_back(_t);
    }
    if (_is_continuous_data_replication){
        _enable_cdr_trigger = true;
        triggers.push_back(trigger::ptr(new continues_data_replication_trigger()));
    }
    else{
        _enable_cdr_trigger = false;
    }
    return triggers;
}

void loader_job::modify_job_scheduler_interval(uint32_t minutes){
    FUN_TRACE;
    thrift_connect<loader_serviceClient>  thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open()){
        try{
            saasame::transport::job_trigger t;
            if (_create_job_detail.triggers.size()){
                t = _create_job_detail.triggers.at(0);
                _create_job_detail.triggers.clear();
                t.interval = minutes;
            }
            else{
                t.id = guid_::create();
                t.start = boost::posix_time::to_simple_string(_created_time);
                t.type = saasame::transport::job_trigger_type::interval_trigger;
                t.interval = minutes;
            }
            _create_job_detail.triggers.push_back(t);
            _create_job_detail.__set_triggers(_create_job_detail.triggers);
            _create_job_detail.__set_is_ssl(_create_job_detail.is_ssl);
            _create_job_detail.__set_management_id(_create_job_detail.management_id);
            _create_job_detail.__set_mgmt_addr(_create_job_detail.mgmt_addr);
            _create_job_detail.__set_mgmt_port(_create_job_detail.mgmt_port);
            _create_job_detail.__set_triggers(_create_job_detail.triggers);
            _create_job_detail.__set_type(_create_job_detail.type);
            thrift.client()->update_job("", macho::stringutils::convert_unicode_to_utf8(_id), _create_job_detail);
        }
        catch (invalid_operation& ex){
        }
        catch (apache::thrift::TException& tx) {
        }
        thrift.close();
    }
}

void loader_job::execute_snapshot_post_script(){
    // Add snapshot post action for professional services.
    boost::filesystem::path post_script = boost::str(boost::wformat(L"%s.cmd") % _config_file.wstring());
    if (boost::filesystem::exists(post_script)){
        try{
            std::wstring command, ret;
            command = boost::str(boost::wformat(L"%s %s") % post_script.wstring() % macho::stringutils::convert_utf8_to_unicode(_current_snapshot));
            LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
            bool result = process::exec_console_application_with_timeout(command, ret);
            LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
        }
        catch (const boost::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        }
    }
}

void loader_job::disable_auto_mount_disk_devices(){
    registry reg;
    if (reg.open(L"System\\CurrentControlSet\\Services\\mountmgr")){
        reg[L"NoAutoMount"] = (DWORD)0x1;
    }
    if (reg.open(L"System\\CurrentControlSet\\Services\\partmgr\\Parameters")){
        reg[L"SanPolicy"] = (DWORD)0x3;
    }
}

void loader_job::intrrupt_unused_cascading_jobs(){
    connection_op::ptr cascading;
    std::vector<std::wstring> parallels;
    std::vector<std::wstring> branches;
    std::map<std::wstring, std::wstring> jobs;
    get_cascadings_info(_loader_job_create_detail.cascadings, cascading, parallels, branches, jobs);
    intrrupt_unused_cascading_jobs(jobs);
}

void loader_job::intrrupt_unused_cascading_jobs(std::map<std::wstring, std::wstring>& jobs){
    macho::job::vtr cascading_jobs = _handler.get_cascading_jobs(_id);
    foreach(macho::job::ptr &j, cascading_jobs){
        if (!jobs.count(j->id())){
            interruptable_job* _interrupt_job = dynamic_cast<interruptable_job*>(j.get());
            if (_interrupt_job != NULL)
                _interrupt_job->interrupt();
        }
    }
}

class local_connection_op : virtual public connection_op{
public:
    virtual connection_op_type              type(){ return connection_op_type::local; }
    virtual bool                            remove_file(std::wstring filename){
        FUN_TRACE;
        bool result = false;
        try{
            boost::filesystem::path p = _folder / filename;
            if (boost::filesystem::exists(p))
                result = boost::filesystem::remove(p);
        }
        catch (const boost::filesystem::filesystem_error& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (const boost::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
        }
        return result;
    }
    virtual bool                            is_file_ready(std::wstring filename){
        return boost::filesystem::exists(_folder / filename);
    }
    
    virtual bool                            read_file(std::wstring filename, std::ifstream& ifs){
        try{
            boost::filesystem::path input_file_path = _folder / filename;
            if (boost::filesystem::exists(input_file_path)){
                std::ifstream().swap(ifs);
                ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
                ifs.open(input_file_path.wstring(), std::ios::in | std::ios::binary, _SH_DENYRW);
                return ifs.is_open();
            }
        }
        catch (...){
        }
        return false;
    }

    virtual bool                            read_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data, uint32_t& size, connection_op::ptr& cascading){
        FUN_TRACE;
        std::ifstream ifs;
        bool result = false;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        if (boost::filesystem::exists(boost::filesystem::path(_folder / filename))){
            ifs.open(boost::filesystem::path(_folder / filename).string(), std::ios::in | std::ios::binary, _SH_DENYWR);
            if (result = ifs.is_open()){
                if (cascading){
                    result = cascading->write_file(filename, ifs);
                }
                if (result){
                    result = data.load(ifs);
                    ifs.seekg(0, std::ios::end);
                    size = ifs.tellg();
                }
            }
        }
        return result;
    }

    virtual bool                            read_file(std::wstring filename, std::stringstream& data){
        FUN_TRACE;
        std::ifstream ifs;
        bool result = false;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        if (boost::filesystem::exists(boost::filesystem::path(_folder / filename))){
            ifs.open(boost::filesystem::path(_folder / filename).string(), std::ios::in | std::ios::binary, _SH_DENYWR);
            if (result = ifs.is_open())
                data << ifs.rdbuf();
        }
        return result;
    }
    virtual macho::windows::lock_able::ptr  get_lock_object(std::wstring filename, std::string flags){
        macho::windows::file_lock_ex* l = new macho::windows::file_lock_ex(_folder / filename, flags);
        l->register_is_cancelled_function(boost::bind(&local_connection_op::is_canceled, this));
        return macho::windows::lock_able::ptr(l);
    }

    virtual bool                            write_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data){
        FUN_TRACE;
        bool result = false;
        boost::filesystem::path output_file_name = _folder / filename;
        bool is_overwrite = (boost::filesystem::exists(output_file_name));
        boost::filesystem::path temp = output_file_name.string() + (is_overwrite ? ".tmp" : "");
        try{
            std::ofstream ofs;
            ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);
            ofs.open(temp.wstring(), std::ios::trunc | std::ios::out | std::ios::binary, _SH_DENYRW);
            if (ofs.is_open()){
                if (result = data.save(ofs)){
                    ofs.close();
                    if (is_overwrite && !MoveFileEx(temp.wstring().c_str(), output_file_name.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                        LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), output_file_name.wstring().c_str(), GetLastError());
                        result = false;
                    }
                }
            }
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            result = false;
        }
        if (!result){
            try{
                boost::filesystem::remove(temp);
            }
            catch (boost::filesystem::filesystem_error &err){
                LOG(LOG_LEVEL_ERROR, L"Cannot remove '%s' : %s", temp.wstring().c_str(), macho::stringutils::convert_utf8_to_unicode(err.what()).c_str());
            }
            catch (...){
            }
        }
        return result;
    }

    virtual bool                            write_file(std::wstring filename, std::istream& data){
        FUN_TRACE;
        boost::filesystem::space_info _space_info = boost::filesystem::space(_folder);
        if ((uint64_t)_space_info.available > (uint64_t)_reserved_space){
            bool result = false;
            boost::filesystem::path output_file_name = _folder / filename;
            bool is_overwrite = (boost::filesystem::exists(output_file_name));
            boost::filesystem::path temp = output_file_name.string() + (is_overwrite ? ".tmp" : "");
            try{
                std::ofstream ofs;
                ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);
                ofs.open(temp.wstring(), std::ios::trunc | std::ios::out | std::ios::binary, _SH_DENYRW);
                if (ofs.is_open()){
                    data.seekg(0, std::ios::end);
                    if (data.tellg() > 0){
                        data.seekg(0, std::ios::beg);
                        ofs << data.rdbuf();
                    }
                    result = true;
                    ofs.close();
                    if (is_overwrite && !MoveFileEx(temp.wstring().c_str(), output_file_name.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                        LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), output_file_name.wstring().c_str(), GetLastError());
                        result = false;
                    }
                }
            }
            catch (...){
                result = false;
            }
            if (!result){
                try{
                    boost::filesystem::remove(temp);
                }
                catch (boost::filesystem::filesystem_error &err){
                    LOG(LOG_LEVEL_ERROR, L"Cannot remove '%s' : %s", temp.wstring().c_str(), macho::stringutils::convert_utf8_to_unicode(err.what()).c_str());
                }
                catch (...){
                }
            }
            return result;
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Can't write the file(%s), Available space %I64u bytes < %I64u bytes", filename.c_str(), _space_info.available, _reserved_space);
        }
        return false;
    }

    local_connection_op(const saasame::transport::connection& conn, boost::filesystem::path folder) : connection_op(conn), _folder(folder), _reserved_space(((boost::uintmax_t)2) * 1024 * 1024 * 1024) {}
    virtual connection_op::ptr              clone() { return connection_op::ptr(new local_connection_op(_conn, _folder)); }
    virtual std::wstring                    path() const { return _folder.wstring(); }
private:
    boost::filesystem::path  _folder;
    boost::uintmax_t         _reserved_space;
};

class net_connection_op : virtual public connection_op{
public:
    virtual connection_op_type              type(){ return connection_op_type::winnet; }
    virtual bool                            remove_file(std::wstring filename){
        FUN_TRACE;
        bool result = false;
        try{
            boost::filesystem::path p = boost::filesystem::path(_wnet->path()) / filename;
            if (boost::filesystem::exists(p))
                result = boost::filesystem::remove(p);
            else
                result = true;
        }
        catch (const boost::filesystem::filesystem_error& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (const boost::exception &ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        }
        catch (const std::exception& ex){
            LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
        catch (...){
        }
        return result;
    }
    virtual bool                            is_file_ready(std::wstring filename){
        return boost::filesystem::exists(boost::filesystem::path(_wnet->path()) / filename);
    }
    
    virtual bool                            read_file(std::wstring filename, std::ifstream& ifs){
        try{
            boost::filesystem::path input_file_path = boost::filesystem::path(_wnet->path()) / filename;
            if (boost::filesystem::exists(input_file_path)){
                std::ifstream().swap(ifs);
                ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
                ifs.open(input_file_path.wstring(), std::ios::in | std::ios::binary, _SH_DENYRW);
                return ifs.is_open();
            }
        }
        catch (...){
        }
        return false;
    }

    virtual bool                            read_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data, uint32_t& size, connection_op::ptr& cascading){
        FUN_TRACE;
        std::ifstream ifs;
        bool result = false;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        if (boost::filesystem::exists(boost::filesystem::path(boost::filesystem::path(_wnet->path()) / filename))){
            ifs.open(boost::filesystem::path(boost::filesystem::path(_wnet->path()) / filename).string(), std::ios::in | std::ios::binary, _SH_DENYWR);
            if (result = ifs.is_open()){
                if (cascading){
                    result = cascading->write_file(filename, ifs);
                }
                if (result){
                    result = data.load(ifs);
                    ifs.seekg(0, std::ios::end);
                    size = ifs.tellg();
                }
            }
        }
        return result;
    }
    virtual bool                            read_file(std::wstring filename, std::stringstream& data){
        FUN_TRACE;
        std::ifstream ifs;
        bool result = false;
        ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        if (boost::filesystem::exists(boost::filesystem::path(boost::filesystem::path(_wnet->path()) / filename))){
            ifs.open(boost::filesystem::path(boost::filesystem::path(_wnet->path()) / filename).string(), std::ios::in | std::ios::binary, _SH_DENYWR);
            if (result = ifs.is_open())
                data << ifs.rdbuf();
        }
        return result;
    }
    virtual macho::windows::lock_able::ptr  get_lock_object(std::wstring filename, std::string flags){
        macho::windows::file_lock_ex* l = new macho::windows::file_lock_ex(boost::filesystem::path(_wnet->path()) / filename, flags);
        l->register_is_cancelled_function(boost::bind(&net_connection_op::is_canceled, this));
        return macho::windows::lock_able::ptr(l);
    }

    virtual bool                            write_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data){
        FUN_TRACE;
        bool result = false;
        boost::filesystem::path output_file_name = boost::filesystem::path(_wnet->path()) / filename;
        bool is_overwrite = (boost::filesystem::exists(output_file_name));
        boost::filesystem::path temp = output_file_name.string() + (is_overwrite ? ".tmp" : "");
        try{
            std::ofstream ofs;
            ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);
            ofs.open(temp.wstring(), std::ios::trunc | std::ios::out | std::ios::binary, _SH_DENYRW);
            if (ofs.is_open()){
                if (result = data.save(ofs)){
                    ofs.close();
                    if (is_overwrite && !MoveFileEx(temp.wstring().c_str(), output_file_name.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                        LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), output_file_name.wstring().c_str(), GetLastError());
                        result = false;
                    }
                }
            }
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Unknown exception");
            result = false;
        }
        if (!result){
            try{
                boost::filesystem::remove(temp);
            }
            catch (boost::filesystem::filesystem_error &err){
                LOG(LOG_LEVEL_ERROR, L"Cannot remove '%s' : %s", temp.wstring().c_str(), macho::stringutils::convert_utf8_to_unicode(err.what()).c_str());
            }
            catch (...){
            }
        }
        return result;
    }

    virtual bool                            write_file(std::wstring filename, std::istream& data){
        FUN_TRACE;
        bool result = false;
        boost::filesystem::path output_file_name = boost::filesystem::path(_wnet->path()) / filename;
        bool is_overwrite = (boost::filesystem::exists(output_file_name));
        boost::filesystem::path temp = output_file_name.string() + (is_overwrite ? ".tmp" : "");
        try{
            std::ofstream ofs;
            ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);
            ofs.open(temp.wstring(), std::ios::trunc | std::ios::out | std::ios::binary, _SH_DENYRW);
            if (ofs.is_open()){
                data.seekg(0, std::ios::end);
                if (data.tellg() > 0){
                    data.seekg(0, std::ios::beg);
                    ofs << data.rdbuf();
                }
                result = true;
                ofs.close();
                if (is_overwrite && !MoveFileEx(temp.wstring().c_str(), output_file_name.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                    LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), output_file_name.wstring().c_str(), GetLastError());
                    result = false;
                }
            }
        }
        catch (...){
            result = false;
        }
        if (!result){
            try{
                boost::filesystem::remove(temp);
            }
            catch (boost::filesystem::filesystem_error &err){
                LOG(LOG_LEVEL_ERROR, L"Cannot remove '%s' : %s", temp.wstring().c_str(), macho::stringutils::convert_utf8_to_unicode(err.what()).c_str());
            }
            catch (...){
            }
        }
        return result;
    }

    net_connection_op(const saasame::transport::connection& conn, macho::windows::wnet_connection::ptr &wnet) : connection_op(conn), _wnet(wnet){}
    virtual connection_op::ptr              clone() { return connection_op::ptr(new net_connection_op(_conn, _wnet)); }
    virtual std::wstring                    path() const { return _wnet ? _wnet->path() : L""; }
private:
    macho::windows::wnet_connection::ptr    _wnet;
};

template <class Type_ptr, class Type_queue, class Type_lock> 
class remote_connection_op : virtual public connection_op{
public:
    virtual connection_op_type              type(){ return connection_op_type::remote; }
    virtual bool                            remove_file(std::wstring filename){
        bool result = false;
        std::string name = ::stringutils::convert_unicode_to_ansi(filename);
        Type_ptr remote = _pop_front();
        if (remote){
            if (remote->is_existing(name))
                result = remote->remove(name);
            else
                result = true;
            _push_front(remote);
        }    
        return result;
    }
    virtual bool                            is_file_ready(std::wstring filename){
        bool result = false;
        Type_ptr remote = _pop_front();
        if (remote){
            result = remote->is_existing(macho::stringutils::convert_unicode_to_ansi(filename));
            _push_front(remote);
        }
        return result;
    }
    
    virtual bool                            read_file(std::wstring filename, std::ifstream& ifs){
        return false;
    }

    virtual bool                            read_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data, uint32_t& size, connection_op::ptr& cascading){
        bool result = false;
        std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
        if (result = read_file(filename, buffer)){
            if (cascading){
                result = cascading->write_file(filename, buffer);
            }
            if (result){
                result = data.load(buffer);
                buffer.seekg(0, std::ios::end);
                size = buffer.tellg();
            }
        }
        return result;
    }
    virtual bool                            read_file(std::wstring filename, std::stringstream& data){
        bool result = false;
        Type_ptr remote = _pop_front();
        if (remote){
            result = remote->get_file(macho::stringutils::convert_unicode_to_ansi(filename), data);
            _push_front(remote);
        }
        return result;
    }
    virtual macho::windows::lock_able::ptr  get_lock_object(std::wstring filename, std::string flags){
        Type_lock* l = new Type_lock(filename, _remote, flags);
        l->register_is_cancelled_function(boost::bind(&remote_connection_op::is_canceled, this));
        return macho::windows::lock_able::ptr(l);
    }
    remote_connection_op(const saasame::transport::connection& conn, Type_ptr& remote, int worker_thread_number = 1) : connection_op(conn), _remote(remote), _count(worker_thread_number){
        int      worker_threads = NumberOfWorkers;
        if (worker_thread_number > 0)
            worker_threads = worker_thread_number;
        for (int i = 0; i < worker_threads; i++)
            _queue.push_back(_remote->clone());
    }
    connection_op::ptr clone(){
        connection_op::ptr p;
        Type_ptr remote = _pop_front();
        if (remote){
            p = connection_op::ptr(new remote_connection_op<Type_ptr, Type_queue, Type_lock>(_conn, remote, _count));
            _push_front(remote);
        }
        return p;
    }

    virtual bool                            write_file(std::wstring filename, saasame::ironman::imagex::irm_imagex_base& data){
        FUN_TRACE;
        bool result = false;
        Type_ptr remote = _pop_front();
        if (remote){
            std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
            if (data.save(buffer))
                result = remote->put_file(macho::stringutils::convert_unicode_to_utf8(filename), buffer);
            _push_front(remote);
        }
        return result;
    }

    virtual bool                            write_file(std::wstring filename, std::istream& data){
        FUN_TRACE;
        bool result = false;
        Type_ptr remote = _pop_front();
        if (remote){
            result = remote->put_file(macho::stringutils::convert_unicode_to_utf8(filename), data);
            _push_front(remote);
        }
        return result;
    }
    virtual std::wstring                    path() const { return L""; }
private:
    Type_ptr _pop_front(){
        if (_count > 1){
            macho::windows::auto_lock lock(_cs);
            Type_ptr remote = _queue.front();
            if (remote){
                _queue.pop_front();
            }
            else{
                remote = _remote->clone();
            }
            return remote;
        }
        else{
            return _queue.front();
        }
    }

    void _push_front(Type_ptr remote){
        if (_count > 1){
            macho::windows::auto_lock lock(_cs);
            _queue.push_front(remote);
        }
    }
    macho::windows::critical_section      _cs;
    Type_queue                            _queue;
    Type_ptr                              _remote;
    int                                   _count;
};

connection_op::ptr  connection_op::get(const std::string connection_id, int worker_thread_number){
    FUN_TRACE;
    connection_op::ptr conn_ptr;
    thrift_connect<loader_serviceClient>  thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open()){
        saasame::transport::connection          conn;
        try{
            thrift.client()->get_connection(conn, "", connection_id);
            conn_ptr = get(conn, worker_thread_number);
        }
        catch (invalid_operation& ex){
        }
        catch (apache::thrift::TException& tx) {
        }
        thrift.close();
    }
    return conn_ptr;
}

bool  connection_op::remove(const std::string connection_id){
    FUN_TRACE;
    bool ret = false;
    thrift_connect<loader_serviceClient>  thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open()){
        saasame::transport::connection          conn;
        try{
            thrift.client()->get_connection(conn, "", connection_id);
            if (conn.type != saasame::transport::connection_type::LOCAL_FOLDER){
                if (ret = thrift.client()->remove_connection("", connection_id)){
                    LOG(LOG_LEVEL_WARNING, L"Remove unused connection %s", macho::stringutils::convert_ansi_to_unicode(connection_id).c_str());
                }
            }
        }
        catch (invalid_operation& ex){
            ret = true;
        }
        catch (apache::thrift::TException& tx) {
        }
    }
    return ret;
}

connection_op::ptr  connection_op::get(const saasame::transport::connection &conn, int worker_thread_number){
    FUN_TRACE;
    connection_op::ptr c ;
    switch (conn.type){
    case saasame::transport::connection_type::LOCAL_FOLDER:{
        std::string folder;
        if (conn.options.count("folder")){
            folder = conn.options.at("folder");
        }
        else{
            folder = conn.detail.local.path;
        }
        c = connection_op::ptr(new local_connection_op(conn, folder));
        break;
    }
    case connection_type::LOCAL_FOLDER_EX:
        c = connection_op::ptr(new local_connection_op(conn, conn.detail.remote.path));
        break;
    case saasame::transport::connection_type::NFS_FOLDER:{
        macho::windows::wnet_connection::ptr wnet =
            macho::windows::wnet_connection::connect(
            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.path),
            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.username),
            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.password), false);
        if (wnet) c = connection_op::ptr(new net_connection_op(conn, wnet));      
        }
        break;
    case saasame::transport::connection_type::S3_BUCKET:
    case saasame::transport::connection_type::S3_BUCKET_EX:{
        aws_s3::ptr s3 = aws_s3::connect(conn.detail.remote.path,
            conn.detail.remote.username,
            conn.detail.remote.password,
            conn.detail.remote.s3_region,
            conn.detail.remote.proxy_host,
            conn.detail.remote.proxy_port,
            conn.detail.remote.proxy_username,
            conn.detail.remote.proxy_password, 
            conn.detail.remote.timeout);
        if (s3) c = connection_op::ptr(new remote_connection_op<aws_s3::ptr, aws_s3::queue, aws_s3::lock_ex>(conn, s3, worker_thread_number));
        }
        break;
    case saasame::transport::connection_type::WEBDAV_WITH_SSL:
    case saasame::transport::connection_type::WEBDAV_EX:{
        webdav::ptr dav = webdav::connect(conn.detail.remote.path,
            conn.detail.remote.username,
            conn.detail.remote.password,
            conn.detail.remote.port,
            conn.detail.remote.proxy_host,
            conn.detail.remote.proxy_port,
            conn.detail.remote.proxy_username,
            conn.detail.remote.proxy_password, 
            conn.detail.remote.timeout);
        if (dav) c = connection_op::ptr(new remote_connection_op<webdav::ptr, webdav::queue, webdav::lock_ex>(conn, dav, worker_thread_number));
        }
        break;
    }
    return c;
}

connection_op::ptr connection_op::get_cascading(const saasame::transport::connection &conn, int workthread){
    FUN_TRACE;
    connection_op::ptr c;
    switch (conn.type){
        case saasame::transport::connection_type::LOCAL_FOLDER:
        case saasame::transport::connection_type::WEBDAV_EX:
        case saasame::transport::connection_type::S3_BUCKET_EX:
        case connection_type::LOCAL_FOLDER_EX:
        {
            std::string folder;
            if (conn.options.count("folder")){
                folder = conn.options.at("folder");
            }
            else{
                folder = conn.detail.local.path;
            }
            c = connection_op::ptr(new local_connection_op(conn, folder));
            break;
        }
    }
    return c;
}

saasame::transport::cascading loader_job::load_cascadings(const json_spirit::mObject& cascadings){
    saasame::transport::cascading _cascadings;
    _cascadings.level = find_value_int32(cascadings, "level");
    _cascadings.machine_id = find_value_string(cascadings, "machine_id");
    json_spirit::mValue conn = find_value(cascadings, "connection");
    _cascadings.connection_info = _handler.load_connection(conn);
    json_spirit::mArray branches;
    foreach(json_spirit::mValue c, branches){
        saasame::transport::cascading _c = load_cascadings(c.get_obj());
        _cascadings.branches[_c.machine_id] = _c;
    }
    return _cascadings;
}

json_spirit::mObject loader_job::save_cascadings(const saasame::transport::cascading& cascadings){
    json_spirit::mObject _cascadings;
    typedef std::map<std::string, cascading> cascading_map_type;
    _cascadings["level"] = cascadings.level;
    _cascadings["machine_id"] = cascadings.machine_id;
    _cascadings["connection"] = _handler.save_connection(cascadings.connection_info);
    mArray branches;
    foreach(cascading_map_type::value_type c, cascadings.branches){
        branches.push_back(save_cascadings(c.second));
    }
    _cascadings["branches"] = branches;
    return _cascadings;
}

void loader_job::get_cascadings_info(saasame::transport::cascading& cascadings, connection_op::ptr& cascading_ptr, std::vector<std::wstring>& parallels, std::vector<std::wstring>& branches, std::map<std::wstring, std::wstring>& jobs){
    typedef std::map<std::string, cascading> cascading_map_type;
    if (cascadings.machine_id == _machine_id){
        foreach(cascading_map_type::value_type c, cascadings.branches){
            branches.push_back(macho::stringutils::convert_ansi_to_unicode(c.second.machine_id));
            jobs[macho::stringutils::convert_ansi_to_unicode(c.first)] = macho::stringutils::convert_ansi_to_unicode(c.second.machine_id);
            _handler.create_cascading_job(c.first, c.second.machine_id, macho::stringutils::convert_unicode_to_utf8(_id), _create_job_detail);
        }
        saasame::transport::connection _conn;
        _handler.get_connection(_conn, "", _loader_job_create_detail.connection_id);
        if (_conn.type != saasame::transport::connection_type::LOCAL_FOLDER && !branches.empty()){
            cascading_ptr = connection_op::get_cascading(cascadings.branches.begin()->second.connection_info);
        }
    }
    else{
        foreach(cascading_map_type::value_type c, cascadings.branches){
            get_cascadings_info(c.second, cascading_ptr, parallels, branches, jobs);
            if (!branches.empty())
                break;
        }
        if (cascadings.branches.count(macho::stringutils::convert_unicode_to_utf8(_id))){
            saasame::transport::connection _conn;
            _handler.get_connection(_conn, "", _loader_job_create_detail.connection_id);
            if (_conn.type != saasame::transport::connection_type::LOCAL_FOLDER){
                foreach(cascading_map_type::value_type c, cascadings.branches){
                    if (c.second.machine_id != _machine_id){
                        parallels.push_back(macho::stringutils::convert_ansi_to_unicode(c.second.machine_id));
                        jobs[macho::stringutils::convert_ansi_to_unicode(c.first)] = macho::stringutils::convert_ansi_to_unicode(c.second.machine_id);
                        _handler.create_cascading_job(c.first, c.second.machine_id, macho::stringutils::convert_unicode_to_utf8(_id), _create_job_detail);
                    }
                }
                if (!cascadings.machine_id.empty() && !cascadings.connection_info.id.empty())
                    parallels.push_back(macho::stringutils::convert_ansi_to_unicode(cascadings.machine_id));
            }
        }
    }
}

cascading_job::cascading_job(loader_service_handler& handler, std::wstring id, std::wstring machine_id, std::wstring loader_job_id, saasame::transport::create_job_detail detail) : loader_job(handler, id, detail)  {
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % CASCADING_JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % CASCADING_JOB_EXTENSION);
    _progress_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.progress") % _id % CASCADING_JOB_EXTENSION);
    _machine_id = macho::stringutils::convert_unicode_to_utf8(machine_id);
    _group = loader_job_id;
}

cascading_job::ptr cascading_job::create(loader_service_handler& handler, std::string id, std::string machine_id, std::string loader_job_id, saasame::transport::create_job_detail detail){
    cascading_job::ptr job;
    job = cascading_job::ptr(new cascading_job(handler, macho::stringutils::convert_utf8_to_unicode(id), macho::stringutils::convert_utf8_to_unicode(machine_id), macho::stringutils::convert_utf8_to_unicode(loader_job_id), detail));
    job->_created_time = boost::posix_time::microsec_clock::universal_time();
    job->_latest_update_time = job->_created_time;
    return job;
}

cascading_job::ptr cascading_job::load(loader_service_handler& handler, boost::filesystem::path config_file, boost::filesystem::path status_file){
    cascading_job::ptr job;
    std::wstring id, machine_id, loader_job_id;
    saasame::transport::create_job_detail _create_job_detail = load_config(config_file.wstring(), id, machine_id, loader_job_id);
    job = cascading_job::ptr(new cascading_job(handler, id, machine_id, loader_job_id, _create_job_detail));
    job->load_create_job_detail(config_file.wstring());
    job->load_status(status_file.wstring());
    return job;
}

saasame::transport::create_job_detail cascading_job::load_config(std::wstring config_file, std::wstring &job_id, std::wstring &machine_id, std::wstring &loader_job_id){
    saasame::transport::create_job_detail detail;
    try{
        std::ifstream is(config_file);
        mValue job_config;
        read(is, job_config);
        mValue _create_job_detail = find_value(job_config.get_obj(), "create_job_detail").get_obj();
        job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(_create_job_detail.get_obj(), "id").get_str());
        machine_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(_create_job_detail.get_obj(), "machine_id").get_str());
        loader_job_id = macho::stringutils::convert_utf8_to_unicode((std::string)find_value(_create_job_detail.get_obj(), "group").get_str());

        detail.management_id = find_value(_create_job_detail.get_obj(), "management_id").get_str();
        detail.mgmt_port = find_value_int32(_create_job_detail.get_obj(), "mgmt_port", 80);
        detail.is_ssl = find_value_bool(_create_job_detail.get_obj(), "is_ssl");

        mArray addr = find_value(_create_job_detail.get_obj(), "mgmt_addr").get_array();
        foreach(mValue a, addr){
            detail.mgmt_addr.insert(a.get_str());
        }
        detail.type = (saasame::transport::job_type::type)find_value(_create_job_detail.get_obj(), "type").get_int();
        mArray triggers = find_value(_create_job_detail.get_obj(), "triggers").get_array();
        foreach(mValue t, triggers){
            saasame::transport::job_trigger trigger;
            trigger.type = (saasame::transport::job_trigger_type::type)find_value(t.get_obj(), "type").get_int();
            trigger.start = find_value(t.get_obj(), "start").get_str();
            trigger.finish = find_value(t.get_obj(), "finish").get_str();
            trigger.interval = find_value(t.get_obj(), "interval").get_int();
            trigger.duration = find_value(t.get_obj(), "duration").get_int();
            trigger.id = find_value_string(t.get_obj(), "id");
            detail.triggers.push_back(trigger);
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
    return detail;
}

void cascading_job::get_cascadings_info(saasame::transport::cascading& cascadings, connection_op::ptr& conn_ptr, connection_op::ptr& cascading_ptr, std::vector<std::wstring>& parallels, int worker_thread_number){
    typedef std::map<std::string, cascading> cascading_map_type;
    if (cascadings.machine_id == _machine_id){	
        if (cascadings.connection_info.type != saasame::transport::connection_type::LOCAL_FOLDER){
            cascading_ptr = connection_op::get(cascadings.connection_info, worker_thread_number);
            conn_ptr = connection_op::get_cascading(cascadings.connection_info);
        }
    }
    else{
        foreach(cascading_map_type::value_type c, cascadings.branches){
            get_cascadings_info(c.second, conn_ptr, cascading_ptr, parallels, worker_thread_number);
            if (!parallels.empty())
                break;
        }
        if (cascadings.branches.count(_machine_id) || cascadings.branches.count(macho::stringutils::convert_unicode_to_utf8(_id))){
            foreach(cascading_map_type::value_type c, cascadings.branches){
                if (c.second.machine_id != _machine_id)
                    parallels.push_back(macho::stringutils::convert_ansi_to_unicode(c.second.machine_id));
            }
            if (!cascadings.machine_id.empty() && !cascadings.connection_info.id.empty())
                parallels.push_back(macho::stringutils::convert_ansi_to_unicode(cascadings.machine_id));
        }
    }
}

void cascading_job::earse_data(){
    LOG(LOG_LEVEL_WARNING, _T("Earse job (%s) temp data."), _id.c_str());
    connection_op::ptr cascading;
    connection_op::ptr conn_ptr;
    std::vector<std::wstring> parallels;
    get_cascadings_info(_loader_job_create_detail.cascadings, conn_ptr, cascading, parallels);
    if (conn_ptr){
        std::string machine_id;
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"Machine"].exists() && reg[L"Machine"].is_string())
                machine_id = reg[L"Machine"].string();
        }

        std::wstring wsz_machine_id = macho::stringutils::convert_utf8_to_unicode(_machine_id);
        std::remove_if(parallels.begin(), parallels.end(), [&wsz_machine_id](const std::wstring& parallel){return 0 == _wcsicmp(parallel.c_str(), wsz_machine_id.c_str()); });

        foreach(std::string snapshot_id, _loader_job_create_detail.snapshots){
            foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[snapshot_id]){
                read_imagex readx(conn_ptr, d.second, cascading, machine_id, parallels, std::vector<std::wstring>());
                saasame::ironman::imagex::irm_transport_image::ptr image = readx.get_image_info();
                saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal = readx.get_journal_info();
                if (image && journal && journal->is_version_2()){
                    uint64_t next_block_index = readx.get_cascadings_journal_next_block_index();
                    if (image->completed){
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            if (next_block_index > index){
                                std::wstring _file = image->get_block_name(index);
                                conn_ptr->remove_file(_file);
                            }
                        }
                        {
                            macho::windows::lock_able::ptr lock_obj = readx.get_source_journal_lock();
                            macho::windows::auto_lock lock(*lock_obj);
                            if (readx.are_cascadings_journals_closed())
                                readx.clear();
                            else if (readx.is_source_journal_existing())
                                readx.close_journal(journal);
                        }
                    }
                    else if (image->canceled){
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            std::wstring _file = image->get_block_name(index);
                            conn_ptr->remove_file(_file);
                        }
                        {
                            macho::windows::lock_able::ptr lock_obj = readx.get_source_journal_lock();
                            macho::windows::auto_lock lock(*lock_obj);
                            if (readx.are_cascadings_journals_closed())
                                readx.clear();
                            else if (readx.is_source_journal_existing())
                                readx.close_journal(journal);
                        }
                    }
                    else{
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            if (next_block_index > index){
                                std::wstring _file = image->get_block_name(index);
                                conn_ptr->remove_file(_file);
                            }
                        }
                    }
                }
            }
        }
    }
}

void cascading_job::execute(){
    VALIDATE;
    FUN_TRACE;
    _state &= ~mgmt_job_state::job_state_error;
    bool is_get_loader_job_create_detail = false;
    _is_cdr = false;
    connection_op::ptr cascading;
    connection_op::ptr conn_ptr;
    std::vector<std::wstring> parallels;
    if (!_is_initialized){
        is_get_loader_job_create_detail = _is_initialized = get_loader_job_create_detail(_loader_job_create_detail);
        LOG(LOG_LEVEL_INFO, _T("Initializing Loader job."));
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Initializing Loader job."));
    }
    else{
        is_get_loader_job_create_detail = get_loader_job_create_detail(_loader_job_create_detail);
    }
    save_config();
    if (!_is_initialized){
        LOG(LOG_LEVEL_ERROR, _T("Cannot initialize Loader job."));
        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, record_format("Cannot initialize Loader job."));
    }
    else if (_is_removing){
        earse_data();
    }
    else if (!is_get_loader_job_create_detail){
        _get_loader_job_create_detail_error_count++;
        if (_get_loader_job_create_detail_error_count > 3){
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, record_format("Cannot get the loader job detail"));
            _state |= mgmt_job_state::job_state_error;
            save_status();
        }
    }
    else if (_loader_job_create_detail.is_paused){
        LOG(LOG_LEVEL_WARNING, _T("Loader job(%s) is paused."), _id.c_str());
    }
    else if (!_loader_job_create_detail.snapshots.size()){
        LOG(LOG_LEVEL_WARNING, _T("Doesn't have any snapshot for Loader job(%s)."), _id.c_str());
        if (!_loader_job_create_detail.keep_alive && !_loader_job_create_detail.is_continuous_data_replication)
            modify_job_scheduler_interval(15);
    }
    else if (!_is_canceled){
        modify_job_scheduler_interval(1);
        uint32_t concurrent_count = boost::thread::hardware_concurrency();
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"ConcurrentLoaderJobNumber"].exists() && (DWORD)reg[L"ConcurrentLoaderJobNumber"] > 0){
                concurrent_count = ((DWORD)reg[L"ConcurrentLoaderJobNumber"]);
            }
        }
        macho::windows::semaphore  loader(L"Global\\Cascading", concurrent_count);
        if (!loader.trylock()){
            LOG(LOG_LEVEL_WARNING, L"Resource limited. Job %s will be resumed later.", _id.c_str());
            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_QUEUE_FULL,
                record_format("Pending for available job resource."));
        }
        else{
            _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
            _latest_enter_time = boost::posix_time::microsec_clock::universal_time();
            get_cascadings_info(_loader_job_create_detail.cascadings, conn_ptr, cascading, parallels, _loader_job_create_detail.worker_thread_number);
            if (conn_ptr && cascading){
                cascading->register_is_cancelled_function(boost::bind(&cascading_job::is_canceled, this));
                _is_interrupted = _terminated = false;
                if (_state & mgmt_job_state::job_state_none && !(_state & mgmt_job_state::job_state_error)){
                    _state = job_state::type::job_state_initialed;
                    LOG(LOG_LEVEL_INFO, _T("Initialization of Loader job completed."));
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Initialization of Loader job completed."));
                    save_status();
                }
                try{
                    while (!(_is_canceled || _is_interrupted || _state & mgmt_job_state::job_state_error)){
                        get_loader_job_create_detail(_loader_job_create_detail);
                        save_config();
                        if (_is_removing){
                            earse_data();
                            break;
                        }
                        if (!_loader_job_create_detail.snapshots.size()){
                            LOG(LOG_LEVEL_WARNING, _T("Doesn't have any snapshot for loader job(%s)."), _id.c_str());
                            break;
                        }

                        if (_loader_job_create_detail.block_mode_enable){
                            LOG(LOG_LEVEL_WARNING, _T("It's running in block mode."));
                        }

                        if (_state & mgmt_job_state::job_state_finished){
                            bool is_snapshoted = _current_snapshot.empty();
                            while (!(_is_canceled || _is_interrupted || is_snapshoted)){
                                if (is_snapshoted = check_snapshot(_current_snapshot)){
                                    break;
                                }
                                boost::unique_lock<boost::mutex> wait(_wait_lock);
                                _wait_cond.timed_wait(wait, boost::posix_time::seconds(20));
                            }
                            if (is_snapshoted){
                                size_t next_snapshot_index = 0;
                                std::vector<std::string> snapshots = _loader_job_create_detail.snapshots;
                                snapshots.erase(std::unique(snapshots.begin(), snapshots.end()), snapshots.end());
                                for (size_t i = 0; i < snapshots.size(); i++){
                                    if (_current_snapshot == snapshots[i]){
                                        next_snapshot_index = i + 1;
                                        break;
                                    }
                                }
                                if (next_snapshot_index == snapshots.size()){
                                    save_status();
                                    break;
                                }
                                else{
                                    _state = job_state::type::job_state_initialed;
                                    _current_snapshot = snapshots[next_snapshot_index];
                                    _progress_map.clear();

                                    if (!_loader_job_create_detail.disks_snapshot_mapping.count(_current_snapshot)){
                                        LOG(LOG_LEVEL_WARNING, _T("Incorrect job spec."));
                                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Incorrect job spec."));
                                        break;
                                    }
                                    foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[_current_snapshot]){
                                        loader_disk::ptr disk_progress;
                                        if (!_progress_map.count(d.first)){
                                            disk_progress = loader_disk::ptr(new loader_disk(d.first));
                                            _progress_map[d.first] = disk_progress;
                                        }
                                    }
                                    save_status();
                                }
                            }
                            else{
                                break;
                            }
                        }
                        else{
                            if (!_current_snapshot.empty()){
                                if (std::find(_loader_job_create_detail.snapshots.begin(), _loader_job_create_detail.snapshots.end(), _current_snapshot) == _loader_job_create_detail.snapshots.end()){
                                    if (check_snapshot(_current_snapshot)){
                                        _current_snapshot.clear();
                                    }
                                }
                            }
                            if (_current_snapshot.empty()){
                                foreach(std::string snapshot_id, _loader_job_create_detail.snapshots){
                                    if (!check_snapshot(snapshot_id)){
                                        _current_snapshot = snapshot_id;
                                        _state = mgmt_job_state::job_state_initialed;
                                        _progress_map.clear();
                                        foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[_current_snapshot]){
                                            loader_disk::ptr disk_progress;
                                            if (!_progress_map.count(d.first)){
                                                disk_progress = loader_disk::ptr(new loader_disk(d.first));
                                                _progress_map[d.first] = disk_progress;
                                            }
                                        }
                                        save_status();
                                        break;
                                    }
                                }
                            }
                        }

                        if (_current_snapshot.empty()){
                            LOG(LOG_LEVEL_WARNING, _T("Incorrect job spec."));
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, record_format("Incorrect job spec."));
                            break;
                        }
                        else if (_current_snapshot.length() && !_is_interrupted && !_is_canceled && (_state & mgmt_job_state::job_state_initialed) || (_state & mgmt_job_state::job_state_replicating)){
                            using namespace saasame::ironman::imagex;
                            _is_replicating = false;
                            foreach(string_map::value_type &d, _loader_job_create_detail.disks_snapshot_mapping[_current_snapshot]){
                                loader_disk::ptr disk_progress;
                                if (_progress_map.count(d.first)){
                                    disk_progress = _progress_map[d.first];
                                }
                                else{
                                    disk_progress = loader_disk::ptr(new loader_disk(d.first));
                                    _progress_map[d.first] = disk_progress;
                                    save_status();
                                }
                                if (disk_progress->completed || disk_progress->canceled)
                                    continue;

                                LOG(LOG_LEVEL_INFO, _T("Replicating snapshot (%s) data from (%s)"), macho::stringutils::convert_utf8_to_unicode(_current_snapshot).c_str(), macho::stringutils::convert_utf8_to_unicode(d.second).c_str(), macho::stringutils::convert_utf8_to_unicode(d.first).c_str());

                                bool is_canceled_snapshot = false;
                                foreach(loader_disk::map::value_type& l, _progress_map){
                                    if (l.second->canceled){
                                        is_canceled_snapshot = true;
                                        break;
                                    }
                                }
                                _execute<cascading_job, cascading_task>(d.second, conn_ptr, disk_progress, universal_disk_rw::ptr(), is_canceled_snapshot, 0, cascading, parallels, std::vector<std::wstring>(), true);
                                save_status();
                                if (_is_interrupted || _is_canceled)
                                    break;
                            }
                            bool is_finished, is_completed = true;
                            bool is_cdr = false;
                            foreach(loader_disk::map::value_type& l, _progress_map){
                                if (!(l.second->completed || l.second->canceled)){
                                    if (!(l.second->completed))
                                        is_completed = false;
                                    is_finished = false;
                                }
                                else if (l.second->canceled)
                                    is_completed = false;
                                if (l.second->cdr)
                                    is_cdr = true;
                            }
                            if (is_completed){
                                _state = is_cdr ? mgmt_job_state::job_state_cdr_replicated : mgmt_job_state::job_state_replicated;
                                save_status();
                                LOG(LOG_LEVEL_INFO, _T("Replicated all %s (%s) data"), is_cdr ? "changed" : "snapshot", macho::stringutils::convert_ansi_to_unicode(_current_snapshot).c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Replication of %1% data completed.") % (is_cdr ? "changed" : "snapshot")));
                            }
                            else if (is_finished){
                                _state = mgmt_job_state::job_state_discard;
                                save_status();
                            }
                            else if (!_is_replicating){
                                _is_interrupted = true;
                                break;
                            }
                        }

                        if (_state & mgmt_job_state::job_state_replicated || _state & mgmt_job_state::job_state_snapshot_created){
                            if (check_snapshot(_current_snapshot))
                                _state = mgmt_job_state::job_state_finished;
                        }
                        else if (_state & mgmt_job_state::job_state_discard){
                            if (check_snapshot(_current_snapshot))
                                _state = mgmt_job_state::job_state_finished;
                        }
                        else if (_state & mgmt_job_state::job_state_cdr_replicated){
                            if (check_snapshot(_current_snapshot))
                                _state = mgmt_job_state::job_state_finished;
                        }
                        save_status();
                        {
                            boost::unique_lock<boost::mutex> wait(_wait_lock);
                            _wait_cond.timed_wait(wait, boost::posix_time::seconds(30));
                        }
                    }
                }
                catch (loader_job::exception &ex){

                }
                catch (...){

                }
                _terminated = true;
            }
            _latest_leave_time = boost::posix_time::microsec_clock::universal_time();
        }
    }
    save_status();
}

void cascading_job::remove(){
    _is_removing = true;
    LOG(LOG_LEVEL_RECORD, L"Remove job : %s", _id.c_str());
    if (_running.try_lock()){
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        boost::filesystem::remove(_progress_file);
        boost::filesystem::remove(_config_file.string() + ".cmd");
        _running.unlock();
    }
}



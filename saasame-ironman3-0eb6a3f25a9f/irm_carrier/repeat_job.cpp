
#include "macho.h"
#include "..\gen-cpp\common_service_handler.h"
#include "repeat_job.h"
#include <codecvt>

using namespace macho::windows;
using namespace macho;
using namespace json_spirit;
using namespace saasame::ironman::imagex;

#define PurggingSize              20
#define TransportModePurggingSize 2
#define NumberOfWorkers           4

class relay_op{
public:
    relay_op(std::wstring basename, std::wstring filename, irm_imagex_local_op::ptr source_op, irm_imagex_op::ptr target_op, std::string connection_id, std::set<connection> &conns) :
        _filename(filename),
        _basename(basename),
        _source_op(source_op),
        _target_op(target_op),
        _sz_connection_id(connection_id)
    {
        _connection_id = macho::stringutils::convert_ansi_to_unicode(connection_id);
        _sz_connection_id = connection_id;
        foreach(connection c, conns){
            if (c.id != connection_id)
                _parallels.push_back(macho::stringutils::convert_ansi_to_unicode(c.id));
        }
    }
    relay_op(std::string basename, std::string filename, irm_imagex_local_op::ptr source_op, irm_imagex_op::ptr target_op, std::string connection_id, std::set<connection> &conns) :
        _basename(basename),
        _source_op(source_op),
        _target_op(target_op),
        _sz_connection_id(connection_id)
    {
        _filename = macho::stringutils::convert_ansi_to_unicode(filename);
        _connection_id = macho::stringutils::convert_ansi_to_unicode(connection_id);
        foreach(connection c, conns){
            if (c.id != connection_id)
                _parallels.push_back(macho::stringutils::convert_ansi_to_unicode(c.id));
        }
    }

    bool clear()
    {
        bool result = false;
        if (!(result = _source_op->remove_file(_filename))) {}
        else if (!(result = _source_op->remove_file(_filename + (IRM_IMAGE_JOURNAL_EXT)))){}
        else if (!(result = _source_op->remove_image(_filename))){}
        _source_op->remove_file(_filename + L"." + _connection_id);
        _source_op->remove_file(_filename + L".close." + _connection_id);
        foreach(std::wstring _c, _parallels){
            _source_op->remove_file(_filename + L"." + _c);
            _source_op->remove_file(_filename + L".close." + _c);
        }
        return result;
    }

    irm_transport_image::ptr get_source_image_info()
    {
        try
        { 
            std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
            macho::windows::lock_able::vtr lock_objs = _source_op->get_lock_objects(_filename);
            macho::windows::auto_locks lock(lock_objs);
            // Can't use read_metafile because we need to read the file content from the output folder.
            irm_transport_image::ptr p = irm_transport_image::ptr(new irm_transport_image());
            if (p && _source_op->read_file(_filename, buffer))
            { 
                p->load(buffer);
                return p;
            }
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        return NULL;
    }

    bool is_source_journal_existing(){
        return _source_op->is_file_existing(_filename + (IRM_IMAGE_JOURNAL_EXT));
    }

    irm_transport_image_blks_chg_journal::ptr get_source_journal_info()
    {
        try
        { 
            std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
            macho::windows::lock_able::vtr lock_objs = _source_op->get_lock_objects(_filename + (IRM_IMAGE_JOURNAL_EXT));
            macho::windows::auto_locks lock(lock_objs);
            // Can't use read_metafile because we need to read the file content from the output folder.
            if (_source_op->read_file(_filename + (IRM_IMAGE_JOURNAL_EXT), buffer))
            { 
                irm_transport_image_blks_chg_journal::ptr p = irm_transport_image_blks_chg_journal::ptr(new irm_transport_image_blks_chg_journal());
                p->load(buffer);
                return p;
            }
        }
        catch (macho::windows::lock_able::exception &ex){
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        return NULL;
    }

    irm_transport_image_blks_chg_journal::ptr get_target_journal_info()
    {
        std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
        irm_transport_image_blks_chg_journal::ptr p = irm_transport_image_blks_chg_journal::ptr(new irm_transport_image_blks_chg_journal());
        if (p && _target_op->read_metafile(_basename / (_filename + (IRM_IMAGE_JOURNAL_EXT)), *p.get())){
            return p;
        }
        return NULL;
    }

    macho::windows::lock_able::ptr get_source_journal_lock(){
        return _source_op->get_lock_object(_filename + IRM_IMAGE_JOURNAL_EXT, _sz_connection_id);
    }

    saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr get_parallels_journal_info(bool is_closed = false){
        FUN_TRACE;
        try{
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr result;
            if (_source_op){
                foreach(std::wstring _c, _parallels){
                    irm_transport_image_blks_chg_journal::ptr p = irm_transport_image_blks_chg_journal::ptr(new irm_transport_image_blks_chg_journal());
                    macho::windows::lock_able::ptr lock_obj = _source_op->get_lock_object(_filename + (is_closed ? L".close." : L".") + _c, _sz_connection_id);
                    macho::windows::auto_lock lock(*lock_obj);
                    if (p && _source_op->read_file(_filename + (is_closed ? L".close." : L".") + _c, *p.get())){
                        result.push_back(p);
                    }
                    else{
                        break;
                    }
                }
                if (result.size() == _parallels.size())
                    return result;
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

    uint64_t get_parallels_journal_next_block_index(){
        uint64_t index = -1LL;
        if (!_parallels.empty()){
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr journals = get_parallels_journal_info();
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

    bool are_parallel_journals_closed(){
        bool result = true;
        if (!_parallels.empty()){
            saasame::ironman::imagex::irm_transport_image_blks_chg_journal::vtr journals = get_parallels_journal_info(true);
            if (journals.empty())
                return false;
            foreach(saasame::ironman::imagex::irm_transport_image_blks_chg_journal::ptr journal, journals){
                if (!journal->is_closed())
                    result = false;
            }
        }
        return result;
    }

    bool remove_source_file(std::wstring filename)
    {
        _source_op->remove_file(filename + IRM_IMAGE_DEDUPE_TMP_EXT);
        return _source_op->remove_file(filename);
    }

    bool replicate_transport_image_block(std::wstring transport_image_block_name, uint32_t &data, bool chip_mode)
    {
        bool result = false;
        std::ifstream ifs;
        if (result = _source_op->read_file(transport_image_block_name, ifs))
        {
            try
            { 
                if (chip_mode){
                    /* if (result = _target_op->is_file_existing(transport_image_block_name)){
                    }
                    else */
                    if (!(result = _target_op->write_file(transport_image_block_name, ifs))){
                    }
                }
                else{
                    macho::windows::lock_able::vtr lock_objs = _target_op->get_lock_objects(transport_image_block_name);
                    macho::windows::auto_locks lock(lock_objs);
                    result = _target_op->write_file(transport_image_block_name, ifs);
                }
            }
            catch (macho::windows::lock_able::exception &ex){                    
                LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
            }
        }

        if (result){
            saasame::ironman::imagex::irm_transport_image_block image_block(0, 0);
            image_block.load_header(ifs);
            if (chip_mode){
                data = image_block.length;
            }
            else{
                data = 0;
                uint64_t newBit, oldBit = 0;
                bool     dirty_range = false;
                for (newBit = 0; newBit < (image_block.length / IRM_IMAGE_SECTOR_SIZE); newBit++)
                {
                    if (image_block.bitmap[newBit >> 3] & (1 << (newBit & 7)))
                    {
                        if (!dirty_range)
                        {
                            dirty_range = true;
                            oldBit = newBit;
                        }
                    }
                    else
                    {
                        if (dirty_range)
                        {
                            uint32_t length = (newBit - oldBit) * IRM_IMAGE_SECTOR_SIZE;
                            data += length;
                            dirty_range = false;
                        }
                    }
                }
                if (dirty_range)
                {
                    uint32_t length = (newBit - oldBit) * IRM_IMAGE_SECTOR_SIZE;
                    data += length;
                }
            }
        }
        return result;
    }

    bool replicate_journal(irm_transport_image_blks_chg_journal::ptr journal, bool local_only = false)
    {    
        if (!_parallels.empty()){
            macho::windows::lock_able::ptr lock_obj = _source_op->get_lock_object(_filename + L"." + _connection_id, _sz_connection_id);
            macho::windows::auto_lock lock(*lock_obj);
            if (!_source_op->write_file(_filename + L"." + _connection_id, *journal.get()))
                return false;
        }
        if (local_only)
            return true;
        return _target_op->write_metafile(_basename / (_filename + (IRM_IMAGE_JOURNAL_EXT)), *journal.get());
    }

    bool close_journal(irm_transport_image_blks_chg_journal::ptr journal)
    {
        if (!_parallels.empty()){
            macho::windows::lock_able::ptr lock_obj = _source_op->get_lock_object(_filename + L".close." + _connection_id, _sz_connection_id);
            macho::windows::auto_lock lock(*lock_obj);
            irm_transport_image_blks_chg_journal _journal = *journal.get();
            _journal.extension.append(IRM_IMAGE_CLOSE_EXT);
            if (!_source_op->write_file(_filename + L".close." + _connection_id, _journal))
                return false;
        }
        return true;
    }

    bool replicate_image(irm_transport_image::ptr image, bool local_only = false)
    {
        bool result = false;
        irm_transport_image *img = dynamic_cast<irm_transport_image *>(image.get());
        return _target_op->write_metafile(_basename / _filename, *img, local_only);
    }

    bool replicate_image_remote(irm_transport_image::ptr image)
    {
        std::vector<boost::filesystem::path> metafiles;
        metafiles.push_back(_basename / _filename);
        return replicate_image(image, true) && _target_op->flush_metafiles(metafiles);
    }

    bool flush_metafiles()
    {
        std::vector<boost::filesystem::path> metafiles;
        metafiles.push_back(_basename/_filename);
        metafiles.push_back(boost::filesystem::path(_basename/(_filename + (IRM_IMAGE_JOURNAL_EXT))));
        return _target_op->flush_metafiles(metafiles);
    }

    macho::windows::lock_able::vtr get_source_lock_objects(std::wstring transport_image_block_name)
    {
        return _source_op->get_lock_objects(transport_image_block_name);
    }

private:
    boost::filesystem::path                       _basename;
    std::wstring                                  _filename;
    std::wstring                                  _connection_id;
    std::string                                   _sz_connection_id;
    irm_imagex_local_op::ptr                      _source_op;
    irm_imagex_op::ptr                            _target_op;
    std::vector<std::wstring>                     _parallels;
};

struct replication_task{
    typedef std::auto_ptr<replication_task> ptr;
    typedef boost::signals2::signal<bool(), macho::_or<bool>> job_is_canceled;
    typedef boost::signals2::signal<void()> job_save_statue;
    typedef boost::signals2::signal<void(saasame::transport::job_state::type state, int error, record_format& format)> job_record;
    inline void register_job_is_canceled_function(job_is_canceled::slot_type slot){
        is_canceled.connect(slot);
    }
    inline void register_job_save_callback_function(job_save_statue::slot_type slot){
        save_status.connect(slot);
    }
    inline void register_job_record_callback_function(job_record::slot_type slot){
        record.connect(slot);
    }
    replication_task(relay_op& _op, relay_disk& _progress) : op(_op), terminated(false), progress(_progress), update_journal_error(false){}
    macho::windows::critical_section                           cs;
    saasame::ironman::imagex::irm_transport_image::ptr         image;
    relay_block_file::queue                                    waitting;
    relay_block_file::vtr                                      processing;
    relay_block_file::queue                                    error;
    relay_op&                                                  op;
    relay_disk&                                                progress;
    irm_transport_image_blks_chg_journal::ptr                  journal_db;
    bool                                                       terminated;
    bool                                                       purge_data;
    bool                                                       update_journal_error;
    void                                                       replicate();
    job_is_canceled                                            is_canceled;
    job_save_statue                                            save_status;
    job_record                                                 record;
    boost::mutex                                               completion_lock;
    boost::condition_variable                                  completion_cond;
};

void replication_task::replicate()
{
    bool            result = true;
    relay_block_file::ptr bk_file;
    int      purgging_size = IRM_IMAGE_TRANSPORT_MODE == image->mode ? TransportModePurggingSize : PurggingSize;
    registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"PurggingSize"].exists() && (DWORD)reg[L"PurggingSize"] > 0){
            purgging_size = (DWORD)reg[L"PurggingSize"];
        }
    }
    while (true){
        {
            macho::windows::auto_lock lock(cs);
            bool update_journal = false;
            if (bk_file){
                if (!result){
                    terminated = true;
                    error.push_back(bk_file);
                    break;
                }
                else{
                    for (relay_block_file::vtr::iterator b = processing.begin(); b != processing.end(); b++){
                        if ((*b)->name == bk_file->name){
                            processing.erase(b);
                            break;
                        }
                    }
                    macho::windows::auto_lock lock(progress.cs);
                    progress.completed_blocks.push_back(bk_file);
                    std::sort(progress.completed_blocks.begin(), progress.completed_blocks.end(), relay_block_file::compare());
                    while (progress.completed_blocks.size() && error.size() == 0){
                        if (0 == processing.size() || progress.completed_blocks.front()->index < processing[0]->index){
                            bk_file = progress.completed_blocks.front();
                            if (IRM_IMAGE_TRANSPORT_MODE == image->mode && (progress.journal_index + 1) != bk_file->index){
                                LOG(LOG_LEVEL_WARNING, _T("Can't update the journal db file. (%I64u != %I64u)"), progress.journal_index, bk_file->index);
                                break;
                            }
                            progress.completed_blocks.pop_front();
                            if (purge_data){
                                if (IRM_IMAGE_TRANSPORT_MODE == image->mode){
                                    progress.purgging.push_back(bk_file);
                                }
                                else{
                                    std::wstring _file = bk_file->name;
                                    auto it = std::find_if(progress.purgging.begin(), progress.purgging.end(), [&_file](const relay_block_file::ptr& obj){return obj->name == _file; });
                                    if (it != progress.purgging.end()){
                                        if (bk_file->index > (*it)->index)
                                            (*it)->index = bk_file->index;
                                    }
                                    else{
                                        progress.purgging.push_back(bk_file);
                                    }
                                }
                                std::sort(progress.purgging.begin(), progress.purgging.end(), relay_block_file::compare());
                                uint64_t next_block_index = op.get_parallels_journal_next_block_index();
                                for (relay_block_file::vtr::iterator b = progress.purgging.begin(); b != progress.purgging.end() && progress.purgging.size() > purgging_size;){
                                    if (IRM_IMAGE_TRANSPORT_MODE == image->mode){
                                        if (next_block_index > (*b)->index && !op.remove_source_file((*b)->name)){
                                            LOG(LOG_LEVEL_ERROR, _T("Failed to remove the data block file (%s)."), (*b)->name.c_str());
                                            break;
                                        }
                                    }
                                    else{
                                        if (0 == (*b)->locks.size())
                                            (*b)->locks = op.get_source_lock_objects((*b)->name);
                                        try{
                                            macho::windows::auto_locks lock((*b)->locks);
                                            if (!op.remove_source_file((*b)->name)){
                                                LOG(LOG_LEVEL_ERROR, _T("Failed to remove the data block file (%s)."), (*b)->name.c_str());
                                                break;
                                            }
                                        }
                                        catch (macho::windows::lock_able::exception &ex){
                                            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                                            break;
                                        }
                                    }
                                    b = progress.purgging.erase(b);
                                }
                            }
                            update_journal = true;
                            if (IRM_IMAGE_TRANSPORT_MODE != image->mode)
                                journal_db->dirty_blocks_list.push_back(bk_file->number);
                            else{
                                journal_db->next_block_index = bk_file->index + 1;
                            }
                            progress.journal_index = bk_file->index;
                            progress.data += bk_file->data;
                            save_status();
                            if (IRM_IMAGE_TRANSPORT_MODE != image->mode){
                                foreach(macho::windows::lock_able::ptr lck, bk_file->locks)
                                    lck->unlock();
                            }
                        }
                        else
                            break;
                    }
                    if (update_journal && !op.replicate_journal(journal_db))
                    {
                        LOG(LOG_LEVEL_ERROR, _T("Cannot replicate journal file."));
                        record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate journal file.")));
                        result = false;
                        update_journal_error = true;
                        save_status();
                        break;
                    }
                    else if (!update_journal){
                        save_status();
                    }
                }
            }

            if (0 == waitting.size() || terminated || is_canceled())
                break;
            bk_file = waitting.front();
            waitting.pop_front();
            if (IRM_IMAGE_TRANSPORT_MODE == image->mode){
                foreach(relay_block_file::ptr &b, progress.completed_blocks){
                    if (b->index == bk_file->index){
                        LOG(LOG_LEVEL_INFO, _T("Skip completed block( %I64u)"), bk_file->index);
                        if (0 == waitting.size()){
                            bk_file = NULL;
                            break;
                        }
                        bk_file = waitting.front();
                        waitting.pop_front();
                    }
                    else if (b->index > bk_file->index){
                        LOG(LOG_LEVEL_INFO, _T("b->index > bk_file->index(%I64u > %I64u)"), b->index, bk_file->index);
                        break;
                    }
                }
                if (!bk_file)
                    break;
            }
            processing.push_back(bk_file);
            std::sort(processing.begin(), processing.end(), relay_block_file::compare());
        }

        if (terminated)
            break;

        LOG(LOG_LEVEL_INFO, _T("Replicate block file (%s)."), bk_file->name.c_str());
        if (!(result = op.replicate_transport_image_block(bk_file->name, bk_file->data, IRM_IMAGE_TRANSPORT_MODE == image->mode)))
        {
            LOG(LOG_LEVEL_ERROR, _T("Cannot replicate block file (%s)."), bk_file->name.c_str());
            record((job_state::type)(mgmt_job_state::job_state_replicating), error_codes::SAASAME_E_FAIL, (record_format("Cannot replicate block file %1%.") % macho::stringutils::convert_unicode_to_ansi(bk_file->name)));
        }     
    }
    completion_cond.notify_one();
}

repeat_job::repeat_job(std::wstring id, std::string image_name, std::string parnet, std::string connection_id) :
_terminated(false),
_is_canceled(false),
_is_interrupted(false),
_state(job_state_none),
_purge(false),
_parent(parnet),
_image_name(image_name),
_connection_id(connection_id),
macho::removeable_job(id, macho::stringutils::convert_ansi_to_unicode(image_name))
{
    FUN_TRACE;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::create_directories(work_dir / L"jobs");
    _config_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s") % _id % JOB_EXTENSION);
    _status_file = work_dir / L"jobs" / boost::str(boost::wformat(L"%s%s.status") % _id % JOB_EXTENSION);
    _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
}

repeat_job::vtr  repeat_job::create_jobs(std::string image_name, std::string parnet, std::set<connection>& conns)
{
    repeat_job::vtr jobs;
    typedef std::map<std::string, std::set<connection> > conns_map_type;
    conns_map_type _conns_map;
    foreach(connection conn, conns)
    {
        switch (conn.type)
        {
        case connection_type::LOCAL_FOLDER:
            _conns_map[conn.detail.local.path].insert(conn);
            break;
        case connection_type::WEBDAV_EX:
        case connection_type::S3_BUCKET_EX:
        case connection_type::LOCAL_FOLDER_EX:
            _conns_map[conn.detail.local.path].insert(conn);
            break;
        default:
            break;
        }
    }

    foreach(conns_map_type::value_type &c, _conns_map)
    {
        bool purge = true;
        bool need_relay = false;
        foreach(connection conn, c.second)
        {
            switch (conn.type)
            {
            case connection_type::LOCAL_FOLDER:
                purge = false;
                break;
            case connection_type::WEBDAV_EX:
            case connection_type::S3_BUCKET_EX:
            case connection_type::LOCAL_FOLDER_EX:
                need_relay = true;
                break;
            }
        }
        if (need_relay)
        {
            foreach(connection conn, c.second){
                repeat_job::ptr j = repeat_job::ptr(new repeat_job(macho::guid_::create(), image_name, parnet, conn.id));
                j->_conns = c.second;
                j->_created_time = boost::posix_time::microsec_clock::universal_time();
                j->_purge = purge;
                j->save_config();
                j->save_status();
                jobs.push_back(j);
            }
        }
    }
    return jobs;
}

repeat_job::ptr repeat_job::load(boost::filesystem::path config_file, boost::filesystem::path status_file)
{
    repeat_job::ptr job = _load_config(config_file);
    if (job)
        job->load_status(status_file.wstring());
    return job;
}

repeat_job::ptr repeat_job::_load_config(boost::filesystem::path &config_file)
{
    repeat_job::ptr job;
    try
    {
        std::ifstream is(config_file.string());
        mValue job_config;
        json_spirit::read(is, job_config);
        std::wstring id =  macho::stringutils::convert_utf8_to_unicode(find_value_string(job_config.get_obj(), "id"));
        std::string group = find_value_string(job_config.get_obj(), "group");
        std::string parent = find_value_string(job_config.get_obj(), "parent");
        std::string _connection_id = find_value_string(job_config.get_obj(), "connection_id");

        if (id.length() && group.length())
        {
            job = repeat_job::ptr(new repeat_job(id, group, parent, _connection_id));
            if (job)
            {
                mArray connections = find_value(job_config.get_obj(), "connections").get_array();
                foreach(mValue &conn, connections)
                {
                    connection _conn;
                    if (_connection_id.empty())
                        _connection_id = _conn.id = find_value(conn.get_obj(), "id").get_str();
                    else
                        _conn.id = find_value(conn.get_obj(), "id").get_str();

                    _conn.type = (saasame::transport::connection_type::type)(find_value(conn.get_obj(), "type").get_int());
                    mArray options = find_value(conn.get_obj(), "options").get_array();
                    foreach(mValue &o, options)
                    {
                        _conn.options[find_value(o.get_obj(), "key").get_str()] = find_value(o.get_obj(), "value").get_str();
                    }

                    _conn.checksum = find_value_bool(conn.get_obj(), "checksum");
                    _conn.compressed = find_value_bool(conn.get_obj(), "compressed");
                    _conn.encrypted = find_value_bool(conn.get_obj(), "encrypted");

                    macho::bytes user, password, u1, u2;
                    macho::bytes proxy_user, proxy_password, u3, u4;

                    switch (_conn.type)
                    {
                    case connection_type::LOCAL_FOLDER_EX:
                        _conn.detail.remote.path = find_value_string(conn.get_obj(), "remote_path");
                    case connection_type::LOCAL_FOLDER:
                        _conn.detail.local.path = find_value_string(conn.get_obj(), "path");
                        if (!_conn.options.count("folder"))
                            _conn.options["folder"] = _conn.detail.local.path;
                        break;
                    case connection_type::S3_BUCKET_EX:
                    case connection_type::WEBDAV_EX:
                        _conn.detail.local.path = find_value_string(conn.get_obj(), "local_path");
                    case connection_type::S3_BUCKET:
                    case connection_type::WEBDAV_WITH_SSL:
                        _conn.detail.remote.port = find_value_int32(conn.get_obj(), "port");
                        _conn.detail.remote.proxy_host = find_value_string(conn.get_obj(), "proxy_host");
                        _conn.detail.remote.proxy_port = find_value_int32(conn.get_obj(), "proxy_port");
                        if (_conn.type == connection_type::S3_BUCKET || _conn.type == connection_type::S3_BUCKET_EX)
                            _conn.detail.remote.s3_region = (saasame::transport::aws_region::type)find_value_int32(conn.get_obj(), "s3_region");
                        proxy_user.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u3")));
                        proxy_password.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u4")));
                        u3 = macho::windows::protected_data::unprotect(proxy_user, true);
                        u4 = macho::windows::protected_data::unprotect(proxy_password, true);
                        if (u3.length() && u3.ptr()){
                            _conn.detail.remote.proxy_username = std::string(reinterpret_cast<char const*>(u3.ptr()), u3.length());
                        }
                        else{
                            _conn.detail.remote.proxy_username = "";
                        }
                        if (u4.length() && u4.ptr()){
                            _conn.detail.remote.proxy_password = std::string(reinterpret_cast<char const*>(u4.ptr()), u4.length());
                        }
                        else{
                            _conn.detail.remote.proxy_password = "";
                        }

                    case connection_type::NFS_FOLDER:
                        _conn.detail.remote.path = find_value_string(conn.get_obj(), "path");
                        user.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u1")));
                        password.set(macho::stringutils::convert_utf8_to_unicode(find_value_string(conn.get_obj(), "u2")));
                        u1 = macho::windows::protected_data::unprotect(user, true);
                        u2 = macho::windows::protected_data::unprotect(password, true);
                        if (u1.length() && u1.ptr()){
                            _conn.detail.remote.username = std::string(reinterpret_cast<char const*>(u1.ptr()), u1.length());
                        }
                        else{
                            _conn.detail.remote.username = "";
                        }
                        if (u2.length() && u2.ptr()){
                            _conn.detail.remote.password = std::string(reinterpret_cast<char const*>(u2.ptr()), u2.length());
                        }
                        else{
                            _conn.detail.remote.password = "";
                        }
                        break;
                    }
                    job->_conns.insert(_conn);
                }
                job->_purge = find_value_bool(job_config.get_obj(), "purge");
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read config info.")).c_str());
    }
    catch (...){
    }
    return job;
}

bool repeat_job::save_config()
{
    try
    {
        mObject job_config;
        job_config["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_config["group"] = macho::stringutils::convert_unicode_to_utf8(_group);
        job_config["connection_id"] = _connection_id;
        job_config["parent"] = _parent;

        mArray conns;
        foreach(connection _conn, _conns)
        {
            mObject conn;
            conn["id"] = _conn.id;
            conn["type"] = (int)_conn.type;
            macho::bytes user, password, u1, u2;
            macho::bytes proxy_user, proxy_password, u3, u4;
            switch (_conn.type)
            {
            case connection_type::LOCAL_FOLDER_EX:
                conn["remote_path"] = _conn.detail.remote.path;
            case connection_type::LOCAL_FOLDER:
                conn["path"] = _conn.detail.local.path;
                break;
            case connection_type::WEBDAV_EX:
            case connection_type::S3_BUCKET_EX:
                conn["local_path"] = _conn.detail.local.path;
            case connection_type::S3_BUCKET:
            case connection_type::WEBDAV_WITH_SSL:
                conn["port"] = _conn.detail.remote.port;
                conn["proxy_host"] = _conn.detail.remote.proxy_host;
                conn["proxy_port"] = _conn.detail.remote.proxy_port;
                if (_conn.type == connection_type::S3_BUCKET || _conn.type == connection_type::S3_BUCKET_EX)
                    conn["s3_region"] = (int)_conn.detail.remote.s3_region;
                proxy_user.set((LPBYTE)_conn.detail.remote.proxy_username.c_str(), _conn.detail.remote.proxy_username.length());
                proxy_password.set((LPBYTE)_conn.detail.remote.proxy_password.c_str(), _conn.detail.remote.proxy_password.length());
                u3 = macho::windows::protected_data::protect(proxy_user, true);
                u4 = macho::windows::protected_data::protect(proxy_password, true);
                conn["u3"] = macho::stringutils::convert_unicode_to_utf8(u3.get());
                conn["u4"] = macho::stringutils::convert_unicode_to_utf8(u4.get());

            case connection_type::NFS_FOLDER:                conn["path"] = _conn.detail.remote.path;
                user.set((LPBYTE)_conn.detail.remote.username.c_str(), _conn.detail.remote.username.length());
                password.set((LPBYTE)_conn.detail.remote.password.c_str(), _conn.detail.remote.password.length());
                u1 = macho::windows::protected_data::protect(user, true);
                u2 = macho::windows::protected_data::protect(password, true);
                conn["u1"] = macho::stringutils::convert_unicode_to_utf8(u1.get());
                conn["u2"] = macho::stringutils::convert_unicode_to_utf8(u2.get());
                break;
            }
            mArray options;
            typedef std::map<std::string, std::string> string_map;
            foreach(const string_map::value_type &option, _conn.options)
            {
                mObject opt;
                opt["key"] = option.first;
                opt["value"] = option.second;
                options.push_back(opt);
            }
            conn["options"] = options;
            conn["checksum"] = _conn.checksum;
            conn["compressed"] = _conn.compressed;
            conn["encrypted"] = _conn.encrypted;
            conns.push_back(conn);
        }
        job_config["connections"] = conns;
        job_config["purge"] = _purge;

        boost::filesystem::path temp = _config_file.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            write(job_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
        }
        if (!MoveFileEx(temp.wstring().c_str(), _config_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), _config_file.wstring().c_str(), GetLastError());
            return false;
        }
        return true;
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output config info.")).c_str());
    }
    catch (...)
    {
    }
    return false;
}

bool repeat_job::save_status()
{
    try
    {
        macho::windows::auto_lock lock(_cs);
        mObject job_status;
        job_status["id"] = macho::stringutils::convert_unicode_to_utf8(_id);
        job_status["base_name"] = _base_name;
        job_status["is_interrupted"] = _is_interrupted;
        job_status["is_canceled"] = _is_canceled;
        job_status["is_removing"] = _is_removing;
        job_status["state"] = _state;
        job_status["created_time"] = boost::posix_time::to_simple_string(_created_time);
        job_status["latest_enter_time"] = boost::posix_time::to_simple_string(_latest_enter_time);
        job_status["latest_leave_time"] = boost::posix_time::to_simple_string(_latest_leave_time);
        mArray histories;
        foreach(history_record::ptr &h, _histories)
        {
            mObject o;
            o["time"] = boost::posix_time::to_simple_string(h->time);
            o["state"] = (int)h->state;
            o["error"] = h->error;
            o["description"] = h->description;
            o["format"] = h->format;
            mArray  args(h->args.begin(), h->args.end());
            o["arguments"] = args;
            histories.push_back(o);
        }
        job_status["histories"] = histories;

        macho::windows::auto_lock _lock(_progress.cs);
        mObject progress;
        progress["completed"] = _progress.completed;
        progress["journal_index"] = _progress.journal_index;
        progress["data"] = _progress.data;

        mArray purgging;
        for (relay_block_file::vtr::iterator b = _progress.purgging.begin(); b != _progress.purgging.end(); b++)
        {
            mObject _b;
            _b["name"] = macho::stringutils::convert_unicode_to_utf8((*b)->name);
            _b["index"] = (*b)->index;
            _b["number"] = (*b)->number;
            _b["data"] = (int)(*b)->data;
            purgging.push_back(_b);
        }
        progress["purgging"] = purgging;

        mArray completed_blocks;
        foreach(relay_block_file::ptr& b ,_progress.completed_blocks)
        {
            mObject _b;
            _b["name"] = macho::stringutils::convert_unicode_to_utf8(b->name);
            _b["index"] = b->index;
            _b["number"] = b->number;
            _b["data"] = (int)b->data;
            completed_blocks.push_back(_b);
        }
        progress["completed_blocks"] = completed_blocks;

        job_status["progress"] = progress;
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
            return false;
        }
        return true;
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't output status info.")));
    }
    catch (...)
    {
    }
    return false;
}

bool repeat_job::load_status(std::wstring status_file)
{
    try
    {
        if (boost::filesystem::exists(status_file))
        {
            std::ifstream is(status_file);
            mValue job_status;
            read(is, job_status);
            _base_name = find_value_string(job_status.get_obj(), "base_name");
            _is_canceled = find_value(job_status.get_obj(), "is_canceled").get_bool();
            _is_removing = find_value_bool(job_status.get_obj(), "is_removing");
            _state = find_value(job_status.get_obj(), "state").get_int();
            _created_time = boost::posix_time::time_from_string(find_value(job_status.get_obj(), "created_time").get_str());
            std::string latest_enter_time_str = find_value(job_status.get_obj(), "latest_enter_time").get_str();
            if (0 == latest_enter_time_str.length() || latest_enter_time_str == "not-a-date-time")
                _latest_enter_time = boost::date_time::not_a_date_time;
            else
                _latest_enter_time = boost::posix_time::time_from_string(latest_enter_time_str);

            std::string latest_leave_time_str = find_value(job_status.get_obj(), "latest_leave_time").get_str();
            if (0 == latest_leave_time_str.length() || latest_leave_time_str == "not-a-date-time")
                _latest_leave_time = boost::date_time::not_a_date_time;
            else
                _latest_leave_time = boost::posix_time::time_from_string(latest_leave_time_str);

            std::string latest_update_time_str = find_value(job_status.get_obj(), "latest_update_time").get_str();
            if (0 == latest_update_time_str.length() || latest_update_time_str == "not-a-date-time")
                _latest_update_time = boost::date_time::not_a_date_time;
            else
                _latest_update_time = boost::posix_time::time_from_string(latest_update_time_str);

            mArray histories = find_value(job_status.get_obj(), "histories").get_array();
            foreach(mValue &h, histories)
            {
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
                    arguments)));
            }
            mObject progress = find_value(job_status.get_obj(), "progress").get_obj();
            _progress.completed = find_value(progress, "completed").get_bool();
            _progress.journal_index = find_value(progress, "journal_index").get_int64();
            _progress.data = find_value(progress, "data").get_int64();

            mArray purgging = find_value(progress, "purgging").get_array();
            foreach(mValue &b, purgging)
            {
                relay_block_file::ptr _b = relay_block_file::ptr(new relay_block_file());
                _b->name = macho::stringutils::convert_utf8_to_unicode(find_value(b.get_obj(), "name").get_str());
                _b->index = find_value(b.get_obj(), "index").get_int();
                _b->number = find_value(b.get_obj(), "number").get_int();
                _b->data = find_value(b.get_obj(), "data").get_int();
                _progress.purgging.push_back(_b);
            }

            mArray completed_blocks = find_value_array(progress, "completed_blocks");
            foreach(mValue &b, completed_blocks)
            {
                relay_block_file::ptr _b = relay_block_file::ptr(new relay_block_file());
                _b->name = macho::stringutils::convert_utf8_to_unicode(find_value(b.get_obj(), "name").get_str());
                _b->index = find_value(b.get_obj(), "index").get_int();
                _b->number = find_value(b.get_obj(), "number").get_int();
                _b->data = find_value(b.get_obj(), "data").get_int();
                _progress.completed_blocks.push_back(_b);
            }
            return true;
        }
    }
    catch (boost::exception& ex)
    {
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read status info.")));
    }
    catch (...)
    {
    }
    return false;
}

void repeat_job::record(saasame::transport::job_state::type state, int error, record_format& format){
    macho::windows::auto_lock lock(_cs);
    if (_histories.size() == 0 || _histories[_histories.size() - 1]->description != format.str()){
        _histories.push_back(history_record::ptr(new history_record(state, error, format)));
        LOG((error ? LOG_LEVEL_ERROR : LOG_LEVEL_RECORD), L"(%s)%s", _id.c_str(), macho::stringutils::convert_ansi_to_unicode(format.str()).c_str());
    }
    while (_histories.size() > _MAX_HISTORY_RECORDS)
        _histories.erase(_histories.begin());
}

void repeat_job::interrupt()
{
    FUN_TRACE
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled)
    {
        return;
    }
    _is_interrupted = true;

    save_status();
}

void repeat_job::earse_image_data(){
    if (_conns.size() > 1){
        irm_imagex_local_op::ptr  source_op = _get_source_op();
        if (source_op){
            irm_transport_image_blks_chg_journal::ptr target_journal_db;
            relay_op op(_id, _group, source_op, irm_imagex_op::ptr(), _connection_id, _conns);
            irm_transport_image::ptr image = op.get_source_image_info();
            irm_transport_image_blks_chg_journal::ptr journal = op.get_source_journal_info();
            if (image && journal && journal->is_version_2()){
                target_journal_db = irm_transport_image_blks_chg_journal::ptr(new irm_transport_image_blks_chg_journal());
                target_journal_db->parent_path = journal->parent_path;
                target_journal_db->comment = journal->comment;
                target_journal_db->name = journal->name;
                target_journal_db->extension = journal->extension;
                if (image->completed || image->canceled)
                    target_journal_db->next_block_index = journal->next_block_index;
                else
                    target_journal_db->next_block_index = -1;
                if (op.replicate_journal(target_journal_db, true)){
                    uint64_t next_block_index = op.get_parallels_journal_next_block_index();
                    if (image->completed){
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            if (next_block_index > index){
                                std::wstring _file = image->get_block_name(index);
                                source_op->remove_file(_file);
                            }
                        }
                        if (1 == _conns.size())
                            op.clear();
                        else{
                            macho::windows::lock_able::ptr lock_obj = op.get_source_journal_lock();
                            macho::windows::auto_lock lock(*lock_obj);
                            if (op.are_parallel_journals_closed())
                                op.clear();
                            else if (op.is_source_journal_existing())
                                op.close_journal(journal);
                        }
                    }
                    else if (image->canceled){
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            std::wstring _file = image->get_block_name(index);
                            source_op->remove_file(_file);
                        }
                        if (1 == _conns.size())
                            op.clear();
                        else{
                            macho::windows::lock_able::ptr lock_obj = op.get_source_journal_lock();
                            macho::windows::auto_lock lock(*lock_obj);                       
                            if (op.are_parallel_journals_closed())
                                op.clear();
                            else if (op.is_source_journal_existing())
                                op.close_journal(journal);
                        }
                    }
                    else{
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            if (next_block_index > index){
                                std::wstring _file = image->get_block_name(index);
                                source_op->remove_file(_file);
                            }
                        }
                    }
                }
            }
        }
    }
}

void repeat_job::remove()
{
    _is_removing = true;
    if (_running.try_lock()){
        earse_image_data();
        boost::filesystem::remove(_config_file);
        boost::filesystem::remove(_status_file);
        boost::filesystem::remove_all(_config_file.parent_path() / _id);
        _running.unlock();
    }
}

irm_imagex_local_op::ptr repeat_job::_get_source_op()
{
    irm_imagex_local_op::ptr op;
    if (_conns.size())
    {
        uint16_t _magic = MAGIC;
        if (_conns.begin()->options.count("magic"))
        {
            try 
            {
                _magic = boost::lexical_cast<uint16_t>(_conns.begin()->options.find("magic")->second);
            }
            catch (boost::bad_lexical_cast const&)
            {
            }
        }
        op = irm_imagex_local_op::ptr(new irm_imagex_local_op(_conns.begin()->detail.local.path, _config_file.parent_path(), _config_file.parent_path(), _magic, "r"));
    }
    return op;
}

irm_imagex_op::ptr repeat_job::_get_target_op()
{
    int i = 0;
    irm_imagex_op::vtr ops;
    foreach(connection conn, _conns)
    {
        uint16_t _magic = MAGIC;
        if (conn.id == _connection_id)
        {
            if (conn.options.count("magic"))
            {
                try
                {
                    _magic = boost::lexical_cast<uint16_t>(conn.options["magic"]);
                }
                catch (boost::bad_lexical_cast const&)
                {
                }
            }

            int      worker_threads = NumberOfWorkers;
            registry reg;
            if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)))
            {
                if (reg[L"NumberOfWorkers"].exists() && (DWORD)reg[L"NumberOfWorkers"] > 0)
                {
                    worker_threads = (DWORD)reg[L"NumberOfWorkers"];
                }
            }

            switch (conn.type)
            {
            case connection_type::LOCAL_FOLDER_EX:
            {
                i++;
                irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_local_op(_conns.begin()->detail.remote.path, _config_file.parent_path(), _config_file.parent_path(), _magic, "r"));
                if (op)
                    ops.push_back(op);
                break;
            }

            case connection_type::WEBDAV_EX:
            {
                i++;
                webdav::ptr dav = webdav::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.port, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
                if (dav)
                {
                    irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_webdav_op(dav, _config_file.parent_path(), _config_file.parent_path(), _magic, "r", worker_threads));
                    if (op)
                        ops.push_back(op);
                    else
                    {
                        _state |= mgmt_job_state::job_state_error;
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Cannot connect to the target path %1%.") % conn.detail.remote.path));
                    }
                }
                break;
            }

            case  connection_type::S3_BUCKET_EX:
            {
                i++;
                aws_s3::ptr s3 = aws_s3::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.s3_region, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
                if (s3)
                {
                    irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_s3_op(s3, _config_file.parent_path(), _config_file.parent_path(), _magic, "r", worker_threads));
                    if (op)
                        ops.push_back(op);
                    else
                    {
                        _state |= mgmt_job_state::job_state_error;
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_INITIAL_FAIL, (record_format("Cannot connect to the target path %1%.") % conn.detail.remote.path));
                    }
                }
                break;
            }
            default:
                break;
            }
            break;
        }
    }

    if (ops.empty())
    {
        return irm_imagex_op::ptr();
    }
    return ops[0];
}

void repeat_job::execute()
{
    VALIDATE;
    if ((_state & mgmt_job_state::job_state_finished) || _is_canceled || _is_removing)
    {
        return;
    }

    _latest_enter_time = _latest_leave_time = boost::date_time::not_a_date_time;
    _latest_enter_time = boost::posix_time::microsec_clock::universal_time();
    _terminated = false;
    _state &= ~mgmt_job_state::job_state_error;
    save_status();
    irm_imagex_local_op::ptr  source_op = _get_source_op();
    irm_imagex_op::ptr        target_op = _get_target_op();
    if (!_is_image_uploaded(_parent, _connection_id)){
        LOG(LOG_LEVEL_WARNING, _T("Waitting for the parent image (%s) uploading."), macho::stringutils::convert_utf8_to_unicode(_parent).c_str());
    }
    else if (source_op && target_op)
    {
        irm_transport_image_blks_chg_journal::ptr target_journal_db;
        relay_op op(_id, _group, source_op, target_op, _connection_id, _conns);
        LOG(LOG_LEVEL_INFO, _T("Start replicate blocks for(%s)."), _group.c_str());
        while (true)
        {
            _state = mgmt_job_state::job_state_replicating;
            irm_transport_image::ptr image = op.get_source_image_info();
            irm_transport_image_blks_chg_journal::ptr journal = op.get_source_journal_info();
            if (image){
                _base_name = macho::stringutils::convert_unicode_to_utf8(image->base_name);
                save_status();
            }
            if (image && journal)
            {
                if (!target_journal_db)
                {
                    if (-1 == _progress.journal_index)
                    {
                        target_journal_db = irm_transport_image_blks_chg_journal::ptr(new irm_transport_image_blks_chg_journal());
                        target_journal_db->parent_path = journal->parent_path;
                        target_journal_db->comment = journal->comment;
                        target_journal_db->name = journal->name;
                        target_journal_db->extension = journal->extension;
                    }
                    else
                    {
                        target_journal_db = op.get_target_journal_info();
                    }
                }

                if (!target_journal_db)
                {
                    LOG(LOG_LEVEL_ERROR, _T("Target journal is not available for disk (%s)."), _group.c_str());
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Target journal is not available for disk %1%.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                    break;
                }
                else if (image->canceled && !image->checksum)
                {
                    LOG(LOG_LEVEL_INFO, _T("Abort replicating all data from disk (%s)."), _group.c_str());
                    record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Abort replicating all data from disk %1%.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                    _progress.completed = false;				
                    if (journal->is_version_2()){
                        for (uint64_t index = 0; index < journal->next_block_index; index++){
                            op.remove_source_file(image->get_block_name(index));
                        }
                        target_journal_db->next_block_index = journal->next_block_index;
                        if (op.replicate_journal(target_journal_db, true) && op.replicate_image_remote(image)){
                            if (1 == _conns.size())
                                op.clear();
                            else{
                                macho::windows::lock_able::ptr lock_obj = op.get_source_journal_lock();
                                macho::windows::auto_lock lock(*lock_obj);
                                if (op.are_parallel_journals_closed())
                                    op.clear();
                                else if (op.is_source_journal_existing())
                                    op.close_journal(journal);
                            }
                            _state = mgmt_job_state::job_state_finished | mgmt_job_state::job_state_error;
                        }
                    }
                    else{
                        op.clear();
                        _state = mgmt_job_state::job_state_finished | mgmt_job_state::job_state_error;
                    }
                    break;
                }
                else if (!image->completed)
                {
                    if (journal->is_version_2() && _progress.journal_index == (journal->next_block_index - 1))                    
                    {
                        LOG(LOG_LEVEL_WARNING, _T("Non-Block Mode: no more data for (%s). Break thread first."), _group.c_str());
                        break;
                    }
                    else if ( (!journal->is_version_2()) && _progress.journal_index == (journal->dirty_blocks_list.size() - 1))
                    {
                        LOG(LOG_LEVEL_WARNING, _T("Non-Block Mode: no more data for (%s). Break thread first."), _group.c_str());
                        break;
                    }
                    else if(!op.replicate_image(image))
                    {
                        LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s image file."), _group.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot update disk %1% image file.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                        break;
                    }
                }

                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Replication scheduled for disk %1%.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                save_status();
                relay_block_file::vtr  block_files;
                try
                {
                    uint64_t index = _progress.journal_index + 1;
                    if (journal->is_version_2()){
                        for (index; index < journal->next_block_index; index++){
                            relay_block_file::ptr b = relay_block_file::ptr(new relay_block_file());
                            b->name = image->get_block_name(index);
                            b->index = index;
                            block_files.push_back(b);
                        }
                    }
                    else{
                        _progress.completed_blocks.clear();
                        for (index; index < journal->dirty_blocks_list.size(); index++){
                            std::wstring _file = image->get_block_name(journal->dirty_blocks_list[index]);
                            auto it = std::find_if(block_files.begin(), block_files.end(), [&_file](const relay_block_file::ptr& obj){return obj->name == _file; });
                            if (it != block_files.end())
                                (*it)->index = index;
                            else{
                                relay_block_file::ptr b = relay_block_file::ptr(new relay_block_file());
                                b->name = _file;
                                b->number = journal->dirty_blocks_list[index];
                                b->index = index;
                                b->locks = op.get_source_lock_objects(_file);
                                foreach(lock_able::ptr lock, b->locks)
                                    lock->lock();
                                block_files.push_back(b);
                            }
                        }
                        std::sort(block_files.begin(), block_files.end(), relay_block_file::compare());
                    }
                    replication_task task(op, _progress);
                    task.image = image;
                    task.purge_data = _purge;
                    task.journal_db = target_journal_db;
                    task.register_job_is_canceled_function(boost::bind(&repeat_job::is_canceled, this));
                    task.register_job_save_callback_function(boost::bind(&repeat_job::save_status, this));
                    task.register_job_record_callback_function(boost::bind(&repeat_job::record, this, _1, _2, _3));
                    foreach(relay_block_file::ptr &_bk_file, block_files)
                        task.waitting.push_back(_bk_file);
                    if (task.waitting.size()){
                        int worker_threads = NumberOfWorkers;
                        int max_worker_threads = worker_threads;
                        int resume_thread_count = RESUME_THREAD_COUNT;
                        int resume_thread_time = RESUME_THREAD_TIME;
                        registry reg;
                        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                            if (reg[L"NumberOfWebDavWorkers"].exists() && (DWORD)reg[L"NumberOfWebDavWorkers"] > 0){
                                worker_threads = (DWORD)reg[L"NumberOfWebDavWorkers"];
                            }
                            if (reg[L"ResumeThreadCount"].exists() && (DWORD)reg[L"ResumeThreadCount"] > 0){
                                resume_thread_count = (DWORD)reg[L"ResumeThreadCount"];
                            }
                            if (reg[L"ResumeThreadTime"].exists() && (DWORD)reg[L"ResumeThreadTime"] > 0){
                                resume_thread_time = (DWORD)reg[L"ResumeThreadTime"];
                            }
                        }
                        while (worker_threads > 0){
                            task.terminated = false;
                            _state &= ~mgmt_job_state::job_state_error;
                            worker_threads = worker_threads > task.waitting.size() ? task.waitting.size() : worker_threads;
                            boost::thread_group  thread_pool;
                            for (int i = 0; i < worker_threads; i++)
                                thread_pool.create_thread(boost::bind(&replication_task::replicate, &task));
                            boost::unique_lock<boost::mutex> _lock(task.completion_lock);
                            int waitting_count = 0;
                            while (true)
                            {
                                task.completion_cond.timed_wait(_lock, boost::posix_time::milliseconds(60000));
                                {
                                    macho::windows::auto_lock lock(task.cs);
                                    if (task.error.size() != 0 || task.update_journal_error)
                                    {
                                        _state |= mgmt_job_state::job_state_error;
                                        task.terminated = true;
                                        resume_thread_count--;
                                        break;
                                    }
                                    else if ((task.processing.size() == 0 && task.waitting.size() == 0) || is_canceled())
                                    {
                                        task.terminated = true;
                                        break;
                                    }
                                    else if (resume_thread_count && max_worker_threads > worker_threads && task.waitting.size() > worker_threads){
                                        if (waitting_count >= resume_thread_time){
                                            thread_pool.create_thread(boost::bind(&replication_task::replicate, &task));
                                            waitting_count = 0;
                                            worker_threads++;
                                        }
                                        waitting_count++;
                                    }
                                }
                            }
                            thread_pool.join_all();
                            worker_threads--;
                            if ((task.error.size() == 0 && task.processing.size() == 0 && task.waitting.size() == 0) || is_canceled()){
                                break;
                            }
                            else if (worker_threads){
                                foreach(relay_block_file::ptr& b, task.processing){
                                    task.waitting.push_front(b);
                                }
                                task.processing.clear();
                                task.error.clear();
                                task.update_journal_error = false;
                                std::sort(task.waitting.begin(), task.waitting.end(), relay_block_file::compare());
                                boost::this_thread::sleep(boost::posix_time::seconds(5));
                            }
                            else {
                                break;
                            }
                        }
                    }
                    if (image->completed && task.error.size() == 0 && !op.replicate_image(image, true)){
                        LOG(LOG_LEVEL_ERROR, _T("Cannot update disk %s image file."), _group.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot update disk %1% image file.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                        break;
                    }
                }
                catch (macho::windows::lock_able::exception &ex){
                    _state |= mgmt_job_state::job_state_error;
                    LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                }
                if (image->mode != IRM_IMAGE_TRANSPORT_MODE){
                    foreach(relay_block_file::ptr &bk_file, block_files){
                        foreach(lock_able::ptr lock, bk_file->locks)
                            lock->unlock();
                    }
                }
                if (_state & mgmt_job_state::job_state_error){
                    save_status();
                    break;
                }

                if (image->completed && 
                    ((!journal->is_version_2() && _progress.journal_index == (journal->dirty_blocks_list.size() - 1)) ||
                    (journal->is_version_2() && _progress.journal_index == (journal->next_block_index - 1))))
                {
                    if (!op.flush_metafiles())
                    {
                        LOG(LOG_LEVEL_ERROR, _T("Cannot flush disk %s meta files."), _group.c_str());
                        record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_E_FAIL, (record_format("Cannot flush disk %1% meta files.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                    }
                    else
                    {
                        _progress.completed = true;
                        _state = mgmt_job_state::job_state_finished;
                        uint64_t next_block_index = op.get_parallels_journal_next_block_index();
                        for (relay_block_file::vtr::iterator b = _progress.purgging.begin(); b != _progress.purgging.end();)
                        {
                            //macho::windows::auto_lock lock(*(*b)->lock);
                            if (next_block_index > (*b)->index && !op.remove_source_file((*b)->name))
                            {
                                LOG(LOG_LEVEL_ERROR, _T("Cannot remove the data block file (%s)."), (*b)->name.c_str());
                            }
                            b = _progress.purgging.erase(b);
                        }
                        if (_purge){
                            if (1 == _conns.size()){
                                op.clear();
                                LOG(LOG_LEVEL_INFO, _T("Disk %s replication completed."), _group.c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Disk %1% replication completed.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                                _is_removing = true;
                                save_status();
                                break;
                            }
                            else{
                                macho::windows::lock_able::ptr lock_obj = op.get_source_journal_lock();
                                macho::windows::auto_lock lock(*lock_obj);
                                if (_is_removing = op.are_parallel_journals_closed())
                                    op.clear();
                                else if (op.is_source_journal_existing())
                                    _is_removing = op.close_journal(journal);
                                else
                                    _is_removing = true;
                                LOG(LOG_LEVEL_INFO, _T("Disk %s replication completed."), _group.c_str());
                                record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Disk %1% replication completed.") % macho::stringutils::convert_unicode_to_utf8(_group)));                               
                                save_status();
                                break;
                            }
                        }
                        else{
                            LOG(LOG_LEVEL_INFO, _T("Disk %s replication completed."), _group.c_str());
                            record((job_state::type)(_state & ~mgmt_job_state::job_state_error), error_codes::SAASAME_S_OK, (record_format("Disk %1% replication completed.") % macho::stringutils::convert_unicode_to_utf8(_group)));
                            _is_removing = true;
                            save_status();
                            break;
                        }             
                    }
                }
            }
            else if (!image)
            {
                LOG(LOG_LEVEL_ERROR, _T("Cannot open the meta file (%s)."), _group.c_str());
            }
            else if (!journal)
            {
                LOG(LOG_LEVEL_ERROR, _T("Cannot open the journal file (%s%s)"), _group.c_str(), IRM_IMAGE_JOURNAL_EXT);
            }
            save_status();
            if (_is_interrupted || _is_canceled)
                break;
            else if (!image || !journal)
            {
                LOG(LOG_LEVEL_WARNING, _T("Non-Block Mode: no more data for (%s). Break thread first."), _group.c_str());
                break;
            }
        }
    }
    _latest_leave_time = _latest_update_time = boost::posix_time::microsec_clock::universal_time();
    save_status();
}

void repeat_job::_update()
{
    //save_status();
}
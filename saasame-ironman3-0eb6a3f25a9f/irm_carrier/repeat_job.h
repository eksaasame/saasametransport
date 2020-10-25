#pragma once
#ifndef repeat_job_H
#define repeat_job_H

#include "job.h"
#include "macho.h"
#include "common\jobs_scheduler.hpp"
#include "..\gen-cpp\irm_imagex_op.h"

#define JOB_EXTENSION L".cjob"
#define _MAX_HISTORY_RECORDS 50

class relay_block_file{
public:
    typedef boost::shared_ptr<relay_block_file> ptr;
    typedef std::deque<ptr> queue;
    typedef std::vector<ptr> vtr;
    struct compare {
        bool operator() (relay_block_file::ptr const & lhs, relay_block_file::ptr const & rhs) const {
            return (*lhs).index < (*rhs).index;
        }
    };
    relay_block_file() : index(0), number(0), data(0){}
    std::wstring                   name;
    uint64_t                       index;
    int                            number;
    uint32_t                       data;
    macho::windows::lock_able::vtr locks;
};

struct relay_disk{
    typedef boost::shared_ptr<relay_disk> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    relay_disk() : journal_index(-1), completed(false){}
    macho::windows::critical_section cs;
    uint64_t                         journal_index;
    uint64_t                         data;
    bool                             completed;
    relay_block_file::vtr            purgging;
    relay_block_file::queue          completed_blocks;
};

class repeat_job : public macho::removeable_job{
public:
    typedef boost::shared_ptr<repeat_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::wstring, ptr> map;
    repeat_job(std::wstring id, std::string image_name, std::string parent, std::string connection_id);
    virtual ~repeat_job(){}
    typedef boost::signals2::signal<bool(std::string, std::string), macho::_or<bool>> is_image_uploaded;

    static repeat_job::vtr create_jobs(std::string image_name, std::string parent, std::set<connection>& conns);
    static repeat_job::ptr load(boost::filesystem::path config_file, boost::filesystem::path status_file);
    inline void register_is_image_uploaded_callback_function(is_image_uploaded::slot_type slot){
        _is_image_uploaded.connect(slot);
    }
    bool virtual save_config();
    bool virtual save_status();
    bool virtual load_status(std::wstring status_file);
    void virtual record(saasame::transport::job_state::type state, int error, record_format& format);
    void virtual cancel(){ _is_canceled = true; interrupt(); }
    void virtual remove();
    void virtual earse_image_data();
    virtual void interrupt();
    virtual void execute();
    int          state() const { return _state; }
    std::string image_name() const { return _image_name; }
    std::string base_name() const { return _base_name; }
    std::set<connection> connections()  const { return _conns; }
    std::string connection_id() const { return _connection_id; }
protected:
    bool is_canceled() { return _is_canceled || _is_interrupted; }
    virtual irm_imagex_local_op::ptr                                      _get_source_op();
    virtual saasame::ironman::imagex::irm_imagex_op::ptr                  _get_target_op();
    void                                                                  _update( );
    is_image_uploaded                                                     _is_image_uploaded;
    macho::windows::critical_section                                      _cs;
    std::string                                                           _parent;
    std::string                                                           _image_name;
    std::string                                                           _base_name;
    bool                                                                  _terminated;
    bool                                                                  _is_canceled;
    bool                                                                  _is_interrupted;
    bool                                                                  _purge;
    int                                                                   _state;
    history_record::vtr                                                   _histories;
    std::set<connection>                                                  _conns;
    boost::filesystem::path                                               _config_file;
    boost::filesystem::path                                               _status_file;
    boost::posix_time::ptime                                              _created_time;
    boost::posix_time::ptime                                              _latest_update_time;
    boost::posix_time::ptime                                              _latest_enter_time;
    boost::posix_time::ptime                                              _latest_leave_time;
    relay_disk                                                            _progress;
    std::string                                                           _connection_id;
private:
    static repeat_job::ptr _load_config(boost::filesystem::path& config_file);
};

#endif
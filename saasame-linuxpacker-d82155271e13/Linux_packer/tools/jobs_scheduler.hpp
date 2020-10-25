#pragma once

#ifndef JOBS_SCHEDULER_H
#define JOBS_SCHEDULER_H

#include <boost/date_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/signals2/signal.hpp>
//#include <boost/detail/interlocked.hpp> //not need for linux
#include <map>
#include <set>
#include <deque>
#include <string.h>
#include "exception_base.hpp"

#include <string.h>
using namespace mumi;

class trigger : virtual public boost::noncopyable {
public:
    typedef boost::shared_ptr<trigger> ptr;
    struct cmp_trigger_ptr {
        bool operator() (trigger::ptr const & lhs, trigger::ptr const & rhs) const {
            return (*rhs)._priority < (*lhs)._priority;
        }
    }; 
    typedef std::vector<trigger::ptr> vtr;
    trigger() : _start_time(boost::posix_time::microsec_clock::universal_time()), _priority(0){}
    virtual trigger::ptr             clone() = 0;
    virtual bool                     may_fire_again()           const = 0;
    virtual boost::posix_time::ptime get_next_fire_time()       const = 0;
    virtual void                     set_latest_finished_time(boost::posix_time::ptime finished_time = boost::posix_time::microsec_clock::universal_time()){
        _latest_finished_time = finished_time;
    }
    virtual void                     set_fire_time(boost::posix_time::ptime fire_time = boost::posix_time::microsec_clock::universal_time()){
        _final_fire_time = fire_time;
    }
    boost::posix_time::ptime get_start_time() const { return _start_time; }
    boost::posix_time::ptime get_previous_finished_time() const { return _latest_finished_time; }
    boost::posix_time::ptime get_previous_fire_time() const { return _final_fire_time; }
    bool operator < (const trigger &t) const{
        return _priority < t._priority;
    }
    friend class scheduler;
protected:
    void clone_values(const trigger& t){
        _name                   = t._name;
        _description            = t._description;
        _job_id                 = t._job_id;
        _start_time             = t._start_time;
        _end_time               = t._end_time;
        _latest_finished_time   = t._latest_finished_time;
        _priority               = t._priority;
    }
    std::string                                     _name;
    std::string                                     _description;
    std::string                                     _job_id;
    boost::posix_time::ptime                         _start_time;
    boost::posix_time::ptime                         _end_time;
    boost::posix_time::ptime                         _final_fire_time;
    boost::posix_time::ptime                         _latest_finished_time;
    int                                              _priority;
};

class run_once_trigger : virtual public trigger{
public:
    run_once_trigger(
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time()) {
        _name = "run_once_trigger";
        _start_time = start_time;
        _latest_finished_time = boost::date_time::not_a_date_time;
        _priority = -1;
    }
    virtual ~run_once_trigger(){}
    virtual trigger::ptr             clone(){
        trigger::ptr t = trigger::ptr(new run_once_trigger( _start_time ));
        trigger::clone_values((*t));
        return t;
    }
    virtual bool                     may_fire_again() const { 
        return (_final_fire_time == boost::date_time::not_a_date_time);
    }
    virtual boost::posix_time::ptime get_next_fire_time() const { 
        return _start_time;
    }
};

class interval_trigger : virtual public trigger{
public:
    interval_trigger(
        boost::posix_time::time_duration repeat_interval = boost::posix_time::seconds(15),
        std::size_t repeat_count = 0, 
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time(), 
        boost::posix_time::ptime end_time = boost::date_time::not_a_date_time, boost::posix_time::ptime latest_finished_time = boost::date_time::not_a_date_time) {
        _name = "interval_trigger";
#pragma push_macro("max")
#undef max
        if ( repeat_interval.total_seconds() > 0 )
            _repeat_interval = repeat_interval;
        else
            _repeat_interval = boost::posix_time::minutes(std::numeric_limits<long>::max());               
#pragma pop_macro("max")
        _repeat_count = repeat_count;
        _start_time = start_time;
        _end_time = end_time;
        if (latest_finished_time >= _start_time)
            _final_fire_time = _latest_finished_time = latest_finished_time;
        _ref_count = 0;
        _priority = 0;
    }
    virtual ~interval_trigger(){}
    virtual trigger::ptr             clone(){
        trigger::ptr t = trigger::ptr(new interval_trigger(_repeat_interval, _repeat_count, _start_time, _end_time, _latest_finished_time));
        trigger::clone_values((*t));
        return t;
    }
    virtual bool                     may_fire_again() const {
        if ((_end_time != boost::date_time::not_a_date_time) && (_end_time < boost::posix_time::microsec_clock::universal_time()))
            return false;
        else
            return _repeat_count ? _repeat_count > _ref_count : true;
    }
    virtual boost::posix_time::ptime get_next_fire_time() const {
        boost::posix_time::time_duration diff(_latest_finished_time - _start_time);
        uint64_t df = (uint64_t)_repeat_interval.total_seconds() - ((uint64_t)diff.total_seconds() % (uint64_t)_repeat_interval.total_seconds());
#pragma push_macro("max")
#undef max
        boost::posix_time::seconds t_offset(df > std::numeric_limits<long>::max() ? std::numeric_limits<long>::max() : (long)df);
#pragma pop_macro("max")
        return (_final_fire_time != boost::date_time::not_a_date_time) ? (_latest_finished_time != boost::date_time::not_a_date_time ? _latest_finished_time + t_offset : _start_time + _repeat_interval) : _start_time;
    }
    virtual void                     set_fire_time(boost::posix_time::ptime fire_time = boost::posix_time::microsec_clock::universal_time()){
        _final_fire_time = fire_time;
        if (_repeat_count) __sync_fetch_and_add(&_ref_count,1); //use __sync_fetch_and_add to replace
    }
protected:
    void virtual clone_values(const interval_trigger& t){
        trigger::clone_values(t);
        _repeat_count       = t._repeat_count;
        _repeat_interval    = t._repeat_interval;
    }
    std::size_t                                      _repeat_count;
    boost::posix_time::time_duration                 _repeat_interval;
    long                                             _ref_count;
};

class interval_trigger_ex : virtual public interval_trigger{
public:
    interval_trigger_ex(
        boost::posix_time::time_duration repeat_interval = boost::posix_time::seconds(15),
        std::size_t repeat_count = 0,
        boost::posix_time::ptime start_time = boost::posix_time::microsec_clock::universal_time(),
        boost::posix_time::time_duration duration = boost::posix_time::hours(24),
        boost::posix_time::ptime end_time = boost::date_time::not_a_date_time, boost::posix_time::ptime latest_finished_time = boost::date_time::not_a_date_time) :        
        interval_trigger(repeat_interval, repeat_count, start_time, end_time, latest_finished_time), _duration(duration){
        _name = "interval_trigger_ex";
    }

    virtual trigger::ptr             clone(){
        trigger::ptr t = trigger::ptr(new interval_trigger_ex(_repeat_interval, _repeat_count, _start_time, _duration, _end_time, _latest_finished_time));
        trigger::clone_values((*t));
        return t;
    }

    virtual boost::posix_time::ptime get_next_fire_time() const {
        boost::posix_time::ptime next_fire_time = interval_trigger::get_next_fire_time();
        if (_duration.total_seconds() > 0){
            boost::posix_time::ptime _start = boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time().date(), _start_time.time_of_day());
            boost::posix_time::ptime _end = _start + _duration;
            if ((next_fire_time >= _start) && (next_fire_time <= _end))
                return next_fire_time;
            else
                return  _start + boost::posix_time::hours(24);
        }
        return next_fire_time;
    }
protected:  
    boost::posix_time::time_duration _duration;
};

struct job_execution_context;
class job : virtual public boost::noncopyable {
public:
    struct  exception : virtual public exception_base {};
    typedef boost::shared_ptr<job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    typedef boost::signals2::signal<void(const trigger::ptr&, const job_execution_context&)> job_to_be_executed;
    typedef boost::signals2::signal<void(const trigger::ptr&, const job_execution_context&, const job::exception&)> job_was_executed;
    job(std::string id, std::string group) : _id(id), _group(group) {}
    virtual ~job(){}
    virtual void execute() = 0;
    std::string inline id() const { return _id; }
    std::string inline group() const { return _group; }
    std::string inline type() const { return _type; }
    inline void register_job_to_be_executed_callback_function(job_to_be_executed::slot_type slot){
        _job_to_be_executed.connect(slot);
    }
    inline void register_job_was_executed_callback_function(job_was_executed::slot_type slot){
        _job_was_executed.connect(slot);
    }
    friend class scheduler;
protected:
    std::string                       _type;
    std::string                       _description;
    std::string                       _id;
    std::string                       _group;
    boost::recursive_mutex             _running;
private:
    job_to_be_executed _job_to_be_executed;
    job_was_executed   _job_was_executed;
};

class interruptable_job : public job{
public:
    typedef boost::shared_ptr<interruptable_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    interruptable_job(std::string id, std::string group) : job(id, group) {}
    virtual ~interruptable_job(){}
    virtual void interrupt() = 0;
};

class removeable_job : public interruptable_job{
public:
    typedef boost::shared_ptr<removeable_job> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    removeable_job(std::string& id, std::string group) : interruptable_job(id, group), _is_removing(false){}
    virtual ~removeable_job(){}
    virtual void remove() = 0;
    friend class scheduler;
protected:
    bool             _is_removing;
};
   
struct job_execution_context : virtual public boost::noncopyable {
    typedef boost::shared_ptr<job_execution_context> ptr;
    typedef std::deque<ptr> queue;
    typedef std::map<std::string, ptr> map;
    job_execution_context(job::ptr &j) : job_(j), job_run_time(boost::posix_time::seconds(0)){}
    virtual ~job_execution_context(){}
    job::ptr                            job_;
    trigger::ptr                        trigger_;
    boost::posix_time::time_duration    job_run_time;
    boost::posix_time::ptime            fire_time_utc;
    boost::posix_time::ptime            previous_fire_time_utc;
};

class scheduler : public boost::noncopyable{
public:
    scheduler(int workers = 0) : _workers(workers), _seconds(1), _terminated(false){
        //initial(workers);
    }
    virtual ~scheduler(){ stop(); }
    void virtual start();
    void virtual run();
    void initial(int workers);

    void schedule_job(job::ptr job_,trigger & t/* = run_once_trigger()*/);
    void schedule_job(job::ptr job_, const trigger::vtr triggers);
    void schedule_jobs(job::vtr jobs, trigger & t/* = run_once_trigger()*/);
    void schedule_jobs(job::map jobs, trigger & t/* = run_once_trigger()*/);
    /*void schedule_job(job::ptr job_, trigger& t);
    void schedule_job(job::ptr job_, trigger::vtr triggers);
    void schedule_jobs(job::vtr jobs, trigger& t);
    void schedule_jobs(job::map jobs, trigger& t);*/
    bool interrupt_job(std::string job_id);
    void suspend_job(std::string job_id);
    void resume_job(std::string job_id);
    void suspend_group_jobs(std::string group);
    void resume_group_jobs(std::string group);
    void remove_group_jobs(std::string group);
    bool has_group_jobs(std::string group);
    void remove_job(std::string job_id);
    void remove_all_jobs();
    void stop();
    bool is_running(std::string job_id); 
    bool is_scheduled(std::string job_id);
    bool is_suspended(std::string job_id);
    void update_job_triggers(std::string job_id, std::vector<trigger> triggers, boost::posix_time::ptime latest_finished_time = boost::date_time::not_a_date_time);
    void update_job_triggers(std::string job_id, trigger::vtr triggers, boost::posix_time::ptime latest_finished_time = boost::date_time::not_a_date_time);

private:  
    void _run();
    void _run_job();
    void run_job(trigger::ptr t, job_execution_context::ptr jobex);
    job_execution_context::map                          _jobs;
    job_execution_context::map                          _suspended_jobs;
    job_execution_context::map                          _running_jobs;
    boost::mutex                                        _cs;
    boost::condition_variable                           _cond;
    boost::mutex                                        _mutex;
    job_execution_context::queue                        _queue;
    boost::thread_group                                 _thread_pool;
    boost::thread                                       _thread;
    long                                                _seconds;
    bool                                                _terminated;
    trigger::vtr                                        _triggers;
    std::vector<boost::thread*>                         _threads;
    int                                                 _workers;
};

inline void scheduler::start(){
    initial(_workers);
    if (!_thread.joinable())
        _thread = boost::thread(&scheduler::_run, this);
}

inline void scheduler::run(){
    boost::unique_lock<boost::mutex> lock(_cs);
    job_execution_context::map    can_run_jobs;
    trigger::vtr::iterator t = _triggers.begin();
    while (t != _triggers.end()){
        if (!(*t)->may_fire_again())
            t = _triggers.erase(t);
        else {
            if (!_jobs.count((*t)->_job_id)){
                if (!_suspended_jobs.count((*t)->_job_id))
                    t = _triggers.erase(t);
                else
                    t++;
            }
            else{
                job_execution_context::ptr run_job_ec = _jobs[(*t)->_job_id];
                can_run_jobs[(*t)->_job_id] = run_job_ec;
                boost::posix_time::ptime schedule_time = (*t)->get_next_fire_time();
                boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
                if ((now >= schedule_time) && (run_job_ec->fire_time_utc == boost::date_time::not_a_date_time)){                    
                    run_job_ec->job_run_time = boost::posix_time::seconds(0);
                    run_job_ec->fire_time_utc = now;
                    run_job_ec->trigger_ = *t;
                    {
                        boost::unique_lock<boost::mutex> _lock(_mutex);
                        _queue.push_back(run_job_ec);
                    }
                    _cond.notify_one();
                    (*t)->set_fire_time(now);
                }
                t++;
            }
        }
    }
    _jobs = can_run_jobs;
}

inline void scheduler::schedule_job(job::ptr job_,trigger & t) {
    boost::unique_lock<boost::mutex> lock(_cs);
    if (!(_suspended_jobs.count(job_->id()) || _jobs.count(job_->id())))
        _jobs[job_->id()] = job_execution_context::ptr(new job_execution_context(job_));
    trigger::ptr _t = t.clone();
    _t->_job_id = job_->id();
    _triggers.push_back(_t);
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

inline void scheduler::schedule_job(job::ptr job_, const trigger::vtr triggers) {
    boost::unique_lock<boost::mutex> lock(_cs);
    if (!(_suspended_jobs.count(job_->id()) || _jobs.count(job_->id())))
        _jobs[job_->id()] = job_execution_context::ptr(new job_execution_context(job_));
    for(const trigger::ptr &t: triggers){
        trigger::ptr _t = t->clone();
        _t->_job_id = job_->id();
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

inline void scheduler::schedule_jobs(job::vtr jobs,trigger & t) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job::vtr::iterator job_ = jobs.begin(); job_ != jobs.end(); job_++){
        if (!(_suspended_jobs.count((*job_)->id()) || _jobs.count((*job_)->id())))
            _jobs[(*job_)->id()] = job_execution_context::ptr(new job_execution_context((*job_)));
        trigger::ptr _t = t.clone();
        _t->_job_id = (*job_)->id();
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

inline void scheduler::schedule_jobs(job::map jobs,trigger & t) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job::map::iterator job_ = jobs.begin(); job_ != jobs.end(); job_++){
        if (!(_suspended_jobs.count(job_->second->id()) || _jobs.count(job_->second->id())))
            _jobs[job_->second->id()] = job_execution_context::ptr(new job_execution_context(job_->second));
        trigger::ptr _t = t.clone();
        _t->_job_id = job_->second->id();
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

inline bool scheduler::interrupt_job(std::string job_id){
    boost::unique_lock<boost::mutex> lock(_cs);
    if (_jobs.count(job_id)){
        job_execution_context::map::iterator p = _jobs.find(job_id);
        interruptable_job* _interrupt_job = dynamic_cast<interruptable_job*>(p->second->job_.get());
        if (_interrupt_job != NULL){
            _interrupt_job->interrupt();
            return true;
        }
    }
    return false;
}

inline void scheduler::suspend_job(std::string job_id){
    boost::unique_lock<boost::mutex> lock(_cs);
    if (_jobs.count(job_id)){
        job_execution_context::map::iterator p = _jobs.find(job_id);
        _suspended_jobs[job_id] = p->second;
        _jobs.erase(p);
    }
}

inline void scheduler::resume_job(std::string job_id){
    boost::unique_lock<boost::mutex> lock(_cs);
    if (_suspended_jobs.count(job_id)){
        job_execution_context::map::iterator p = _suspended_jobs.find(job_id);
        _jobs[job_id] = p->second;
        _suspended_jobs.erase(p);
    }
}

inline bool scheduler::has_group_jobs(std::string group){
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _jobs.begin(); p != _jobs.end(); p++){
        if (0 == strcasecmp(p->second->job_->group().c_str(), group.c_str()))
           return true;   
    }
    return false;
}

inline void scheduler::suspend_group_jobs(std::string group){
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _jobs.begin(); p != _jobs.end();){
        if (strcasecmp(p->second->job_->group().c_str(), group.c_str()))
            p++;
        else{
            _suspended_jobs[p->second->job_->id()] = p->second;
            _jobs.erase(p++);
        }
    }
}

inline void scheduler::resume_group_jobs(std::string group){
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _suspended_jobs.begin(); p != _suspended_jobs.end();){
        if (strcasecmp(p->second->job_->group().c_str(), group.c_str()))
            p++;
        else{
            _jobs[p->second->job_->id()] = p->second;
            _suspended_jobs.erase(p++);
        }
    }
}

inline void scheduler::remove_group_jobs(std::string group){
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _jobs.begin(); p != _jobs.end();){
        if (!strcasecmp(p->second->job_->group().c_str(), group.c_str())){
            if (is_running(p->second->job_->id())){
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    p++;
                }
                else{
                    _jobs.erase(p++);
                }
            }
            else{
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    _remove_job->remove();
                }
                _jobs.erase(p++);
            }
        }
        else
            p++;
    }
    for (job_execution_context::map::iterator p = _suspended_jobs.begin(); p != _suspended_jobs.end();){
        if (!strcasecmp(p->second->job_->group().c_str(), group.c_str())){
            if (is_running(p->second->job_->id())){
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                }
            }
            _suspended_jobs.erase(p++);
        }
        else
            p++;
    }
}

inline void scheduler::remove_job(std::string job_id){
    boost::unique_lock<boost::mutex> lock(_cs);
    if (is_running(job_id)){
        if (_jobs.count(job_id)){
            job_execution_context::map::iterator p = _jobs.find(job_id);
            if (p != _jobs.end()){
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                }
            }
        }
        if (_suspended_jobs.count(job_id)){
            job_execution_context::map::iterator p = _suspended_jobs.find(job_id);
            if (p != _jobs.end()){
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                }
            }
            _suspended_jobs.erase(p);
        }
    }
    else{
        if (_jobs.count(job_id)){
            job_execution_context::map::iterator p = _jobs.find(job_id);
            if (p != _jobs.end()){
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    _remove_job->remove();
                }
            }
            _jobs.erase(p);
        }

        if (_suspended_jobs.count(job_id)){         
            job_execution_context::map::iterator p = _suspended_jobs.find(job_id);
            if (p != _jobs.end()){
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL){
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    _remove_job->remove();
                }
            }
            _suspended_jobs.erase(p);
        }
    }
}

inline void scheduler::remove_all_jobs(){
    boost::unique_lock<boost::mutex> lock(_cs);
    _jobs.clear();
    _suspended_jobs.clear();
    _triggers.clear();
}

inline void scheduler::stop(){
    _terminated = true;
    if (_thread.joinable()){
        _thread.join();
    }
    {
        boost::unique_lock<boost::mutex> _lock(_mutex);
        _queue.clear();
        _running_jobs.clear();
    }
    _cond.notify_all();
    _thread_pool.join_all();
    for(boost::thread* t: _threads)
        _thread_pool.remove_thread(t);
    _threads.clear();
}

inline bool scheduler::is_scheduled(std::string job_id){
    boost::unique_lock<boost::mutex> lock(_cs);
    return 0 != _jobs.count(job_id) || 0 != _suspended_jobs.count(job_id);
}

inline bool scheduler::is_running(std::string job_id){
    boost::unique_lock<boost::mutex> _lock(_mutex);
    return 0 != _running_jobs.count(job_id);
}

inline bool scheduler::is_suspended(std::string job_id){
    boost::unique_lock<boost::mutex> lock(_cs);
    return  0 != _suspended_jobs.count(job_id);
}

inline void scheduler::update_job_triggers(std::string job_id, std::vector<trigger> triggers, boost::posix_time::ptime latest_finished_time){
    boost::unique_lock<boost::mutex> lock(_cs);
    trigger::vtr::iterator t = _triggers.begin();
    while (t != _triggers.end()){
        if ((*t)->_job_id == job_id)
            t = _triggers.erase(t);
        else
            t++;
    }
    for(trigger &t: triggers){
        trigger::ptr _t = t.clone();
        _t->_job_id = job_id;
        if (latest_finished_time != boost::date_time::not_a_date_time && latest_finished_time > _t->get_start_time()){
            _t->set_fire_time(latest_finished_time);
            _t->set_latest_finished_time(latest_finished_time);
        }
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

inline void scheduler::update_job_triggers(std::string job_id, trigger::vtr triggers, boost::posix_time::ptime latest_finished_time){
    boost::unique_lock<boost::mutex> lock(_cs);
    trigger::vtr::iterator t = _triggers.begin();
    while (t != _triggers.end()){
        if ((*t)->_job_id == job_id)
            t = _triggers.erase(t);
        else
            t++;
    }
    for(trigger::ptr &t: triggers){
        trigger::ptr _t = t->clone();
        _t->_job_id = job_id;
        if (latest_finished_time != boost::date_time::not_a_date_time && latest_finished_time > _t->get_start_time()){
            _t->set_fire_time(latest_finished_time);
            _t->set_latest_finished_time(latest_finished_time);
        }
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

inline void scheduler::_run(){
    while (!_terminated){
        boost::this_thread::sleep(boost::posix_time::seconds(_seconds));
        if (_terminated)
            break;
        run();
    };
}

inline void scheduler::_run_job(){
    while (!_terminated){
        job_execution_context::ptr jobex;
        {
            boost::unique_lock<boost::mutex> _lock(_mutex);
            while (!_terminated && !_queue.size())
                _cond.wait(_lock);
            if (!_terminated){
                if (_queue.size()){
                    jobex = _queue.front();
                    _queue.pop_front();
                    _running_jobs[jobex->job_->id()] = jobex;
                }
                else{
                    jobex = NULL;
                }
            }
        }
        if (!_terminated && jobex){
            run_job(jobex->trigger_, jobex);
            {
                boost::unique_lock<boost::mutex> _lock(_mutex);
                _running_jobs.erase(jobex->job_->id());
            }
        }
    }
}

inline void scheduler::run_job(trigger::ptr t, job_execution_context::ptr jobex){
    jobex->job_->_job_to_be_executed(t, *jobex);
    boost::posix_time::ptime begin_time(boost::posix_time::microsec_clock::universal_time());
    try{
        boost::unique_lock<boost::recursive_mutex> lock(jobex->job_->_running);
        jobex->job_->execute();
    }
    catch (job::exception ex){
        jobex->job_run_time = boost::posix_time::microsec_clock::universal_time() - begin_time;
        jobex->job_->_job_was_executed(t, *jobex, ex);
        return;
    }
    catch (...){}

    removeable_job* _remove_job = dynamic_cast<removeable_job*>(jobex->job_.get());
    if (_remove_job != NULL && _remove_job->_is_removing){
        _remove_job->remove();
        {
            boost::unique_lock<boost::mutex> lock(_cs);
            job_execution_context::map::iterator p = _jobs.find(jobex->job_->id());
            if (_jobs.end() != p)
                _jobs.erase(p);
        }
    }
    
    job::exception ex;
    jobex->job_run_time = boost::posix_time::microsec_clock::universal_time() - begin_time;
    jobex->job_->_job_was_executed(t, *jobex, ex);
    jobex->previous_fire_time_utc = jobex->fire_time_utc;
    jobex->fire_time_utc = boost::date_time::not_a_date_time;
    t->set_latest_finished_time();
}

inline void scheduler::initial(int workers){
    if (_terminated){
        boost::unique_lock<boost::mutex> _lock(_mutex);
        _terminated = false;
    }
    _workers = workers;
    std::size_t workers_ = (_workers > 0) ? _workers : boost::thread::hardware_concurrency();
    if (workers_ > _thread_pool.size()){
        workers_ = workers_ - _thread_pool.size();
        for (std::size_t i = 0; i < workers_; ++i){
            _threads.push_back(_thread_pool.create_thread(boost::bind(&scheduler::_run_job, this)));
        }
    }
}
#endif

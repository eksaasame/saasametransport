#include "jobs_scheduler.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
trigger::ptr interval_trigger_ex::clone(){
    trigger::ptr t = trigger::ptr(new interval_trigger_ex(_repeat_interval, _repeat_count, _start_time, _duration, _end_time, _latest_finished_time));
    trigger::clone_values((*t));
    return t;
}
boost::posix_time::ptime interval_trigger_ex::get_next_fire_time() const {
    boost::posix_time::ptime next_fire_time = interval_trigger::get_next_fire_time();
    if (_duration.total_seconds() > 0) {
        boost::posix_time::ptime _start = boost::posix_time::ptime(boost::posix_time::microsec_clock::universal_time().date(), _start_time.time_of_day());
        boost::posix_time::ptime _end = _start + _duration;
        if ((next_fire_time >= _start) && (next_fire_time <= _end))
            return next_fire_time;
        else
            return  _start + boost::posix_time::hours(24);
    }
    return next_fire_time;
}
void scheduler::start() {
    initial(_workers);
    while (!_thread.joinable())
    {
        _thread = boost::thread(&scheduler::_run, this);
    }
}

void scheduler::run() {
    boost::unique_lock<boost::mutex> lock(_cs);
    job_execution_context::map    can_run_jobs;
    trigger::vtr::iterator t = _triggers.begin();
    while (t != _triggers.end()) {
        if (!(*t)->may_fire_again())
        {
            t = _triggers.erase(t);
            LOG_TRACE("1_triggers.erase\r\n");
        }
        else {
            if (!_jobs.count((*t)->_job_id)) {
                if (!_suspended_jobs.count((*t)->_job_id))
                {
                    t = _triggers.erase(t);
                    LOG_TRACE("2_triggers.erase\r\n");
                }
                else
                    t++;
            }
            else {
                job_execution_context::ptr run_job_ec = _jobs[(*t)->_job_id];
                can_run_jobs[(*t)->_job_id] = run_job_ec;
                boost::posix_time::ptime schedule_time = (*t)->get_next_fire_time();
                LOG_TRACE("schedule_time = %s\r\n", boost::posix_time::to_simple_string(schedule_time).c_str());
                boost::posix_time::ptime now(boost::posix_time::microsec_clock::universal_time());
                LOG_TRACE("now = %s\r\n", boost::posix_time::to_simple_string(now).c_str());
                if ((now >= schedule_time) && (run_job_ec->fire_time_utc == boost::date_time::not_a_date_time)) {
                    run_job_ec->job_run_time = boost::posix_time::seconds(0);
                    run_job_ec->fire_time_utc = now;
                    run_job_ec->trigger_ = *t;
                    {
                        boost::unique_lock<boost::mutex> _lock(_mutex);
                        LOG_TRACE("_queue.push_back\r\n");
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

void scheduler::schedule_job(job::ptr job_, trigger & t) {
    FUN_TRACE;
    boost::unique_lock<boost::mutex> lock(_cs);
    if (!(_suspended_jobs.count(job_->id()) || _jobs.count(job_->id())))
        _jobs[job_->id()] = job_execution_context::ptr(new job_execution_context(job_));
    trigger::ptr _t = t.clone();
    _t->_job_id = job_->id();
    _triggers.push_back(_t);
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

void scheduler::schedule_job(job::ptr job_, const trigger::vtr triggers) {
    boost::unique_lock<boost::mutex> lock(_cs);
    if (!(_suspended_jobs.count(job_->id()) || _jobs.count(job_->id())))
        _jobs[job_->id()] = job_execution_context::ptr(new job_execution_context(job_));
    for (const trigger::ptr &t : triggers) {
        trigger::ptr _t = t->clone();
        _t->_job_id = job_->id();
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

void scheduler::schedule_jobs(job::vtr jobs, trigger & t) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job::vtr::iterator job_ = jobs.begin(); job_ != jobs.end(); job_++) {
        if (!(_suspended_jobs.count((*job_)->id()) || _jobs.count((*job_)->id())))
            _jobs[(*job_)->id()] = job_execution_context::ptr(new job_execution_context((*job_)));
        trigger::ptr _t = t.clone();
        _t->_job_id = (*job_)->id();
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

void scheduler::schedule_jobs(job::map jobs, trigger & t) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job::map::iterator job_ = jobs.begin(); job_ != jobs.end(); job_++) {
        if (!(_suspended_jobs.count(job_->second->id()) || _jobs.count(job_->second->id())))
            _jobs[job_->second->id()] = job_execution_context::ptr(new job_execution_context(job_->second));
        trigger::ptr _t = t.clone();
        _t->_job_id = job_->second->id();
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

bool scheduler::interrupt_job(std::string job_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    if (_jobs.count(job_id)) {
        job_execution_context::map::iterator p = _jobs.find(job_id);
        interruptable_job* _interrupt_job = dynamic_cast<interruptable_job*>(p->second->job_.get());
        if (_interrupt_job != NULL) {
            _interrupt_job->interrupt();
            return true;
        }
    }
    return false;
}

void scheduler::suspend_job(std::string job_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    if (_jobs.count(job_id)) {
        job_execution_context::map::iterator p = _jobs.find(job_id);
        _suspended_jobs[job_id] = p->second;
        _jobs.erase(p);
    }
}

void scheduler::resume_job(std::string job_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    if (_suspended_jobs.count(job_id)) {
        job_execution_context::map::iterator p = _suspended_jobs.find(job_id);
        _jobs[job_id] = p->second;
        _suspended_jobs.erase(p);
    }
}

bool scheduler::has_group_jobs(std::string group) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _jobs.begin(); p != _jobs.end(); p++) {
        if (0 == strcasecmp(p->second->job_->group().c_str(), group.c_str()))
            return true;
    }
    return false;
}

void scheduler::suspend_group_jobs(std::string group) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _jobs.begin(); p != _jobs.end();) {
        if (strcasecmp(p->second->job_->group().c_str(), group.c_str()))
            p++;
        else {
            _suspended_jobs[p->second->job_->id()] = p->second;
            p = _jobs.erase(p);
            if (p != _jobs.end())
                ++p;
        }
    }
}

void scheduler::resume_group_jobs(std::string group) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _suspended_jobs.begin(); p != _suspended_jobs.end();) {
        if (strcasecmp(p->second->job_->group().c_str(), group.c_str()))
            p++;
        else {
            _jobs[p->second->job_->id()] = p->second;
            p = _suspended_jobs.erase(p);
            if (p != _suspended_jobs.end())
                ++p;
        }
    }
}

void scheduler::remove_group_jobs(std::string group) {
    boost::unique_lock<boost::mutex> lock(_cs);
    for (job_execution_context::map::iterator p = _jobs.begin(); p != _jobs.end();) {
        if (!strcasecmp(p->second->job_->group().c_str(), group.c_str())) {
            if (is_running(p->second->job_->id())) {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    p++;
                }
                else {
                    p = _jobs.erase(p);
                    if (p != _jobs.end())
                        ++p;
                }
            }
            else {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    _remove_job->remove();
                }
                p = _jobs.erase(p);
                if (p != _jobs.end())
                    ++p;
            }
        }
        else
            p++;
    }
    for (job_execution_context::map::iterator p = _suspended_jobs.begin(); p != _suspended_jobs.end();) {
        if (!strcasecmp(p->second->job_->group().c_str(), group.c_str())) {
            if (is_running(p->second->job_->id())) {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                }
            }
            p = _suspended_jobs.erase(p);
            if (p != _suspended_jobs.end())
                ++p;
        }
        else
            p++;
    }
}

void scheduler::remove_job(std::string job_id) {
    FUN_TRACE;
    boost::unique_lock<boost::mutex> lock(_cs);
    if (is_running(job_id)) {
        if (_jobs.count(job_id)) {
            job_execution_context::map::iterator p = _jobs.find(job_id);
            if (p != _jobs.end()) {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                }
            }
        }
        if (_suspended_jobs.count(job_id)) {
            job_execution_context::map::iterator p = _suspended_jobs.find(job_id);
            if (p != _jobs.end()) {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                }
            }
            _suspended_jobs.erase(p);
        }
    }
    else {
        if (_jobs.count(job_id)) {
            job_execution_context::map::iterator p = _jobs.find(job_id);
            if (p != _jobs.end()) {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    _remove_job->remove();
                }
            }
            _jobs.erase(p);
        }
        if (_suspended_jobs.count(job_id)) {
            job_execution_context::map::iterator p = _suspended_jobs.find(job_id);
            if (p != _jobs.end()) {
                removeable_job* _remove_job = dynamic_cast<removeable_job*>(p->second->job_.get());
                if (_remove_job != NULL) {
                    _remove_job->interrupt();
                    _remove_job->_is_removing = true;
                    _remove_job->remove();
                }
            }
            _suspended_jobs.erase(p);
        }
    }
}

inline void scheduler::remove_all_jobs() {
    boost::unique_lock<boost::mutex> lock(_cs);
    _jobs.clear();
    _suspended_jobs.clear();
    _triggers.clear();
}

void scheduler::stop() {
    _terminated = true;
    if (_thread.joinable()) {
        _thread.join();
    }
    {
        boost::unique_lock<boost::mutex> _lock(_mutex);
        _queue.clear();
        _running_jobs.clear();
    }
    _cond.notify_all();
    _thread_pool.join_all();
    for (boost::thread* t : _threads)
        _thread_pool.remove_thread(t);
    _threads.clear();
}

bool scheduler::is_scheduled(std::string job_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    return 0 != _jobs.count(job_id) || 0 != _suspended_jobs.count(job_id);
}

bool scheduler::is_running(std::string job_id) {
    boost::unique_lock<boost::mutex> _lock(_mutex);
    return 0 != _running_jobs.count(job_id);
}

bool scheduler::is_suspended(std::string job_id) {
    boost::unique_lock<boost::mutex> lock(_cs);
    return  0 != _suspended_jobs.count(job_id);
}

void scheduler::update_job_triggers(std::string job_id, std::vector<trigger> triggers, boost::posix_time::ptime latest_finished_time) {
    boost::unique_lock<boost::mutex> lock(_cs);
    trigger::vtr::iterator t = _triggers.begin();
    while (t != _triggers.end()) {
        if ((*t)->_job_id == job_id)
            t = _triggers.erase(t);
        else
            t++;
    }
    for (trigger &t : triggers) {
        trigger::ptr _t = t.clone();
        _t->_job_id = job_id;
        if (latest_finished_time != boost::date_time::not_a_date_time && latest_finished_time > _t->get_start_time()) {
            _t->set_fire_time(latest_finished_time);
            _t->set_latest_finished_time(latest_finished_time);
        }
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

void scheduler::update_job_triggers(std::string job_id, trigger::vtr triggers, boost::posix_time::ptime latest_finished_time) {
    boost::unique_lock<boost::mutex> lock(_cs);
    trigger::vtr::iterator t = _triggers.begin();
    while (t != _triggers.end()) {
        if ((*t)->_job_id == job_id)
            t = _triggers.erase(t);
        else
            t++;
    }
    for (trigger::ptr &t : triggers) {
        trigger::ptr _t = t->clone();
        _t->_job_id = job_id;
        if (latest_finished_time != boost::date_time::not_a_date_time && latest_finished_time > _t->get_start_time()) {
            _t->set_fire_time(latest_finished_time);
            _t->set_latest_finished_time(latest_finished_time);
        }
        _triggers.push_back(_t);
    }
    std::sort(_triggers.begin(), _triggers.end(), trigger::cmp_trigger_ptr());
}

void scheduler::_run() {
    FUN_TRACE;
    while (!_terminated) {
        boost::this_thread::sleep(boost::posix_time::seconds(_seconds));
        if (_terminated)
            break;
        run();
    };
}

void scheduler::_run_job() {
    /**/
    /*struct addrinfo hints, *res, *res0;
    char port[sizeof("65536")];
    sprintf(port, "%d", 18891);
    LOG_TRACE("3.1");
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    LOG_TRACE("carrier.c_str() = %s\r\n", "tpe2.saasame.com");
    LOG_TRACE("port.c_str() = %s\r\n", port);
    int error = getaddrinfo("tpe2.saasame.com", port, &hints, &res0);
    LOG_TRACE("error = %d", error); *

    /**/
    while (!_terminated) {
        job_execution_context::ptr jobex;
        {
            boost::unique_lock<boost::mutex> _lock(_mutex);
            while (!_terminated && !_queue.size())
                _cond.wait(_lock);
            if (!_terminated) {
                if (_queue.size()) {
                    jobex = _queue.front();
                    _queue.pop_front();
                    _running_jobs[jobex->job_->id()] = jobex;
                }
                else {
                    jobex = NULL;
                }
            }
        }
        if (!_terminated && jobex) {
            LOG_TRACE("run_job\r\n");
            run_job(jobex->trigger_, jobex);
            {
                boost::unique_lock<boost::mutex> _lock(_mutex);
                _running_jobs.erase(jobex->job_->id());
            }
        }
    }
}

void scheduler::run_job(trigger::ptr t, job_execution_context::ptr jobex) {
    FUN_TRACE;
    jobex->job_->_job_to_be_executed(t, *jobex);
    boost::posix_time::ptime begin_time(boost::posix_time::microsec_clock::universal_time());
    try {
        boost::unique_lock<boost::recursive_mutex> lock(jobex->job_->_running);
        LOG_TRACE("JOB_execute!\r\n")
        jobex->job_->execute();
    }
    catch (job::exception ex) {
        jobex->job_run_time = boost::posix_time::microsec_clock::universal_time() - begin_time;
        jobex->job_->_job_was_executed(t, *jobex, ex);
        return;
    }
    catch (...) {}

    removeable_job* _remove_job = dynamic_cast<removeable_job*>(jobex->job_.get());
    if (_remove_job != NULL && _remove_job->_is_removing) {
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

void scheduler::initial(int workers) {
    if (_terminated) {
        boost::unique_lock<boost::mutex> _lock(_mutex);
        _terminated = false;
    }
    _workers = workers;
    std::size_t workers_ = (_workers > 0) ? _workers : boost::thread::hardware_concurrency();
    if (workers_ > _thread_pool.size()) {
        workers_ = workers_ - _thread_pool.size();
        for (std::size_t i = 0; i < workers_; ++i) {
            /**/
            //set attr,
            boost::thread::attributes attrs;
            attrs.set_stack_size(attrs.get_stack_size() * 2);
            boost::thread th(attrs, boost::bind(&scheduler::_run_job, this));
            _thread_pool.add_thread(&th);
            _threads.push_back(&th);
            //create thread
            //add thread to _thread_pool
            //push back;
            //_threads.push_back(_thread_pool.create_thread(boost::bind(&scheduler::_run_job, this)));
        }
    }
}

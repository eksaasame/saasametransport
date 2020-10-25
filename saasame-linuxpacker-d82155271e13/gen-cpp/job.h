#pragma once
#ifndef __IRM_MGMT_SERVICE_JOB___
#define __IRM_MGMT_SERVICE_JOB___

#include "stdafx.h"
#include "common\jobs_scheduler.hpp"

enum mgmt_job_state{
    job_state_none = 0x1,
    job_state_initialed = 0x2,
    job_state_replicating = 0x4,
    job_state_replicated = 0x8,
    job_state_converting = 0x10,
    job_state_finished = 0x20,
    job_state_sche_completed = 0x40,
    job_state_recover = 0x80,
    job_state_discard = 0x40000000,
    job_state_error = 0x80000000
};

struct record_format{
    record_format(std::string f) : _fmt(f){
        _s = f;
    }
    template<class T>
    record_format&   operator%(const T& x){
        _args.push_back(boost::str(boost::format("%1%") % x));
        _fmt % x;
        return *this;
    }
    std::string str()  const { return boost::str(_fmt); }
    boost::format            _fmt;
    std::string              _s;
    std::vector<std::string> _args;
};

struct history_record {
    typedef boost::shared_ptr<history_record> ptr;
    typedef std::vector<ptr> vtr;
    struct compare {
        bool operator() (history_record::ptr const & lhs, history_record::ptr const & rhs) const {
            return (*rhs).time > (*lhs).time;
        }
    };
    history_record() : time(boost::posix_time::microsec_clock::universal_time()), error(0){
        original_time = time;
    }

    history_record(saasame::transport::job_state::type _state, int _error, record_format _fmt) : time(boost::posix_time::microsec_clock::universal_time()), state(_state), error(_error), description(_fmt.str()), format(_fmt._s), args(_fmt._args){
        original_time = time;
    }
    
    history_record(std::string _time, saasame::transport::job_state::type _state, int _error, record_format _fmt) : time(boost::posix_time::time_from_string(_time)), state(_state), error(_error), description(_fmt.str()), format(_fmt._s), args(_fmt._args){
        original_time = time;
    }

    history_record(saasame::transport::job_state::type _state, int _error, std::string _description) : time(boost::posix_time::microsec_clock::universal_time()), state(_state), error(_error), description(_description), format(_description){
        original_time = time;
    }

    history_record(std::string _time, saasame::transport::job_state::type _state, int _error, std::string _description) : time(boost::posix_time::time_from_string(_time)), state(_state), error(_error), description(_description), format(_description){
        original_time = time;
    }
    
    history_record(std::string _time, std::string _original_time, saasame::transport::job_state::type _state, int _error, std::string _description) : time(boost::posix_time::time_from_string(_time)), state(_state), error(_error), description(_description), format(_description){
        if (_original_time.length())
            original_time = boost::posix_time::time_from_string(_original_time);
        else
            original_time = time;
    }

    history_record(std::string _time, std::string _original_time, saasame::transport::job_state::type _state, int _error, std::string _description, std::string _format, std::vector<std::string> _args) : time(boost::posix_time::time_from_string(_time)), state(_state), error(_error), description(_description), format(_format), args(_args){
        if (_original_time.length())
            original_time = boost::posix_time::time_from_string(_original_time);
        else
            original_time = time;
    }

    boost::posix_time::ptime                 time;
    boost::posix_time::ptime                 original_time;
    saasame::transport::job_state::type      state;
    int                                      error;
    std::string                              description;
    std::string                              format;
    std::vector<std::string>                 args;
};

#endif
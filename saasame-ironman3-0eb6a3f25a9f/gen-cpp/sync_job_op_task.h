#pragma once
#ifndef sync_job_op_task_H
#define sync_job_op_task_H

#include "macho.h"
#include "saasame_types.h"
#include "job.h"
using namespace saasame::transport;
using namespace macho::windows;
using namespace macho;

class sync_job_op_task : public macho::job{
public:
    sync_job_op_task(std::string session, std::string machine_id, std::string mgmt_addr, bool is_ssl) : 
        macho::job(macho::guid_::create(), L""), 
        _session(session), 
        _machine_id(machine_id), 
        _mgmt_addr(mgmt_addr), 
        _is_ssl(is_ssl){
        LOG(LOG_LEVEL_RECORD, L"session id : %s, machine id: %s", 
            stringutils::convert_ansi_to_unicode(session).c_str(), 
            stringutils::convert_ansi_to_unicode(machine_id).c_str());
    }
    void get_tasks(std::vector<async_job_op_task>& tasks);
    void report_result(std::string task_id, saasame::transport::async_job_op_result::type result);
    virtual void execute();
private:
    std::string _machine_id;
    std::string _mgmt_addr;
    std::string _session;
    bool        _is_ssl;
};


#endif
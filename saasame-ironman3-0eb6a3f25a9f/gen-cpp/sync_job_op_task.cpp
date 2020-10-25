#include "stdafx.h"
#include "sync_job_op_task.h"
#ifndef IRM_TRANSPORTER
#include "loader_service.h"
#include "launcher_service.h"
#endif
#include "saasame_constants.h"
#include "TCurlClient.h"

#ifdef IRM_SCHEDULER
#include "management_service.h"
#include "common_service.h"
#include "common_connection_service.h"
#include "scheduler_service.h"
#else
#ifdef IRM_TRANSPORTER
#include "management_service.h"
#include "common_service.h"
#include "common_connection_service.h"
#include "scheduler_service.h"
#endif
#endif

#include "service_op.h"
#include "mgmt_op.h"

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

void sync_job_op_task::get_tasks(std::vector<async_job_op_task>& tasks){
#ifndef IRM_SCHEDULER
#ifndef IRM_TRANSPORTER
    SSL_library_init();
#endif
#endif
    std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
    boost::shared_ptr<saasame::transport::management_serviceClient>    client;
    boost::shared_ptr<TTransport>                                      transport;
    if (_is_ssl){
        boost::shared_ptr<TCurlClient>           http = boost::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % _mgmt_addr%uri)));
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
        boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
        client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
    }
    else{
        boost::shared_ptr<THttpClient>           http = boost::shared_ptr<THttpClient>(new THttpClient(_mgmt_addr, 80, uri));
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
        boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
        client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
    }
    try{
        transport->open();   
        client->query_async_job_op_task_list(tasks, _session, _machine_id);
        transport->close();
    }
    catch (TException & tx){
        LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        if (transport->isOpen())
            transport->close();
    }
}

void sync_job_op_task::report_result(std::string task_id, async_job_op_result::type result){
#ifndef IRM_SCHEDULER
#ifndef IRM_TRANSPORTER
    SSL_library_init();
#endif
#endif
    std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
    boost::shared_ptr<saasame::transport::management_serviceClient>    client;
    boost::shared_ptr<TTransport>                                      transport;
    if (_is_ssl){
        boost::shared_ptr<TCurlClient>           http = boost::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % _mgmt_addr%uri)));
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
        boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
        client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
    }
    else{
        boost::shared_ptr<THttpClient>           http = boost::shared_ptr<THttpClient>(new THttpClient(_mgmt_addr, 80, uri));
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
        boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
        client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
    }
    try{
        transport->open();
        bool ret = client->async_job_op_task_callback(_session, task_id, result);
        transport->close();
    }
    catch (TException & tx){
        LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
        if (transport->isOpen())
            transport->close();
    }
}

void sync_job_op_task::execute(){
    std::vector<async_job_op_task> tasks;
    get_tasks(tasks); 
    foreach(async_job_op_task& task, tasks){
        async_job_op_result::type result = async_job_op_result::JOB_OP_RESULT_UNKNOWN;
        macho::guid_ service_type = macho::guid_(task.service_type);
        switch (task.type){                
            case saasame::transport::async_job_op_type::JOB_OP_CREATE:{
#ifndef IRM_TRANSPORTER
                if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
                    job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);                    
                    if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else if(op.create_job(task.job_id, task.create_detail)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
                    connection_op conn_op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
                    job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);                    
                    if (!conn_op.create_or_modify_connection(task.conn)){
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                    else if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }    
                    else if (op.create_job(task.job_id, task.create_detail)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#else
                if (false){}
#endif
#ifdef IRM_SCHEDULER
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){

                    job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
#ifndef IRM_TRANSPORTER
                    connection_op conn_op(saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT);
                    if (!conn_op.create_or_modify_connection(task.conn)){
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                    else if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
#else
                    if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
#endif
                   
                    else if (op.create_job(task.job_id, task.create_detail)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#endif
                break;
            }

            case saasame::transport::async_job_op_type::JOB_OP_SUSPEND:{
#ifndef IRM_TRANSPORTER
                if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
                    job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
                    if (!op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                    else if (op.suspend_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
                    job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
                    if (!op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                    else if (op.suspend_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#else
                if (false){}
#endif
#ifdef IRM_SCHEDULER
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
                    job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                    if (!op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                    else if (op.suspend_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#endif
                break;
            }
            case saasame::transport::async_job_op_type::JOB_OP_REMOVE:{
#ifndef IRM_TRANSPORTER
                if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
                    job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
                    if (op.remove_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
                    job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
                    if (op.remove_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#else
                if (false){}
#endif
#ifdef IRM_SCHEDULER
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
                    job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                    if (op.remove_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#endif
                break;
            }
            case saasame::transport::async_job_op_type::JOB_OP_RESUME:{
#ifndef IRM_TRANSPORTER
                if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
                    job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
                    if (!op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                    else if (op.resume_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
                    job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
                    if (!op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                    else if (op.resume_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#else
                if (false){}
#endif
#ifdef IRM_SCHEDULER
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
                    job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                    if (!op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                    else if (op.resume_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#endif
                break;
            }
            case saasame::transport::async_job_op_type::JOB_OP_MODIFY:{ 
#ifndef IRM_TRANSPORTER
                if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
                    job_op_ex<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
                    if (op.update_job(task.job_id, task.create_detail)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#else
                if (false){}
#endif
#ifdef IRM_SCHEDULER
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
                    job_op_ex<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                    if (op.update_job(task.job_id,task.create_detail)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_FAILED;
                    }
                }
#endif
                break;
            }
            case saasame::transport::async_job_op_type::JOB_OP_VERIFY:{
#ifndef IRM_TRANSPORTER
                if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
                    job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
                    if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                }
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
                    job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
                    if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                }
#else
                if (false){}
#endif
#ifdef IRM_SCHEDULER
                else if (service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
                    job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
                    if (op.has_job(task.job_id)){
                        result = async_job_op_result::JOB_OP_RESULT_SUCCESS;
                    }
                    else{
                        result = async_job_op_result::JOB_OP_RESULT_NOT_FOUND;
                    }
                }
#endif
                break;
            }
            default:{
                break;
            }
        }
        report_result(task.task_id, result);
    }
}

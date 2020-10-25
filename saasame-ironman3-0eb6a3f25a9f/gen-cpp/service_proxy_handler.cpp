
#include "service_proxy_handler.h"
#include "loader_service.h"
#include "launcher_service.h"
#include "common_service.h"
#include "common_connection_service.h"
#include "virtual_packer_service.h"
#include "physical_packer_service.h"
#include "scheduler_service.h"
#include "carrier_service.h"
#include "service_op.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using namespace  ::saasame::transport;

bool service_proxy_handler::is_ssl_enable(){
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")){
        return true;
    }
    return false;
}

void service_proxy_handler::create_job_ex_p(job_detail& _return, const std::string& session_id, const std::string& job_id, const create_job_detail& create_job, const std::string& service_type){
    bool result = false;
    macho::guid_  _service_type(service_type);
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
        job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        result = op.create_job(job_id, create_job, _return.launcher);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.create_job(job_id, create_job, _return.scheduler);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.create_job(job_id, create_job, _return.loader);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    if (!result){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Cannot create job.";
        throw error;
    }
}

void service_proxy_handler::get_job_p(job_detail& _return, const std::string& session_id, const std::string& job_id, const std::string& service_type){
    bool result = false;
    macho::guid_  _service_type(service_type);
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
        job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        result = op.get_job(job_id, _return.launcher);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.get_job(job_id, _return.scheduler);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.get_job(job_id, _return.loader);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    if (!result){
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Cannot get job.";
        throw error;
    }
}

bool service_proxy_handler::interrupt_job_p(const std::string& session_id, const std::string& job_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
        job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        result = op.suspend_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.suspend_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.suspend_job(job_id);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::resume_job_p(const std::string& session_id, const std::string& job_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
        job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        result = op.resume_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.resume_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.resume_job(job_id);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::remove_job_p(const std::string& session_id, const std::string& job_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
        job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        result = op.remove_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.remove_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.remove_job(job_id);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::running_job_p(const std::string& session_id, const std::string& job_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE)){
        job_op<launcher_serviceClient, launcher_job_detail> op(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        result = op.running_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.running_job(job_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.running_job(job_id);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::update_job_p(const std::string& session_id, const std::string& job_id, const create_job_detail& create_job, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE)){
        job_op_ex<scheduler_serviceClient, replica_job_detail> op(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        result = op.update_job(job_id, create_job);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        job_op_ex<loader_serviceClient, loader_job_detail> op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.update_job(job_id, create_job);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::remove_snapshot_image_p(const std::string& session_id, const std::map<std::string, image_map_info> & images, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        thrift_connect<carrier_serviceClient> thrift(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        if (thrift.open())
            result = thrift.client()->remove_snapshot_image(session_id, images);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        if (thrift.open())
            result = thrift.client()->remove_snapshot_image(session_id, images);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::test_connection_p(const std::string& session_id, const connection& conn, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result= false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        svc_connection_op op(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        result = op.test_connection(conn);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        svc_connection_op op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.test_connection(conn);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;
}

bool service_proxy_handler::add_connection_p(const std::string& session_id, const connection& conn, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        svc_connection_op op(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        result = op.add_connection(conn);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        svc_connection_op op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.add_connection(conn);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;

}

bool service_proxy_handler::remove_connection_p(const std::string& session_id, const std::string& connection_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        svc_connection_op op(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        result = op.remove_connection(connection_id);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        svc_connection_op op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.remove_connection(connection_id);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;

}

bool service_proxy_handler::modify_connection_p(const std::string& session_id, const connection& conn, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        svc_connection_op op(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        result = op.modify_connection(conn);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        svc_connection_op op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.modify_connection(conn);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
    return result;

}

void service_proxy_handler::enumerate_connections_p(std::vector<connection> & _return, const std::string& session_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        svc_connection_op op(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        result = op.enumerate_connections(_return);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        svc_connection_op op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.enumerate_connections(_return);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
}

void service_proxy_handler::get_connection_p(connection& _return, const std::string& session_id, const std::string& connection_id, const std::string& service_type){
    macho::guid_  _service_type(service_type);
    bool result = false;
    if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.CARRIER_SERVICE)){
        svc_connection_op op(is_ssl_enable() ? g_saasame_constants.CARRIER_SERVICE_SSL_PORT : g_saasame_constants.CARRIER_SERVICE_PORT);
        result = op.get_connection(connection_id, _return);
    }
    else if (_service_type == macho::guid_(saasame::transport::g_saasame_constants.LOADER_SERVICE)){
        svc_connection_op op(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        result = op.get_connection(connection_id, _return);
    }
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_INVALID_ARG;
        error.why = "Invalid service type.";
        throw error;
    }
}

void service_proxy_handler::get_virtual_host_info_p(virtual_host& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_virtual_host_info(_return, session_id, host, username, password);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
}

void service_proxy_handler::get_virtual_machine_detail_p(virtual_machine& _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_virtual_machine_detail(_return, session_id, host, username, password, machine_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
}

void service_proxy_handler::get_virtual_hosts_p(std::vector<virtual_host> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_virtual_hosts(_return, session_id, host, username, password);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
}

bool service_proxy_handler::power_off_virtual_machine_p(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->power_off_virtual_machine(session_id, host, username, password, machine_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
    return false;
}

bool service_proxy_handler::remove_virtual_machine_p(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->remove_virtual_machine(session_id, host, username, password, machine_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
    return false;
}

void service_proxy_handler::get_virtual_machine_snapshots_p(std::vector<vmware_snapshot> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_virtual_machine_snapshots(_return,session_id, host, username, password, machine_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
}

bool service_proxy_handler::remove_virtual_machine_snapshot_p(const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& machine_id, const std::string& snapshot_id){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->remove_virtual_machine_snapshot(session_id, host, username, password, machine_id, snapshot_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
    return false;
}

void service_proxy_handler::get_datacenter_folder_list_p(std::vector<std::string> & _return, const std::string& session_id, const std::string& host, const std::string& username, const std::string& password, const std::string& datacenter){
    thrift_connect<virtual_packer_serviceClient> thrift(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_datacenter_folder_list(_return, session_id, host, username, password, datacenter);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "virtual packer service is not ready.";
        throw error;
    }
}

void service_proxy_handler::get_physical_machine_detail_p(physical_machine_info& _return, const std::string& session_id, const std::string& host, const machine_detail_filter::type filter){
    thrift_connect<scheduler_serviceClient> thrift(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_physical_machine_detail(_return, session_id, host, filter);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "scheduler service is not ready.";
        throw error;
    }
}

bool service_proxy_handler::verify_packer_to_carrier_p(const std::string& packer, const std::string& carrier, const int32_t port, const bool is_ssl){
    thrift_connect<scheduler_serviceClient> thrift(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->verify_packer_to_carrier(packer, carrier, port, is_ssl);
    return false;
}

void service_proxy_handler::take_packer_xray_p(std::string& _return, const std::string& session_id, const std::string& host){
    thrift_connect<scheduler_serviceClient> thrift(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->take_packer_xray(_return, session_id, host);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "scheduler service is not ready.";
        throw error;
    }
}

void service_proxy_handler::get_packer_service_info_p(service_info& _return, const std::string& session_id, const std::string& host){
    thrift_connect<scheduler_serviceClient> thrift(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_packer_service_info(_return, session_id, host);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "scheduler service is not ready.";
        throw error;
    }
}

bool service_proxy_handler::set_customized_id_p(const std::string& session_id, const std::string& disk_addr, const std::string& disk_id){
    thrift_connect<loader_serviceClient> thrift(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->set_customized_id(session_id, disk_addr, disk_id);
    return false;
}

void physical_packer_service_proxy_handler::packer_ping_p(service_info& _return, const std::string& session_id, const std::string& addr){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->packer_ping_p(_return, session_id, addr);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::take_snapshots_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const std::set<std::string> & disks){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->take_snapshots_p(_return, session_id, addr, disks);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::take_snapshots_ex_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const std::set<std::string> & disks, const std::string& pre_script, const std::string& post_script){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->take_snapshots_ex_p(_return, session_id, addr, disks, pre_script, post_script);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::take_snapshots2_p(std::vector<snapshot> & _return, const std::string& session_id, const std::string& addr, const take_snapshots_parameters& parameters){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->take_snapshots2_p(_return, session_id, addr, parameters);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::delete_snapshot_p(delete_snapshot_result& _return, const std::string& session_id, const std::string& addr, const std::string& snapshot_id){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->delete_snapshot_p(_return, session_id, addr, snapshot_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::delete_snapshot_set_p(delete_snapshot_result& _return, const std::string& session_id, const std::string& addr, const std::string& snapshot_set_id){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->delete_snapshot_set_p(_return, session_id, addr, snapshot_set_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::get_all_snapshots_p(std::map<std::string, std::vector<snapshot> > & _return, const std::string& session_id, const std::string& addr){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_all_snapshots_p(_return, session_id, addr);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::create_packer_job_ex_p(packer_job_detail& _return, const std::string& session_id, const std::string& addr, const std::string& job_id, const create_packer_job_detail& create_job){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->create_packer_job_ex_p(_return, session_id, addr, job_id, create_job);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

void physical_packer_service_proxy_handler::get_packer_job_p(packer_job_detail& _return, const std::string& session_id, const std::string& addr, const std::string& job_id, const std::string& previous_updated_time){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_packer_job_p(_return, session_id, addr, job_id, previous_updated_time);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

bool physical_packer_service_proxy_handler::interrupt_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->interrupt_packer_job_p(session_id, addr, job_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
    return false;
}

bool physical_packer_service_proxy_handler::resume_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->resume_packer_job_p(session_id, addr, job_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
    return false;
}

bool physical_packer_service_proxy_handler::remove_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->remove_packer_job_p(session_id, addr, job_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
    return false;
}

bool physical_packer_service_proxy_handler::running_packer_job_p(const std::string& session_id, const std::string& addr, const std::string& job_id){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->running_packer_job_p(session_id, addr, job_id);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
    return false;
}

void physical_packer_service_proxy_handler::enumerate_packer_disks_p(std::set<disk_info> & _return, const std::string& session_id, const std::string& addr, const enumerate_disk_filter_style::type filter){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->enumerate_packer_disks_p(_return, session_id, addr, filter);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}

bool physical_packer_service_proxy_handler::verify_packer_carrier_p(const std::string& session_id, const std::string& addr, const std::string& carrier, const bool is_ssl){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        return thrift.client()->verify_packer_carrier_p(session_id, addr, carrier, is_ssl);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
    return false;
}

void physical_packer_service_proxy_handler::get_packer_host_detail_p(physical_machine_info& _return, const std::string& session_id, const std::string& addr, const machine_detail_filter::type filter){
    thrift_connect<physical_packer_service_proxyClient> thrift(saasame::transport::g_saasame_constants.TRANSPORT_SERVICE_PORT);
    if (thrift.open())
        thrift.client()->get_packer_host_detail_p(_return, session_id, addr, filter);
    else{
        saasame::transport::invalid_operation error;
        error.what_op = saasame::transport::error_codes::SAASAME_E_FAIL;
        error.why = "transport service is not ready.";
        throw error;
    }
}


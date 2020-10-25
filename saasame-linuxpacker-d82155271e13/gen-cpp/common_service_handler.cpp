#include <macho.h>
#include "common_service_handler.h"
#include <VersionHelpers.h>
#include <codecvt>
#include "..\vcbt\vcbt\journal.h"
#include <thrift/transport/TSSLSocket.h>
#include "json_spirit.h"
#include <iphlpapi.h>
#include "archive.hpp"

#pragma comment(lib, "IPHLPAPI.lib")

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::server;
using namespace macho::windows;
using namespace macho;
using namespace json_spirit;

#define WORKING_BUFFER_SIZE 15000
#define MAX_TRIES 3

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

typedef struct _MS_DIGITAL_PRODUCTID
{
    unsigned int uiSize;
    BYTE bUnknown1[4];
    char szProductId[28];
    char szEditionId[16];
    BYTE bUnknown2[112];
} MS_DIGITAL_PRODUCTID, *PMS_DIGITAL_PRODUCTID;

typedef struct _MS_DIGITAL_PRODUCTID4
{
    unsigned int uiSize;
    BYTE bUnknown1[4];
    WCHAR szAdvancedPid[64];
    WCHAR szActivationId[72];
    WCHAR szEditionType[256];
    BYTE bUnknown2[96];
    WCHAR szEditionId[64];
    WCHAR szKeyType[64];
    WCHAR szEULA[64];
}MS_DIGITAL_PRODUCTID4, *PMS_DIGITAL_PRODUCTID4;

#ifndef MAX_ISCSI_NAME_LEN
#define MAX_ISCSI_NAME_LEN  223
#endif
static const TCHAR  szIscsiLib[] = _T("Iscsidsc.dll");
typedef HRESULT(WINAPI *fnGetIScsiInitiatorNodeName)(TCHAR* InitiatorNodeName);
static fnGetIScsiInitiatorNodeName          pfnGetIScsiInitiatorNodeName = NULL;

HRESULT _GetIScsiInitiatorNodeName(TCHAR* InitiatorNodeName);
static stdstring decode_ms_key(const LPBYTE lpBuf, const DWORD rpkOffset);
DWORD DetectSectorSize(LPCWSTR devName, PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR pAlignmentDescriptor);
int get_network_adapters(std::vector<network_info> &network_infos);
void get_network_dnss(std::vector<network_info> &network_infos);

const mValue& find_value(const mObject& obj, const std::string& name){
    mObject::const_iterator i = obj.find(name);
    assert(i != obj.end());
    assert(i->first == name);
    return i->second;
}

const bool find_value_bool(const mObject& obj, const std::string& name, bool default_value){
    mObject::const_iterator i = obj.find(name);
    if (i != obj.end()){
        return i->second.get_bool();
    }
    return default_value;
}

const std::string find_value_string(const mObject& obj, const std::string& name){
    mObject::const_iterator i = obj.find(name);
    if (i != obj.end()){
        return i->second.get_str();
    }
    return "";
}

const int find_value_int32(const json_spirit::mObject& obj, const std::string& name, int default_value){
    mObject::const_iterator i = obj.find(name);
    if (i != obj.end()){
        return i->second.get_int();
    }
    return default_value;
}

const mArray& find_value_array(const mObject& obj, const std::string& name){
    mObject::const_iterator i = obj.find(name);
    if (i != obj.end())
        return i->second.get_array();
    return mArray();
}

disk_universal_identify::disk_universal_identify(macho::windows::storage::disk& d) : _mbr_signature(0){
    _serial_number = macho::stringutils::convert_unicode_to_utf8(d.serial_number());
    _partition_style = d.partition_style();
    _mbr_signature = d.signature();
    if (d.guid().length()) 
        _gpt_guid = d.guid();
    _friendly_name = macho::stringutils::convert_unicode_to_utf8(d.friendly_name());
    _address = boost::str(boost::format("%d:%d:%d:%d") % d.scsi_port() % d.scsi_bus() % d.scsi_target_id() % d.scsi_logical_unit());
    _unique_id = macho::stringutils::convert_unicode_to_utf8(d.unique_id());
    _unique_id_format = (unique_id_format_type)d.unique_id_format();
}

disk_universal_identify::disk_universal_identify(const std::string& uri) : _mbr_signature(0) {
    mValue value;
    read(uri, value);
    _serial_number = find_value_string(value.get_obj(), "serial_number");
    _partition_style = (macho::windows::storage::ST_PARTITION_STYLE)find_value(value.get_obj(), "partition_style").get_int();
    _mbr_signature = find_value(value.get_obj(), "mbr_signature").get_int();
    _gpt_guid = find_value_string(value.get_obj(), "gpt_guid");
    _cluster_id = find_value_string(value.get_obj(), "cluster_id");
    _friendly_name = find_value_string(value.get_obj(), "friendly_name");
    _address = find_value_string(value.get_obj(), "address");
    _unique_id = find_value_string(value.get_obj(), "unique_id");
    _unique_id_format = (unique_id_format_type)find_value_int32(value.get_obj(), "unique_id_format");
}

disk_universal_identify::disk_universal_identify(macho::windows::cluster::disk& d) : _mbr_signature(0) {
    _friendly_name = macho::stringutils::convert_unicode_to_utf8(d.name());
    _cluster_id = macho::stringutils::convert_unicode_to_utf8( d.id());
    _partition_style = macho::windows::storage::ST_PST_MBR;
    //_address = boost::str(boost::format("%d:%d:%d:%d") % d.scsi_port() % d.scsi_bus() % d.scsi_target_id() % d.scsi_lun());
    try{
        if (d.gpt_guid().length()){
            _gpt_guid = macho::stringutils::convert_unicode_to_utf8(d.gpt_guid());
            if (d.gpt_guid().length())
                _partition_style = macho::windows::storage::ST_PST_GPT;            
        }
    }
    catch (...){}
    try{
        _mbr_signature = d.signature();
    }
    catch (...){} 
}

void disk_universal_identify::copy(const disk_universal_identify& disk_uri){
    _serial_number      = disk_uri._serial_number;
    _partition_style    = disk_uri._partition_style;
    _mbr_signature      = disk_uri._mbr_signature;
    _gpt_guid           = disk_uri._gpt_guid;
    _cluster_id         = disk_uri._cluster_id;
    _friendly_name      = disk_uri._friendly_name;
    _address            = disk_uri._address;
    _unique_id          = disk_uri._unique_id;
    _unique_id_format   = disk_uri._unique_id_format;
}

const disk_universal_identify &disk_universal_identify::operator = (const disk_universal_identify& disk_uri){
    if (this != &disk_uri)
        copy(disk_uri);
    return(*this);
}

bool disk_universal_identify::operator == (const disk_universal_identify& disk_uri){
    if (_cluster_id.length() && disk_uri._cluster_id.length())
        return  (_cluster_id == disk_uri._cluster_id);
    if (_unique_id_format == unique_id_format_type::FCPH_Name && _unique_id_format == disk_uri._unique_id_format && !_unique_id.empty() && !disk_uri._unique_id.empty())
        return _unique_id == disk_uri._unique_id;
    else if (_serial_number.length() > 4 && 
        disk_uri._serial_number.length() > 4 && 
        (_serial_number != "2020202020202020202020202020202020202020") && 
        (disk_uri._serial_number != "2020202020202020202020202020202020202020"))
        return (_serial_number == disk_uri._serial_number);
    else if(_partition_style == disk_uri._partition_style){
        if (_partition_style == macho::windows::storage::ST_PST_MBR)
            return _mbr_signature == disk_uri._mbr_signature;
        else if (_partition_style == macho::windows::storage::ST_PST_GPT)
            return _gpt_guid == disk_uri._gpt_guid;
        else if (!_address.empty() && !disk_uri._address.empty()){
            return _address == disk_uri._address;
        }
    }
    return false;
}

bool disk_universal_identify::operator != (const disk_universal_identify& disk_uri){
    return !(operator ==(disk_uri));
}

disk_universal_identify::operator std::string(){
    mObject uri;
    uri["serial_number"] = _serial_number;
    uri["partition_style"] = (int)_partition_style;
    uri["mbr_signature"] = (int)_mbr_signature;
    uri["gpt_guid"] = (std::string)_gpt_guid;
    uri["cluster_id"] = (std::string)_cluster_id;
    uri["friendly_name"] = (std::string)_friendly_name;
    uri["address"] = (std::string)_address;
    uri["unique_id"] = (std::string)_unique_id;
    uri["unique_id_format"] = (int)_unique_id_format;
    return write(uri, json_spirit::raw_utf8);
}

void common_service_handler::get_host_detail(physical_machine_info& _return, const std::string& session_id, const machine_detail_filter::type filter){

    VALIDATE;
    try{
        macho::windows::com_init _com;
        get_host_general_info(_return);
        if (machine_detail_filter::FULL == filter){
            get_storage_info(_return);
            get_network_info(_return);
            get_cluster_info(_return);
            registry reg2(REGISTRY_CREATE);
            if (reg2.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (!reg2[_T("Client")].exists()){
                    if (macho::windows::environment::is_winpe())
                        reg2[_T("Client")] = (std::wstring)macho::guid_(GUID_NULL);
                    else
                        reg2[_T("Client")] = (std::wstring)macho::guid_::create();
                }
                if (!reg2[_T("Machine")].exists()){
                    std::string machine_id;
                    foreach(saasame::transport::disk_info d, _return.disk_infos){
                        if (d.is_system){
                            if (d.serial_number.length())
                                machine_id = d.manufacturer + d.serial_number;
                            break;
                        }
                    }
                    foreach(saasame::transport::network_info n, _return.network_infos){
                        machine_id.append(n.mac_address);
                        break;
                    }
                    if (!machine_id.length()){
                        reg2[_T("Machine")] = (std::wstring)macho::guid_::create();
                    }
                    else{
                        std::string md5_checksum = md5(machine_id);
                        std::string id = md5_checksum.substr(0, 8) + "-" + md5_checksum.substr(8, 4) + "-" + md5_checksum.substr(12, 4) + "-" + md5_checksum.substr(16, 4) + "-" + md5_checksum.substr(20, -1);
                        reg2[_T("Machine")] = (std::wstring)macho::guid_(id);
                    }
                }
                _return.client_id = stringutils::convert_unicode_to_utf8(reg2[_T("Client")].wstring());
                _return.machine_id = stringutils::convert_unicode_to_utf8(reg2[_T("Machine")].wstring());
            }
        }
        else{
            registry reg2(REGISTRY_CREATE);
            if (reg2.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                if (!reg2[_T("Client")].exists()){
                    if (macho::windows::environment::is_winpe())
                        reg2[_T("Client")] = (std::wstring)macho::guid_(GUID_NULL);
                    else
                        reg2[_T("Client")] = (std::wstring)macho::guid_::create();
                }
                if (!reg2[_T("Machine")].exists()){
                    macho::windows::storage::ptr stg = macho::windows::storage::get();
                    stg->rescan();
                    macho::windows::storage::disk::vtr disks = stg->get_disks();
                    std::wstring machine_id;
                    foreach(macho::windows::storage::disk::ptr &d, disks){
                        if (d->is_system() && d->serial_number().length()){
                            machine_id = d->manufacturer() + d->serial_number();
                            break;
                        }
                    }
                    if (!machine_id.length()){
                        macho::windows::network::adapter::vtr networks = macho::windows::network::get_network_adapters();
                        if (networks.size())
                            machine_id = networks[0]->mac_address();
                    }
                    if (!machine_id.length()){
                        reg2[_T("Machine")] = (std::wstring)macho::guid_::create();
                    }
                    else{
                        std::string md5_checksum = md5(macho::stringutils::convert_unicode_to_utf8(machine_id));
                        std::string id = md5_checksum.substr(0, 8) + "-" + md5_checksum.substr(8, 4) + "-" + md5_checksum.substr(12, 4) + "-" + md5_checksum.substr(16, 4) + "-" + md5_checksum.substr(20, -1);
                        reg2[_T("Machine")] = (std::wstring)macho::guid_(id);
                    }
                }
                _return.client_id = stringutils::convert_unicode_to_utf8(reg2[_T("Client")].wstring());
                _return.machine_id = stringutils::convert_unicode_to_utf8(reg2[_T("Machine")].wstring());
            }
        }
    } 
    catch (macho::exception_base &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
    }
    catch (const boost::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
}

service_info common_service_handler::get_service(int port){
    service_info _return;
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    boost::shared_ptr<TTransport> transport;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")){
        try
        {
            boost::shared_ptr<TSSLSocketFactory> factory;
            factory = boost::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
            factory->authenticate(false);
            factory->loadCertificate((p / "server.crt").string().c_str());
            factory->loadPrivateKey((p / "server.key").string().c_str());
            factory->loadTrustedCertificates((p / "server.crt").string().c_str());
            boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
            factory->access(accessManager);
            boost::shared_ptr<TSSLSocket> ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket("localhost", port));
            ssl_socket->setConnTimeout(1000);
            transport = boost::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
        }
    }
    else{
        boost::shared_ptr<TSocket> socket(new TSocket("localhost", port));
        socket->setConnTimeout(1000);
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport){
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            transport->open();
            client.ping(_return);
            transport->close();
        }
        catch (TException& tx) {
        }
    }
    return _return;
}

void common_service_handler::get_service_list(std::set<service_info> & _return, const std::string& session_id){
    service_info empty;
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    boost::shared_ptr<TTransport> transport;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        _return.insert(get_service(saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT));
    else
        _return.insert(get_service(saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));

    _return.insert(get_service(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT));
    _return.insert(get_service(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT));
    _return.erase(empty);
}

void common_service_handler::enumerate_disks(std::set<disk_info> & _return, const enumerate_disk_filter_style::type filter){

    VALIDATE;
    macho::windows::com_init com;
    FUN_TRACE;
    
    macho::windows::storage::ptr stg = macho::windows::storage::get();
    stg->rescan();
    macho::windows::storage::disk::vtr disks = stg->get_disks();

    foreach(macho::windows::storage::disk::ptr d, disks){
        if ((filter == saasame::transport::enumerate_disk_filter_style::UNINITIALIZED_DISK) && (d->partition_style() != saasame::transport::partition_style::PARTITION_UNKNOWN))
            continue;
        saasame::transport::disk_info _d;
        _d.boot_from_disk = d->boot_from_disk();
        _d.bus_type = (saasame::transport::bus_type::type)(int)d->bus_type();
        _d.cylinders = d->total_cylinders();
        _d.friendly_name = macho::stringutils::convert_unicode_to_utf8(d->friendly_name());
        std::wstring wzguid = d->guid();
        if (!wzguid.empty()) _d.guid = (std::string)macho::guid_(wzguid);
        _d.is_boot = d->is_boot();
        _d.is_clustered = d->is_clustered();
        _d.is_offline = d->is_offline();
        _d.is_readonly = d->is_read_only();
        _d.is_system = d->is_system();
        _d.location = macho::stringutils::convert_unicode_to_utf8(d->location());
        _d.manufacturer = macho::stringutils::convert_unicode_to_utf8(d->manufacturer());
        _d.model = macho::stringutils::convert_unicode_to_utf8(d->model());
        _d.number = d->number();
        _d.number_of_partitions = d->number_of_partitions();
        _d.offline_reason = d->offline_reason();
        _d.partition_style = (saasame::transport::partition_style::type)(int)d->partition_style();
        _d.path = macho::stringutils::convert_unicode_to_utf8(d->path());
        if (IsWindows8OrGreater()){
            _d.logical_sector_size = d->logical_sector_size();
            _d.physical_sector_size = d->physical_sector_size();
        }
        else{
            DWORD                               error = NO_ERROR;
            STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment = { 0 };
            error = DetectSectorSize(d->path().c_str(), &alignment);
            if (error){
                _d.logical_sector_size = d->logical_sector_size();
                _d.physical_sector_size = d->physical_sector_size();
            }
            else{
                _d.logical_sector_size = alignment.BytesPerLogicalSector;
                _d.physical_sector_size = alignment.BytesPerPhysicalSector;
            }
        }
        _d.sectors_per_track = d->sectors_per_track();
        _d.serial_number = macho::stringutils::convert_unicode_to_utf8(d->serial_number());
        _d.signature = d->signature();
        _d.size = d->size();
        _d.tracks_per_cylinder = d->tracks_per_cylinder();
        _d.scsi_bus = d->scsi_bus();
        _d.scsi_logical_unit = d->scsi_logical_unit();
        _d.scsi_port = d->scsi_port();
        _d.scsi_target_id = d->scsi_target_id();
        //Need to add code for following info
        _d.cluster_owner;
        _d.is_snapshot;
        _d.uri = disk_universal_identify(*d);
        _d.unique_id = macho::stringutils::convert_unicode_to_utf8(d->unique_id());
        _d.unique_id_format = d->unique_id_format();
        _return.insert(_d);
    }
}

void common_service_handler::get_host_general_info(physical_machine_info& _info){

    FUN_TRACE;
    //macho::windows::com_init com_;
    registry reg(REGISTRY_READONLY);
    wmi_services wmi;
    wmi.connect(L"CIMV2");
    wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");

    macho::windows::operating_system os = macho::windows::environment::get_os_version();
    _info.architecture = stringutils::convert_unicode_to_utf8(os.sz_cpu_architecture());
    _info.domain = stringutils::convert_unicode_to_utf8(macho::windows::environment::get_full_domain_name());

    if (reg.open(_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"))){
        if (reg[_T("DigitalProductId4")].exists()){
            PMS_DIGITAL_PRODUCTID4 pDigitalPid4 = (PMS_DIGITAL_PRODUCTID4)(LPBYTE)reg[_T("DigitalProductId4")];
            LOG(LOG_LEVEL_INFO, _T("License Info ( DigitalProductId4 ) : EditionType ( %s ). KeyType ( %s )"), pDigitalPid4->szEditionType, pDigitalPid4->szKeyType);
        }

        if (reg[_T("DigitalProductId")].exists()){
            PMS_DIGITAL_PRODUCTID pDigitalPid = (PMS_DIGITAL_PRODUCTID)(LPBYTE)reg[_T("DigitalProductId")];
#if _DEBUG
            stdstring licKey = decode_ms_key(pDigitalPid->bUnknown2, 0);
            LOG(LOG_LEVEL_INFO, _T("License Info : OS License Key ( %s )."), licKey.c_str());
#endif
            LOG(LOG_LEVEL_INFO, _T("License Info ( DigitalProductId ) : EditionId ( %s ). ProductId ( %s )"), stringutils::convert_ansi_to_unicode(std::string(pDigitalPid->szEditionId)).c_str(), stringutils::convert_ansi_to_unicode(std::string(pDigitalPid->szProductId)).c_str());
            _info.is_oem = stringutils::tolower(std::string(pDigitalPid->szProductId)).find("oem") != std::string::npos;
        }
        reg.close();
    }

    _info.client_name = stringutils::convert_unicode_to_utf8(macho::windows::environment::get_computer_name_ex(COMPUTER_NAME_FORMAT::ComputerNameDnsHostname));
    if (0 ==_info.client_name.length())
        _info.client_name = stringutils::convert_unicode_to_utf8(macho::windows::environment::get_computer_name());
    _info.manufacturer = stringutils::convert_unicode_to_utf8(computer_system[L"Manufacturer"]);
    _info.system_model = stringutils::convert_unicode_to_utf8(computer_system[L"Model"]);;
    _info.os_name = stringutils::convert_unicode_to_utf8(os.name);
    _info.os_type = os.type;
    _info.os_version.csd_version = stringutils::convert_unicode_to_utf8(os.version_info.szCSDVersion);
    _info.os_version.major_version = os.version_info.dwMajorVersion;
    _info.os_version.minor_version = os.version_info.dwMinorVersion;
    _info.os_version.build_number = os.version_info.dwBuildNumber;
    _info.os_version.platform_id = os.version_info.dwPlatformId;
    _info.os_version.product_type = os.version_info.wProductType;
    _info.os_version.servicepack_major = os.version_info.wServicePackMajor;
    _info.os_version.servicepack_minor = os.version_info.wServicePackMinor;
    _info.os_version.suite_mask = os.version_info.wSuiteMask;
    _info.__set_os_version(_info.os_version);
    
    wmi_object_table physical_memorys = wmi.query_wmi_objects(L"Win32_PhysicalMemory");
    if (physical_memorys.size()){
        int64_t total_physical_mamory = 0;
        foreach(wmi_object physical_memory, physical_memorys)
            total_physical_mamory += (int64_t)physical_memory[L"Capacity"];
        _info.physical_memory = total_physical_mamory / (1024 * 1024);
    }
    else{
        _info.physical_memory = computer_system[L"TotalPhysicalMemory"];
    }
    _info.processors = (int16_t)(int)computer_system[L"NumberOfProcessors"];

    try{
        _info.logical_processors = (int16_t)(int)computer_system[L"NumberOfLogicalProcessors"];
    }
    catch (...){
    }
    if (reg.open(_T("SYSTEM\\CurrentControlSet\\Enum\\Root\\PCI_HAL\\0000"), HKEY_LOCAL_MACHINE)){
        if (reg[_T("HardwareID")].exists() && reg[_T("HardwareID")].get_multi_count())
            _info.hal = stringutils::convert_unicode_to_utf8(reg[_T("HardwareID")].get_multi_at(0));
        reg.close();
    }
    if (!_info.system_model.length() && reg.open(_T("SYSTEM\\CurrentControlSet\\Enum\\Root\\ACPI_HAL\\0000"), HKEY_LOCAL_MACHINE)){
        if (reg[_T("HardwareID")].exists() && reg[_T("HardwareID")].get_multi_count())
            _info.hal = stringutils::convert_unicode_to_utf8(reg[_T("HardwareID")].get_multi_at(0));
        reg.close();
    }
    _info.system_root = stringutils::convert_unicode_to_utf8(macho::windows::environment::get_system_directory());
    _info.workgroup = stringutils::convert_unicode_to_utf8(macho::windows::environment::get_workgroup_name());
    _info.os_system_info;// = os.system_info;
    try{
        macho::windows::service iscsi_service = macho::windows::service::get_service(L"MSiSCSI");
        iscsi_service.start();
        TCHAR initiator_name[MAX_ISCSI_NAME_LEN + 1] = { 0 };
        if (ERROR_SUCCESS == _GetIScsiInitiatorNodeName(initiator_name))
            _info.initiator_name = stringutils::convert_unicode_to_utf8(initiator_name);
    }
    catch (...){
    }
    string_array roles = computer_system[L"Roles"];
    _info.role;
    check_vcbt_driver_status(_info.is_vcbt_driver_installed, _info.is_vcbt_enabled);
    _info.__set_is_vcbt_driver_installed(_info.is_vcbt_driver_installed);
    _info.__set_is_vcbt_enabled(_info.is_vcbt_enabled);

    if (_info.is_vcbt_driver_installed){
        try{
            macho::windows::file_version_info fs = macho::windows::file_version_info::get_file_version_info(macho::windows::environment::get_system_directory() + L"\\drivers\\vcbt.sys");
            _info.installed_vcbt_version = boost::str(boost::format("%d.%d.%d.0") %fs.file_version_major() %fs.file_version_minor() %fs.file_version_build());
            _info.__set_installed_vcbt_version(_info.installed_vcbt_version);
        }
        catch (...){
        }
        if (_info.is_vcbt_enabled){
            macho::windows::registry reg(REGISTRY_READONLY_WOW64_64KEY);
            if (reg.open(L"SYSTEM\\CurrentControlSet\\Services\\vcbt")){
                _info.current_vcbt_version = reg[L"Version"].exists() ? reg[L"Version"].string() : "";
                _info.__set_current_vcbt_version(_info.current_vcbt_version);
            }
        }
    }
    _info.is_winpe = macho::windows::environment::is_winpe();
    _info.__set_is_winpe(_info.is_winpe);
}

void common_service_handler::get_storage_info(physical_machine_info& _info){
    FUN_TRACE;
    //macho::windows::com_init _com;
    macho::windows::storage::ptr stg = macho::windows::storage::get();
    stg->rescan();
    bool is_winpe = macho::windows::environment::is_winpe();

    macho::windows::storage::partition::vtr partitions;
    macho::windows::storage::volume::vtr volumes;
    macho::windows::storage::disk::vtr disks = stg->get_disks();
    if (is_winpe){
        foreach(macho::windows::storage::disk::ptr d, disks){
            if (is_winpe && ( d->bus_type() == saasame::transport::bus_type::USB || d->is_boot() ) ){
                continue;
            }           
            macho::windows::storage::partition::vtr parts = stg->get_partitions(d->number());
            partitions.insert(partitions.end(), parts.begin(), parts.end());
            macho::windows::storage::volume::vtr vols = stg->get_volumes(d->number());
            volumes.insert(volumes.end(), vols.begin(), vols.end());
        }
    }
    else{
        partitions = stg->get_partitions();
        volumes = stg->get_volumes();
    }
    std::set<disk_info>  disk_infos;
    std::set<partition_info>  partition_infos;
    std::set<volume_info>  volume_infos;
    std::map<int, macho::windows::storage::disk::ptr> excluded_disks;
    foreach(macho::windows::storage::disk::ptr d, disks){
        if (is_winpe && (d->bus_type() == saasame::transport::bus_type::USB || d->is_boot())){
            excluded_disks[d->number()] = d;
            continue;
        }
        disk_info _d;
        _d.boot_from_disk = d->boot_from_disk();
        _d.bus_type = (bus_type::type)(int)d->bus_type();
        _d.cylinders = d->total_cylinders();
        _d.friendly_name = macho::stringutils::convert_unicode_to_utf8(d->friendly_name());
        std::wstring wzguid = d->guid();
        if (!wzguid.empty()) _d.guid = (std::string)macho::guid_(wzguid);
        _d.is_boot = d->is_boot();
        _d.is_clustered = d->is_clustered();
        _d.is_offline = d->is_offline();
        _d.is_readonly = d->is_read_only();
        _d.is_system = d->is_system();
        _d.location = macho::stringutils::convert_unicode_to_utf8(d->location());
        _d.manufacturer = macho::stringutils::convert_unicode_to_utf8(d->manufacturer());
        _d.model = macho::stringutils::convert_unicode_to_utf8(d->model());
        _d.number = d->number();
        _d.number_of_partitions = d->number_of_partitions();
        _d.offline_reason = d->offline_reason();
        _d.partition_style = (partition_style::type)(int)d->partition_style();
        _d.path = macho::stringutils::convert_unicode_to_utf8(d->path());
        if (IsWindows8OrGreater()){
            _d.logical_sector_size = d->logical_sector_size();
            _d.physical_sector_size = d->physical_sector_size();
        }
        else{
            DWORD                               error = NO_ERROR;
            STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR alignment = { 0 };
            error = DetectSectorSize(d->path().c_str(), &alignment);
            if (error){
                _d.logical_sector_size = d->logical_sector_size();
                _d.physical_sector_size = d->physical_sector_size();
            }
            else{
                _d.logical_sector_size = alignment.BytesPerLogicalSector;
                _d.physical_sector_size = alignment.BytesPerPhysicalSector;
            }
        }
        _d.sectors_per_track = d->sectors_per_track();
        _d.serial_number = macho::stringutils::convert_unicode_to_utf8(d->serial_number());
        _d.signature = d->signature();
        _d.size = d->size();
        _d.tracks_per_cylinder = d->tracks_per_cylinder();
        _d.scsi_bus = d->scsi_bus();
        _d.scsi_logical_unit = d->scsi_logical_unit();
        _d.scsi_port = d->scsi_port();
        _d.scsi_target_id = d->scsi_target_id();
        //Need to add code for following info
        _d.cluster_owner;
        _d.is_snapshot;
        _d.uri = disk_universal_identify(*d);
        _d.unique_id = macho::stringutils::convert_unicode_to_utf8(d->unique_id());
        _d.unique_id_format = d->unique_id_format();
        disk_infos.insert(_d);
    }

    _info.__set_disk_infos(disk_infos);

    foreach(macho::windows::storage::partition::ptr p, partitions){
        if (excluded_disks.count(p->disk_number()))
            continue;
        partition_info _p;
        string_array_w access_paths = p->access_paths();
        std::set<std::string> _access_paths;
        foreach(std::wstring path, access_paths){
            _access_paths.insert(macho::stringutils::convert_unicode_to_utf8(path));
        }
        _p.__set_access_paths(_access_paths);
        _p.disk_number = p->disk_number();
        _p.drive_letter = macho::stringutils::convert_unicode_to_utf8(p->drive_letter());
        _p.gpt_type = macho::stringutils::convert_unicode_to_utf8(p->gpt_type());
        _p.guid = macho::stringutils::convert_unicode_to_utf8(p->guid());
        _p.is_active = p->is_active();
        _p.is_boot = p->is_boot();
        _p.is_hidden = p->is_hidden();
        _p.is_offline = p->is_offline();
        _p.is_readonly = p->is_read_only();
        _p.is_shadowcopy = p->is_shadow_copy();
        _p.is_system = p->is_system();
        _p.mbr_type = p->mbr_type();
        _p.offset = p->offset();
        _p.partition_number = p->partition_number();
        _p.size = p->size();

        partition_infos.insert(_p);
    }

    _info.__set_partition_infos(partition_infos);

    foreach(macho::windows::storage::volume::ptr v, volumes){
        volume_info _v;
        string_array_w access_paths = v->access_paths();
        std::set<std::string> _access_paths;
        foreach(std::wstring path, access_paths){
            _access_paths.insert(macho::stringutils::convert_unicode_to_utf8(path));
        }
        _v.__set_access_paths(_access_paths);
        _v.drive_letter = macho::stringutils::convert_unicode_to_utf8(v->drive_letter());
        _v.drive_type = (drive_type::type)(int)v->drive_type();
        _v.file_system = macho::stringutils::convert_unicode_to_utf8(v->file_system());
        _v.file_system_label = macho::stringutils::convert_unicode_to_utf8(v->file_system_label());
        _v.object_id = macho::stringutils::convert_unicode_to_utf8(v->id());
        _v.path = macho::stringutils::convert_unicode_to_utf8(v->path());
        _v.size = v->size();
        _v.size_remaining = v->size_remaining();

        // Need to add code here.
        _v.file_system_catalogid;
        _v.cluster_access_path;

        volume_infos.insert(_v);
    }
    _info.__set_volume_infos(volume_infos);
}

void common_service_handler::get_network_info(physical_machine_info& _info){
    FUN_TRACE;
    std::set<network_info>  network_infos;
    if (IsWindowsVistaOrGreater()){
        macho::windows::network::adapter::vtr networks = macho::windows::network::get_network_adapters();
        foreach(macho::windows::network::adapter::ptr net, networks){
            if (net->mac_address().length()){
                network_info _net;
                macho::windows::network::adapter_config::ptr setting = net->get_setting();
                _net.adapter_name = macho::stringutils::convert_unicode_to_utf8(net->name());
                _net.description = macho::stringutils::convert_unicode_to_utf8(net->description());
                if (!setting){
                    LOG(LOG_LEVEL_WARNING, _T("Cannot get network setting of '%s' ( %s )."), net->name().c_str(), net->description().c_str());
                    _net.mac_address = macho::stringutils::convert_unicode_to_utf8(net->mac_address());
                }
                else{
                    string_array_w            dnss = setting->dns_server_search_order();
                    std::vector<std::string>  _dnss;
                    foreach(std::wstring dns, dnss){
                        _dnss.push_back(macho::stringutils::convert_unicode_to_utf8(dns));
                    }
                    _net.__set_dnss(_dnss);
                    string_array_w            ipgetways = setting->default_ip_gateway();
                    std::vector<std::string>  _gateways;
                    foreach(std::wstring getway, ipgetways){
                        _gateways.push_back(macho::stringutils::convert_unicode_to_utf8(getway));
                    }
                    _net.__set_gateways(_gateways);
                    string_array_w            ipaddress = setting->ip_address();
                    std::vector<std::string>  _ip_addresses;
                    foreach(std::wstring ipaddr, ipaddress){
                        _ip_addresses.push_back(macho::stringutils::convert_unicode_to_utf8(ipaddr));
                    }
                    _net.__set_ip_addresses(_ip_addresses);
                    string_array_w ipsubnet = setting->ip_subnet();
                    std::vector<std::string>  _ipsubnet;
                    foreach(std::wstring subnet, ipsubnet){
                        _ipsubnet.push_back(macho::stringutils::convert_unicode_to_utf8(subnet));
                    }
                    _net.__set_subnet_masks(_ipsubnet);
                    _net.is_dhcp_v4 = setting->dhcp_enabled();
                    _net.mac_address = macho::stringutils::convert_unicode_to_utf8(setting->mac_address());

                    // need to check the registry to get this setting.
                    _net.is_dhcp_v6;
                }
                std::wstring   guid = net->guid();
                network_infos.insert(_net);
            }
        }
    }
    else{
        std::vector<network_info> nets;
        get_network_adapters(nets);
        get_network_dnss(nets);
        foreach(network_info &_net, nets){
            _net.__set_dnss(_net.dnss);
            _net.__set_gateways(_net.gateways);
            _net.__set_ip_addresses(_net.ip_addresses);
            _net.__set_subnet_masks(_net.subnet_masks);
            network_infos.insert(_net);
        }
    }
    _info.__set_network_infos(network_infos);
}

void common_service_handler::get_cluster_info(physical_machine_info& _info){
    FUN_TRACE;
    //macho::windows::com_init _com;
    cluster_info _cluster;
    //macho::windows::cluster::ptr c = macho::windows::cluster::get(L"192.168.200.58", L"test.falc", L"administrator", L"abc@123");
    macho::windows::cluster::ptr c = macho::windows::cluster::get();
    if (c){
        std::set<cluster_info>  cluster_infos;
        if (IsWindows8OrGreater()){
            _cluster.cluster_name = macho::stringutils::convert_unicode_to_utf8(c->fqdn());
        }
        else{
            std::wstring cluster_fqdn = boost::str(boost::wformat(L"%s.%s") % environment::get_full_domain_name() % c->name());
            _cluster.cluster_name = macho::stringutils::convert_unicode_to_utf8(cluster_fqdn);
        }

        macho::windows::cluster::node::vtr nodes = c->get_nodes();
        std::set<std::string>    _cluster_nodes;
        std::set<cluster_group>  _cluster_groups;
        foreach(macho::windows::cluster::node::ptr n, nodes){
            _cluster_nodes.insert(macho::stringutils::convert_unicode_to_utf8(n->name()));
            macho::windows::cluster::resource_group::vtr groups = n->get_active_groups();
            foreach(macho::windows::cluster::resource_group::ptr g, groups){
                cluster_group _g;
                std::set<disk_info>  _g_cluster_disks;
                std::set<volume_info>  _g_cluster_partitions;
                if (IsWindows8OrGreater()) {
                    _g.group_id = macho::stringutils::convert_unicode_to_utf8(g->id());
                }
                _g.group_owner = macho::stringutils::convert_unicode_to_utf8(n->name());
                _g.group_name = macho::stringutils::convert_unicode_to_utf8(g->name());

                macho::windows::cluster::resource::vtr resources = g->get_resources();
                foreach(macho::windows::cluster::resource::ptr r, resources){
                    macho::windows::cluster::disk::vtr disks = r->get_disks();
                    std::set<disk_info>  _cluster_disks;
                    std::set<volume_info>  _cluster_partitions;
                    foreach(macho::windows::cluster::disk::ptr d, disks){
                        disk_info _disk;
                        _disk.is_clustered = true;
                        _disk.cluster_owner = macho::stringutils::convert_unicode_to_utf8(n->name());
                        _disk.friendly_name = macho::stringutils::convert_unicode_to_utf8(d->name());
                        _disk.uri = disk_universal_identify(*d);;
                        try{
                            _disk.guid = macho::stringutils::convert_unicode_to_utf8(d->gpt_guid());
                        }
                        catch (...){}
                        try{
                            _disk.number = d->number();
                            _disk.size = d->size();
                        }
                        catch (...){}
                        try{
                            _disk.signature = d->signature();
                        }
                        catch (...){}
                        _cluster_disks.insert(_disk);
                    }
                    std::set_union(_g_cluster_disks.begin(), _g_cluster_disks.end(), _cluster_disks.begin(), _cluster_disks.end(), std::inserter(_g_cluster_disks, _g_cluster_disks.begin()));
                    _g.__set_cluster_disks(_g_cluster_disks);
                    macho::windows::cluster::disk_partition::vtr partitions = r->get_disk_partitions();
                    foreach(macho::windows::cluster::disk_partition::ptr p, partitions){
                        volume_info _v;
                        _v.cluster_access_path = macho::stringutils::convert_unicode_to_utf8(p->path());
                        string_array_w mount_points = p->mount_points();
                        foreach(std::wstring m, mount_points)
                            _v.access_paths.insert(macho::stringutils::convert_unicode_to_utf8(m));
                        _v.file_system = macho::stringutils::convert_unicode_to_utf8(p->file_system());
                        _v.file_system_label = macho::stringutils::convert_unicode_to_utf8(p->volume_label());
                        _v.object_id = macho::stringutils::convert_unicode_to_utf8(p->volume_guid());
                        _v.size = p->total_size();
                        _v.size_remaining = p->free_space();
                        _cluster_partitions.insert(_v);
                    }
                    std::set_union(_g_cluster_partitions.begin(), _g_cluster_partitions.end(), _cluster_partitions.begin(), _cluster_partitions.end(), std::inserter(_g_cluster_partitions, _g_cluster_partitions.begin()));
                    _g.__set_cluster_partitions(_g_cluster_partitions);
                }
                _cluster_groups.insert(_g);
            }
        }
        _cluster.__set_cluster_groups(_cluster_groups);
        _cluster.__set_cluster_nodes(_cluster_nodes);
        macho::windows::cluster::resource::ptr quorum_resource = c->get_quorum_resource();
        if (quorum_resource){
            macho::windows::cluster::disk::vtr quorum_disks = quorum_resource->get_disks();
            foreach(macho::windows::cluster::disk::ptr quorum_disk, quorum_disks){
                disk_info _disk;
                if (IsWindows8OrGreater())
                    _disk.cluster_owner = macho::stringutils::convert_unicode_to_utf8(quorum_disk->get_resource()->get_resource_group()->owner_node());
                _disk.friendly_name = macho::stringutils::convert_unicode_to_utf8(quorum_disk->name());
                _disk.uri = disk_universal_identify(*quorum_disk);
                try{
                    _disk.guid = macho::stringutils::convert_unicode_to_utf8(quorum_disk->gpt_guid());
                }
                catch (...){}
                _disk.is_clustered = true;
                try{
                    _disk.number = quorum_disk->number();
                    _disk.size = quorum_disk->size();
                }
                catch (...){}
                try{
                    _disk.signature = quorum_disk->signature();
                }
                catch (...){}
                _cluster.quorum_disk = _disk;
                _cluster.__set_quorum_disk(_cluster.quorum_disk);
            }
        }

        c->get_network_interfaces();

        macho::windows::cluster::network::vtr networks = c->get_networks();
        std::set<cluster_network>  _cluster_network_infos;
        foreach(macho::windows::cluster::network::ptr net, networks){
            cluster_network _net;
            _net.cluster_network_name = macho::stringutils::convert_unicode_to_utf8(net->name());
            try{
                _net.cluster_network_id = macho::stringutils::convert_unicode_to_utf8(net->id());
            }
            catch (...){}
            _net.cluster_network_address = macho::stringutils::convert_unicode_to_utf8(net->address());
            _net.cluster_network_address_mask = macho::stringutils::convert_unicode_to_utf8(net->address_mask());
            macho::windows::cluster::network_interface::vtr interfaces = net->get_network_interfaces();
            std::set<network_info>  _network_infos;
            foreach(macho::windows::cluster::network_interface::ptr interface_, interfaces){
                network_info _net_interface;
                _net_interface.adapter_name = macho::stringutils::convert_unicode_to_utf8(interface_->adapter());
                _net_interface.description = macho::stringutils::convert_unicode_to_utf8(interface_->description());
                string_array_w ipv4_addresses = interface_->ipv4_addresses();
                std::vector<std::string>  _ip_addresses;
                foreach(std::wstring ipaddr, ipv4_addresses){
                    if (ipaddr.length()) _ip_addresses.push_back(macho::stringutils::convert_unicode_to_utf8(ipaddr));
                }
                string_array_w ipv6_address = interface_->ipv6_addresses();
                foreach(std::wstring ipaddr, ipv6_address){
                    if (ipaddr.length()) _ip_addresses.push_back(macho::stringutils::convert_unicode_to_utf8(ipaddr));
                }
                _net_interface.is_dhcp_v4 = interface_->dhcp_enabled();
                _net_interface.__set_ip_addresses(_ip_addresses);
                _network_infos.insert(_net_interface);
            }
            _net.__set_network_infos(_network_infos);
            _cluster_network_infos.insert(_net);
        }
        _cluster.__set_cluster_network_infos(_cluster_network_infos);
        cluster_infos.insert(_cluster);
        _info.__set_cluster_infos(cluster_infos);
    }
}

stdstring decode_ms_key(const LPBYTE lpBuf, const DWORD rpkOffset)
{
    INT32 i, j;
    DWORD  dwAccumulator;
    TCHAR  szPossibleChars[] = _T("BCDFGHJKMPQRTVWXY2346789");
    stdstring szMSKey;
    i = 28;
    szMSKey.resize(i + 1);

    do
    {
        dwAccumulator = 0;
        j = 14;

        do
        {
#if 0
            dwAccumulator = dwAccumulator << 8;
            dwAccumulator = lpBuf[j + rpkOffset] + dwAccumulator;
            lpBuf[j + rpkOffset] = (dwAccumulator / 24) & 0xFF;
#else            
            dwAccumulator = (dwAccumulator << 8) ^ lpBuf[j + rpkOffset];
            lpBuf[j + rpkOffset] = (BYTE)(dwAccumulator / 24);
#endif
            dwAccumulator = dwAccumulator % 24;
            j--;
        } while (j >= 0);

        szMSKey[i] = szPossibleChars[dwAccumulator];
        i--;

        if ((((29 - i) % 6) == 0) && (i != -1))
        {
            szMSKey[i] = _T('-');
            i--;
        }

    } while (i >= 0);

    return szMSKey;
}

HRESULT _GetIScsiInitiatorNodeName(TCHAR* InitiatorNodeName){

    if (NULL == pfnGetIScsiInitiatorNodeName){
        HMODULE                     hIscsi = LoadLibraryEx(szIscsiLib, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hIscsi != NULL){
#if defined( UNICODE ) || defined( _UNICODE )
            pfnGetIScsiInitiatorNodeName = (fnGetIScsiInitiatorNodeName)GetProcAddress(hIscsi, "GetIScsiInitiatorNodeNameW");
#else
            pfnGetIScsiInitiatorNodeName = (fnGetIScsiInitiatorNodeName)GetProcAddress(m_hIscsi, "GetIScsiInitiatorNodeNameA");
#endif
        }
    }
    if (pfnGetIScsiInitiatorNodeName)
        return pfnGetIScsiInitiatorNodeName(InitiatorNodeName);
    return HRESULT_FROM_WIN32(GetLastError());
}

DWORD DetectSectorSize(LPCWSTR devName, PSTORAGE_ACCESS_ALIGNMENT_DESCRIPTOR pAlignmentDescriptor)
{
    DWORD                  Bytes = 0;
    BOOL                   bReturn = FALSE;
    DWORD                  Error = NO_ERROR;
    STORAGE_PROPERTY_QUERY Query;

    ZeroMemory(&Query, sizeof(Query));

    HANDLE  hFile = CreateFileW(devName,
        STANDARD_RIGHTS_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        LOG(LOG_LEVEL_ERROR, L"hFile==INVALID_HANDLE_VALUE. GetLastError() returns %lu.", Error = GetLastError());
        return Error;
    }

    Query.QueryType = PropertyStandardQuery;
    Query.PropertyId = StorageAccessAlignmentProperty;

    bReturn = DeviceIoControl(hFile,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &Query,
        sizeof(STORAGE_PROPERTY_QUERY),
        pAlignmentDescriptor,
        sizeof(STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR),
        &Bytes,
        NULL);

    if (bReturn == FALSE) {
        LOG(LOG_LEVEL_ERROR, L"bReturn==FALSE. GetLastError() returns %lu.", Error = GetLastError());
    }

    CloseHandle(hFile);
    return Error;
}

void common_service_handler::check_vcbt_driver_status(bool& installed, bool& enabled){

    macho::windows::registry reg(REGISTRY_READONLY_WOW64_64KEY);
    if (reg.open(L"SYSTEM\\CurrentControlSet\\Services\\vcbt")){
        installed = reg[L"ImagePath"].exists() && reg[L"Start"].exists() && ((DWORD)reg[L"Start"] == 0);
    }

    macho::windows::auto_handle hDevice = CreateFileW(VCBT_WIN32_DEVICE_NAME,          // drive to open
        0,                // no access to the drive
        FILE_SHARE_READ | // share mode
        FILE_SHARE_WRITE,
        NULL,             // default security attributes
        OPEN_EXISTING,    // disposition
        0,                // file attributes
        NULL);            // do not copy file attributes      
    enabled = hDevice.is_valid();
}

std::string to_address_string(SOCKET_ADDRESS &sock_addr){
    WCHAR   op[256];
    DWORD  size = 256;
    long   errorcode = WSAAddressToStringW(sock_addr.lpSockaddr, sock_addr.iSockaddrLength, NULL, op, &size);
    if (errorcode == 0)
        return macho::stringutils::convert_unicode_to_utf8(op);
    LOG(LOG_LEVEL_ERROR, "WSAAddressToStringW : error(%i)", errorcode);
    return "";
}

void get_network_dnss(std::vector<network_info> &network_infos)
{
    typedef std::map<std::string, std::vector<std::string> > string_array_map_type;
    string_array_map_type dnssmap;
    /* Declare and initialize variables */
    DWORD dwSize = 0;
    DWORD dwRetVal = 0;

    unsigned int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to unspecified address family (both)
    ULONG family = AF_INET;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;
    ULONG Iterations = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;

    // Allocate a 15 KB buffer to start with.
    outBufLen = WORKING_BUFFER_SIZE;

    do {
        pAddresses = (IP_ADAPTER_ADDRESSES *)MALLOC(outBufLen);
        if (pAddresses == NULL) {
            LOG(LOG_LEVEL_ERROR, L"Memory allocation failed for IP_ADAPTER_ADDRESSES struct");
            dwRetVal = ERROR_INSUFFICIENT_BUFFER;
            break;
        }
        dwRetVal = GetAdaptersAddresses(family, flags, NULL, pAddresses, &outBufLen);
        if (dwRetVal == ERROR_BUFFER_OVERFLOW) {
            FREE(pAddresses);
            pAddresses = NULL;
        }
        else {
            break;
        }
        Iterations++;
    } while ((dwRetVal == ERROR_BUFFER_OVERFLOW) && (Iterations < MAX_TRIES));

    if (dwRetVal == NO_ERROR) {
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses) {
            if ((pCurrAddresses->OperStatus & IfOperStatusUp) && (pCurrAddresses->IfType == IF_TYPE_ETHERNET_CSMACD || pCurrAddresses->IfType == IF_TYPE_IEEE80211)){
                pDnServer = pCurrAddresses->FirstDnsServerAddress;
                if (pDnServer) {
                    std::vector<std::string> dnss;
                    for (i = 0; pDnServer != NULL; i++){
                        dnss.push_back(to_address_string(pDnServer->Address));
                        pDnServer = pDnServer->Next;
                    }
                    dnssmap[pCurrAddresses->AdapterName] = dnss;
                }
            }
            pCurrAddresses = pCurrAddresses->Next;
        }
    }
    if (pAddresses) {
        FREE(pAddresses);
    }

    foreach(network_info &_net, network_infos){
        if (dnssmap.count(_net.adapter_name))
            _net.dnss = dnssmap[_net.adapter_name];
    }

    return;
}

int get_network_adapters(std::vector<network_info> &network_infos)
{
    /* Declare and initialize variables */

    // It is possible for an adapter to have multiple
    // IPv4 addresses, gateways, and secondary WINS servers
    // assigned to the adapter. 
    //
    // Note that this sample code only prints out the 
    // first entry for the IP address/mask, and gateway, and
    // the primary and secondary WINS server for each adapter. 

    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter = NULL;
    DWORD dwRetVal = 0;
    UINT i;

    /* variables used to print DHCP time info */

    ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
    pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
    if (pAdapterInfo == NULL) {
        LOG(LOG_LEVEL_ERROR, L"Error allocating memory needed to call GetAdaptersinfo");
        return 1;
    }
    memset(pAdapterInfo, 0, ulOutBufLen);
    // Make an initial call to GetAdaptersInfo to get
    // the necessary size into the ulOutBufLen variable
    if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
        FREE(pAdapterInfo);
        pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
        if (pAdapterInfo == NULL) {
            LOG(LOG_LEVEL_ERROR, L"Error allocating memory needed to call GetAdaptersinfo");
            return 1;
        }
        memset(pAdapterInfo, 0, ulOutBufLen);
    }

    if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR) {
        pAdapter = pAdapterInfo;
        while (pAdapter) {
            if (pAdapter->Type == MIB_IF_TYPE_ETHERNET || pAdapter->Type == 71){
                network_info _net;
                _net.adapter_name = pAdapter->AdapterName;
                _net.description = pAdapter->Description;
                _net.mac_address;

                for (i = 0; i < pAdapter->AddressLength; i++) {
                    if (i == (pAdapter->AddressLength - 1))
                        _net.mac_address.append(boost::str(boost::format("%02X") % (int)pAdapter->Address[i]));
                    else
                        _net.mac_address.append(boost::str(boost::format("%02X-") % (int)pAdapter->Address[i]));
                }

                PIP_ADDR_STRING address = &pAdapter->IpAddressList;
                while (address != NULL){
                    std::string addr = address->IpAddress.String;
                    if (addr.length() && addr != "0.0.0.0"){
                        _net.ip_addresses.push_back(addr);
                        _net.subnet_masks.push_back(address->IpMask.String);
                    }
                    address = address->Next;
                }

                address = &pAdapter->GatewayList;
                while (address != NULL){
                    std::string addr = address->IpAddress.String;
                    if (addr.length() && addr != "0.0.0.0"){
                        _net.gateways.push_back(addr);
                    }
                    address = address->Next;
                }
                _net.is_dhcp_v4 = pAdapter->DhcpEnabled == TRUE;
                network_infos.push_back(_net);
            }
            pAdapter = pAdapter->Next;
        }
    }
    else {
        LOG(LOG_LEVEL_ERROR, L"GetAdaptersInfo failed with error: %d", dwRetVal);
        if (pAdapterInfo)
            FREE(pAdapterInfo);
        return 1;
    }
    if (pAdapterInfo)
        FREE(pAdapterInfo);
    return 0;
}

bool common_service_handler::verify_carrier(const std::string& carrier, const bool is_ssl){

    bool result = false;
    boost::shared_ptr<TSocket>                                    socket;
    boost::shared_ptr<TSSLSocket>                                 ssl_socket;
    boost::shared_ptr<TTransport>                                 transport;
    boost::shared_ptr<TProtocol>                                  protocol;
    boost::shared_ptr<TSSLSocketFactory>                          factory;
    boost::shared_ptr<saasame::transport::common_serviceClient>   client;

    if (is_ssl)
    {
        macho::windows::registry reg;
        boost::filesystem::path p(macho::windows::environment::get_working_directory());
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                p = reg[L"KeyPath"].wstring();
        }
        if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        {
            try
            {
                boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
                factory = boost::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                factory->authenticate(false);
                factory->loadCertificate((p / "server.crt").string().c_str());
                factory->loadPrivateKey((p / "server.key").string().c_str());
                factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                factory->access(accessManager);

                ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket(carrier, saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT));
                ssl_socket->setConnTimeout(30 * 1000);
                transport = boost::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
            }
            catch (TException& ex) {}
        }
    }
    else
    {
        socket = boost::shared_ptr<TSocket>(new TSocket(carrier, saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT));
        socket->setConnTimeout(30 * 1000);
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport){
        protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
        client = boost::shared_ptr<saasame::transport::common_serviceClient>(new saasame::transport::common_serviceClient(protocol));
        saasame::transport::service_info svc_info;
        try
        {
            transport->open();
            client->ping(svc_info);
            result = true;
            transport->close();
        }
        catch (TException& ex)
        {
            if (transport->isOpen())
                transport->close();
        }
    }
    return result;
}

void common_service_handler::take_xray(std::string& _return){
	bool has_data = false;
	boost::filesystem::path working_dir(macho::windows::environment::get_working_directory());
	std::stringstream data;	
    archive::zip::ptr zip_ptr = archive::zip::open(data);
    if (zip_ptr){
        if (zip_ptr->add(working_dir / ("connections")))
            has_data = true;
        if (zip_ptr->add(working_dir / ("logs")))
            has_data = true;
        if (zip_ptr->add(working_dir / ("jobs")))
            has_data = true;
        if (zip_ptr->close() && has_data)
            _return = data.str();
        LOG(LOG_LEVEL_INFO, _T("take_xray, working dir: %s, return data length:%d"), working_dir.string(), _return.length());
    }
}

void common_service_handler::take_xrays(std::string& _return){

	std::stringstream data;
    archive::zip::ptr zip_ptr = archive::zip::open(data);
    if (zip_ptr){
        bool     has_data = false;
        macho::windows::registry reg;
        boost::filesystem::path p(macho::windows::environment::get_working_directory());
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                p = reg[L"KeyPath"].wstring();
        }

        if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")){
            std::string carrier_data = get_xray(saasame::transport::g_saasame_constants.CARRIER_SERVICE_SSL_PORT);
            if (!carrier_data.empty())
            {
                has_data = true;
                zip_ptr->add("Carrier.zip", carrier_data);
            }
        }
        else{
            std::string carrier_data = get_xray(saasame::transport::g_saasame_constants.CARRIER_SERVICE_PORT);
            if (!carrier_data.empty())
            {
                has_data = true;
                zip_ptr->add("Carrier.zip", carrier_data);
            }
        }
        std::string launcher_data = get_xray(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT);
        if (!launcher_data.empty())
        {
            has_data = true;
            zip_ptr->add("Launcher.zip", launcher_data);
        }
        std::string loader_data = get_xray(saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT);
        if (!loader_data.empty())
        {
            has_data = true;
            zip_ptr->add("Loader.zip", loader_data);
        }
        std::string phypac_data = get_xray(saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT);
        if (!phypac_data.empty())
        {
            has_data = true;
            zip_ptr->add("PhysicalPacker.zip", phypac_data);
        }
        std::string scheduler_data = get_xray(saasame::transport::g_saasame_constants.SCHEDULER_SERVICE_PORT);
        if (!scheduler_data.empty())
        {
            has_data = true;
            zip_ptr->add("Scheduler.zip", scheduler_data);
        }
        std::string vpac_data = get_xray(saasame::transport::g_saasame_constants.VIRTUAL_PACKER_SERVICE_PORT);
        if (!vpac_data.empty())
        {
            has_data = true;
            zip_ptr->add("VirtualPacker.zip", vpac_data);
        }
        if (zip_ptr->close() && has_data)
            _return = data.str();
        LOG(LOG_LEVEL_INFO, _T("take_xrays , return data length:%d"), _return.length());
    }
}

std::string common_service_handler::get_xray(int port, const std::string& host){

	std::string _return;
	macho::windows::registry reg;
	boost::filesystem::path p(macho::windows::environment::get_working_directory());
	boost::shared_ptr<TTransport> transport;
	if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
		if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
			p = reg[L"KeyPath"].wstring();
	}
	if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt")){
		try
		{
			boost::shared_ptr<TSSLSocketFactory> factory;
			factory = boost::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
			factory->authenticate(false);
			factory->loadCertificate((p / "server.crt").string().c_str());
			factory->loadPrivateKey((p / "server.key").string().c_str());
			factory->loadTrustedCertificates((p / "server.crt").string().c_str());
			boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
			factory->access(accessManager);
			boost::shared_ptr<TSSLSocket> ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket(host, port));
			ssl_socket->setConnTimeout(1000);
			transport = boost::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
		}
		catch (TException& ex) {
		}
	}
	else{
		boost::shared_ptr<TSocket> socket(new TSocket(host, port));
		socket->setConnTimeout(1000);
		transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
	}
	if (transport){
		boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
		common_serviceClient client(protocol);
		try {
			transport->open();
			client.take_xray(_return);
			transport->close();
			LOG(LOG_LEVEL_INFO, _T("get_xray function(port:%d), return data length:%d"), port, _return.length());
		}
		catch (TException& tx) {
		}
	}
	return _return;
}
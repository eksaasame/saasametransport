
#include "stdafx.h"
#include "winpe_util.h"
#include "json_spirit.h"
#include "boost\filesystem.hpp"
#include <codecvt>
#include <VersionHelpers.h>
#include "common\ntp_client.hpp"

using namespace macho;
using namespace macho::windows;
using namespace json_spirit;

typedef std::map<std::wstring, winpe_network_setting > network_settings_map_type;

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

const uint64_t find_value_int64(const json_spirit::mObject& obj, const std::string& name, uint64_t default_value){
    mObject::const_iterator i = obj.find(name);
    if (i != obj.end()){
        return i->second.get_int64();
    }
    return default_value;
}

const mArray& find_value_array(const mObject& obj, const std::string& name){
    mObject::const_iterator i = obj.find(name);
    if (i != obj.end())
        return i->second.get_array();
    return mArray();
}

void winpe_settings::save(){
    FUN_TRACE;
    try{
        if (is_keep){
            boost::filesystem::path settings_file = std::wstring(macho::windows::environment::get_execution_full_path()) + L".cfg";
            mObject winpe_config;
            winpe_config["host_name"] = macho::stringutils::convert_unicode_to_utf8(host_name);
            winpe_config["client_id"] = macho::stringutils::convert_unicode_to_utf8(client_id);
            winpe_config["machine_id"] = macho::stringutils::convert_unicode_to_utf8(machine_id);
            winpe_config["digital_id"] = macho::stringutils::convert_unicode_to_utf8(digital_id);
            winpe_config["session_id"] = macho::stringutils::convert_unicode_to_utf8(session_id);
            winpe_config["mgmt_addr"] = macho::stringutils::convert_unicode_to_utf8(mgmt_addr);
            mArray settings;
            foreach(network_settings_map_type::value_type &v, network_settings){
                mObject setting;
                setting["mac_addr"] = macho::stringutils::convert_unicode_to_utf8(v.second.mac_addr);
                std::vector<std::string> tbIPAddress, tbSubnetMask, tbGateways, tbDNSServers;
                foreach(std::wstring s, v.second.tbIPAddress)
                    tbIPAddress.push_back(macho::stringutils::convert_unicode_to_utf8(s));
                foreach(std::wstring s, v.second.tbSubnetMask)
                    tbSubnetMask.push_back(macho::stringutils::convert_unicode_to_utf8(s));
                foreach(std::wstring s, v.second.tbGateways)
                    tbGateways.push_back(macho::stringutils::convert_unicode_to_utf8(s));
                foreach(std::wstring s, v.second.tbDNSServers)
                    tbDNSServers.push_back(macho::stringutils::convert_unicode_to_utf8(s));

                mArray  ip_addr(tbIPAddress.begin(), tbIPAddress.end());
                setting["ip_addr"] = ip_addr;
                mArray  subnet_mask(tbSubnetMask.begin(), tbSubnetMask.end());
                setting["subnet_mask"] = subnet_mask;
                mArray  gateways(tbGateways.begin(), tbGateways.end());
                setting["gateways"] = gateways;
                mArray  DNSs(tbDNSServers.begin(), tbDNSServers.end());
                setting["DNSs"] = DNSs;
                settings.push_back(setting);
            }
            winpe_config["network_settings"] = settings;
            boost::filesystem::path temp = settings_file.string() + ".tmp";
            {
                std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
                std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
                output.imbue(loc);
                write(winpe_config, output, json_spirit::pretty_print | json_spirit::raw_utf8);
            }
            if (!MoveFileEx(temp.wstring().c_str(), settings_file.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), settings_file.wstring().c_str(), GetLastError());
            }
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info.")));
    }
    catch (...){
    }
}

bool winpe_settings::load(){
    FUN_TRACE;
    bool result = false;
    try{
        boost::filesystem::path settings_file = std::wstring(macho::windows::environment::get_execution_full_path()) + L".cfg";
        if (boost::filesystem::exists(settings_file)){
            using namespace json_spirit;
            std::ifstream is(settings_file.wstring());
            mValue winpe_config;
            read(is, winpe_config);
            host_name = macho::stringutils::convert_utf8_to_unicode(find_value_string(winpe_config.get_obj(), "host_name"));
            client_id = macho::stringutils::convert_utf8_to_unicode( find_value_string(winpe_config.get_obj(), "client_id"));
            machine_id = macho::stringutils::convert_utf8_to_unicode(find_value_string(winpe_config.get_obj(), "machine_id"));
            digital_id = macho::stringutils::convert_utf8_to_unicode(find_value_string(winpe_config.get_obj(), "digital_id"));
            session_id = macho::stringutils::convert_utf8_to_unicode(find_value_string(winpe_config.get_obj(), "session_id"));
            mgmt_addr = macho::stringutils::convert_utf8_to_unicode(find_value_string(winpe_config.get_obj(), "mgmt_addr"));
            mArray settings = find_value(winpe_config.get_obj(), "network_settings").get_array();
            foreach(mValue n, settings){
                winpe_network_setting setting;
                setting.mac_addr = macho::stringutils::convert_utf8_to_unicode(find_value_string(n.get_obj(), "mac_addr"));

                std::vector<std::string> tbIPAddress, tbSubnetMask, tbGateways, tbDNSServers;
                mArray addr = find_value(n.get_obj(), "ip_addr").get_array();
                foreach(mValue a, addr){
                    tbIPAddress.push_back(a.get_str());
                }
                foreach(std::string& a, tbIPAddress){
                    setting.tbIPAddress.push_back(macho::stringutils::convert_utf8_to_unicode(a));
                }
                
                mArray subnet_mask = find_value(n.get_obj(), "subnet_mask").get_array();
                foreach(mValue a, subnet_mask){
                    tbSubnetMask.push_back(a.get_str());
                }
                foreach(std::string& a, tbSubnetMask){
                    setting.tbSubnetMask.push_back(macho::stringutils::convert_utf8_to_unicode(a));
                }

                mArray gateways = find_value(n.get_obj(), "gateways").get_array();
                foreach(mValue a, gateways){
                    tbGateways.push_back(a.get_str());
                }
                foreach(std::string& a, tbGateways){
                    setting.tbGateways.push_back(macho::stringutils::convert_utf8_to_unicode(a));
                }
                
                mArray DNSs = find_value(n.get_obj(), "DNSs").get_array();
                foreach(mValue a, DNSs){
                    tbDNSServers.push_back(a.get_str());
                }
                foreach(std::string& a, tbDNSServers){
                    setting.tbDNSServers.push_back(macho::stringutils::convert_utf8_to_unicode(a));
                }
                
                network_settings[setting.mac_addr] = setting;
            }
            result = true;
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot read config info.")));
    }
    catch (...){
    }
    return result;
}

void winpe_settings::apply(){
    FUN_TRACE;
    registry reg(REGISTRY_CREATE);
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (!host_name.empty())
            reg[_T("HostName")] = host_name;
        reg[_T("Client")] = client_id;
        reg[_T("Machine")] = machine_id;
        reg[_T("DigitalId")] = bytes(digital_id);
        if (!session_id.empty() && !mgmt_addr.empty()){
            reg[L"SessionId"] = session_id;
            reg[L"MgmtAddr"] = mgmt_addr;
            reg[L"AllowMultiple"] = (DWORD)0;
        }
    }
    macho::windows::com_init com;
    macho::windows::network::adapter::vtr adapters = macho::windows::network::get_network_adapters();
    foreach(macho::windows::network::adapter::ptr adapter, adapters){
        if (adapter->physical_adapter() && adapter->net_enabled() && network_settings.count(adapter->mac_address())){         
            macho::windows::network::adapter_config::ptr config = adapter->get_setting();         
            set_ip_settings_by_netsh_command(config->setting_id(),
                adapter->net_connection_id(), 
                network_settings[adapter->mac_address()].tbIPAddress, 
                network_settings[adapter->mac_address()].tbSubnetMask, 
                network_settings[adapter->mac_address()].tbDNSServers, 
                network_settings[adapter->mac_address()].tbGateways, 
                L"0", 
                std::vector<std::wstring>(), 
                true);
        }
    }
}

void winpe_settings::set_ip_settings_by_netsh_command(std::wstring settings_id, std::wstring connection, std::vector<std::wstring> tbIPAddress, std::vector<std::wstring> tbSubnetMask, std::vector<std::wstring> tbDNSServers, std::vector<std::wstring> tbGateways, std::wstring gatewaymetric, std::vector<std::wstring> tbWINSs, bool is_enable_dhcp){
    using namespace macho::windows;
    using namespace macho;
    std::wstring command;
    bool bIsWin2k8r2orLater = IsWindows7OrGreater();
    bool bIsVisterorLater = IsWindowsVistaOrGreater();
    std::wstring    szRegKeyPath = _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\") + settings_id;
    if (tbIPAddress.size() && tbSubnetMask.size()){
        if (tbIPAddress.size() != tbSubnetMask.size())
            return;
        for (size_t index = 0; index < tbIPAddress.size(); index++){
            if (index == 0)
                command = _T("cmd.exe /C netsh interface ip set address name=\"");
            else
                command = _T("cmd.exe /C netsh interface ip add address name=\"");
            command += connection;
            if (bIsVisterorLater){
                if (index == 0)
                    command += _T("\" source=static address=");
                else
                    command += _T("\" address=");
            }
            else {
                if (index == 0)
                    command += _T("\" source=static addr=");
                else
                    command += _T("\" addr=");
            }

            command += tbIPAddress[index];
            command += _T(" mask=");
            command += tbSubnetMask[index];
            macho::windows::process::exec_console_application_with_retry(command);

            {// Add the code here to fix the win2k8R2 and win7 set ipaddress issue on safemode.
                registry reg;
                BOOL      IsIPAddressEntryExiste = FALSE;
                reg.open(szRegKeyPath, HKEY_LOCAL_MACHINE);

                if (index == 0){
                    if (reg[_T("IPAddress")].exists())
                    {
                        reg[_T("IPAddress")].clear_multi();
                        reg[_T("SubnetMask")].clear_multi();
                    }
                    reg[_T("EnableDHCP")] = (DWORD)0x0;
                }
                else{
                    for (size_t iMultiCount = 0; iMultiCount < reg[_T("IPAddress")].get_multi_count(); iMultiCount++){
                        if (wcscmp(reg[_T("IPAddress")].get_multi_at(iMultiCount), tbIPAddress[index].c_str()) == 0){
                            IsIPAddressEntryExiste = TRUE;
                        }
                    }
                }

                if (!IsIPAddressEntryExiste){
                    reg[_T("IPAddress")].add_multi(tbIPAddress[index]);
                    reg[_T("SubnetMask")].add_multi(tbSubnetMask[index]);
                }
                reg.close();
            }
        }
    }
    else if (is_enable_dhcp){
        command = _T("cmd.exe /C netsh interface ip set address name=\"");
        command += connection;
        command += _T("\" source=dhcp");
        macho::windows::process::exec_console_application_with_retry(command);
    }

    if (tbGateways.size()){
        for (size_t index = 0; index < tbGateways.size(); index++){
            command = _T("cmd.exe /C netsh interface ip add address name=\"");
            command += connection;
            command += _T("\" gateway=");
            command += tbGateways[index];
            command += _T(" gwmetric=");
            command += gatewaymetric;
            macho::windows::process::exec_console_application_with_retry(command);

            {// Add the code here to fix the win2k8R2 and win7 set defaultgateway issue on safemode.
                registry reg;
                BOOL      IsGatewayEntryExiste = FALSE;
                reg.open(szRegKeyPath, HKEY_LOCAL_MACHINE);

                if (index == 0){
                    if (reg[_T("DefaultGateway")].exists()){
                        reg[_T("DefaultGateway")].clear_multi();
                        reg[_T("DefaultGatewayMetric")].clear_multi();
                    }
                }
                else{
                    for (size_t iMultiCount = 0; iMultiCount < reg[_T("DefaultGateway")].get_multi_count(); iMultiCount++){
                        if (wcscmp(reg[_T("DefaultGateway")].get_multi_at(iMultiCount), tbGateways[index].c_str()) == 0){
                            IsGatewayEntryExiste = TRUE;
                        }
                    }
                }

                if (!IsGatewayEntryExiste){
                    reg[_T("DefaultGateway")].add_multi(tbGateways[index]);
                    reg[_T("DefaultGatewayMetric")].add_multi(gatewaymetric);
                }
                reg.close();
            }
        }
    }
    else if (is_enable_dhcp){
        //command = _T("cmd.exe /C netsh interface ip delete address name=\"");
        //command += connection;
        //command += _T("\" gateway=all");
        //macho::windows::process::exec_console_application_with_retry(command);

        registry reg;
        if (reg.open(szRegKeyPath, HKEY_LOCAL_MACHINE)){
            if (reg[_T("DefaultGateway")].exists()){
                reg[_T("DefaultGateway")].clear_multi();
                reg[_T("DefaultGatewayMetric")].clear_multi();
            }
            reg.close();
        }
    }
    if (tbDNSServers.size()){
        for (size_t index = 0; index < tbDNSServers.size(); index++){
            if (bIsVisterorLater){
                if (index == 0)
                    command = _T("cmd.exe /C netsh interface ip set dnsserver name=\"");
                else
                    command = _T("cmd.exe /C netsh interface ip add dnsserver name=\"");
            }
            else{
                if (index == 0)
                    command = _T("cmd.exe /C netsh interface ip set dns name=\"");
                else
                    command = _T("cmd.exe /C netsh interface ip add dns name=\"");
            }

            command += connection;

            if (index == 0){
                if (bIsVisterorLater)
                    command += _T("\" source=static address=");
                else
                    command += _T("\" source=static addr=");

                command += tbDNSServers[index];
                command += _T(" register=PRIMARY");
            }
            else{
                if (bIsVisterorLater)
                    command += _T("\" address=");
                else
                    command += _T("\" addr=");

                command += tbDNSServers[index];
                command += _T(" index=");
                command += macho::stringutils::format(L"%d", index + 1);
            }
            macho::windows::process::exec_console_application_with_retry(command);
        }
    }
    else if (is_enable_dhcp){
        if (bIsVisterorLater)
            command = _T("cmd.exe /C netsh interface ip set dnsserver name=\"");
        else
            command = _T("cmd.exe /C netsh interface ip set dns name=\"");

        command += connection;
        command += _T("\" source=dhcp");
        macho::windows::process::exec_console_application_with_retry(command);
    }

    if (tbWINSs.size()){
        for (size_t index = 0; index < tbWINSs.size(); index++){
            if (bIsWin2k8r2orLater){
                if (index == 0)
                    command = _T("cmd.exe /C netsh interface ip set winsservers name=\"");
                else
                    command = _T("cmd.exe /C netsh interface ip add winsservers name=\"");
            }
            else if (bIsVisterorLater){
                if (index == 0)
                    command = _T("cmd.exe /C netsh interface ip set winsserver name=\"");
                else
                    command = _T("cmd.exe /C netsh interface ip add winsserver name=\"");
            }
            else {
                if (index == 0)
                    command = _T("cmd.exe /C netsh interface ip set wins name=\"");
                else
                    command = _T("cmd.exe /C netsh interface ip add wins name=\"");
            }

            command += connection;

            if (index == 0){
                if (bIsVisterorLater)
                    command += _T("\" source=static address=");
                else
                    command += _T("\" source=static addr=");

                command += tbWINSs[index];
            }
            else{
                if (bIsVisterorLater)
                    command += _T("\" address=");
                else
                    command += _T("\" addr=");

                command += tbWINSs[index];
                command += _T(" index=2");
            }
            macho::windows::process::exec_console_application_with_retry(command);
        }
    }
    else if (is_enable_dhcp){
        if (bIsWin2k8r2orLater)
            command = _T("cmd.exe /C netsh interface ip set winsservers name=\"");
        else if (bIsVisterorLater)
            command = _T("cmd.exe /C netsh interface ip set winsserver name=\"");
        else
            command = _T("cmd.exe /C netsh interface ip set wins name=\"");

        command += connection;
        command += _T("\" source=dhcp");
        macho::windows::process::exec_console_application_with_retry(command);
    }
}

void winpe_settings::sync_time(){
    std::vector<std::string> time_servers;
    time_servers.push_back("time.google.com");
    time_servers.push_back("time1.google.com");
    time_servers.push_back("time2.google.com");
    time_servers.push_back("time3.google.com");
    time_servers.push_back("time4.google.com");
    time_servers.push_back("time.windows.com");
    time_servers.push_back("time.nist.gov");
    time_servers.push_back("time-nw.nist.gov");
    time_servers.push_back("time-a.nist.gov");
    time_servers.push_back("time-b.nist.gov");
    time_servers.push_back("pool.ntp.org");

    foreach(std::string var, time_servers){
        boost::posix_time::ptime pt;
        if (macho::ntp_client::get_time(var, pt)){
            SYSTEMTIME st;
            boost::gregorian::date::ymd_type ymd = pt.date().year_month_day();
            st.wYear = ymd.year;
            st.wMonth = ymd.month;
            st.wDay = ymd.day;
            st.wDayOfWeek = pt.date().day_of_week();

            // Now extract the hour/min/second field from time_duration
            boost::posix_time::time_duration td = pt.time_of_day();
            st.wHour = static_cast<WORD>(td.hours());
            st.wMinute = static_cast<WORD>(td.minutes());
            st.wSecond = static_cast<WORD>(td.seconds());

            // Although ptime has a fractional second field, SYSTEMTIME millisecond
            // field is 16 bit, and will not store microsecond. We will treat this
            // field separately later.
            st.wMilliseconds = 0;
            if (SetSystemTime(&st)){
                LOG(LOG_LEVEL_RECORD, L"Succeed to set time from time server(%s). UTC time (%s)", macho::stringutils::convert_ansi_to_unicode(var).c_str(), macho::stringutils::convert_ansi_to_unicode(boost::posix_time::to_simple_string(pt)).c_str());
                break;
            }
        }
        else{
            LOG(LOG_LEVEL_WARNING, L"Failed to get time from time server(%s).", macho::stringutils::convert_ansi_to_unicode(var).c_str());
        }
    }
}
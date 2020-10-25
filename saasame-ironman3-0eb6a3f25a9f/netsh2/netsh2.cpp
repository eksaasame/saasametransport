// netsh2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "boost\thread.hpp"
#include "boost\date_time.hpp"
#include "macho.h"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <VersionHelpers.h>
#include "..\gen-cpp\network_settings.h"
#include <locale>

using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;

bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    po::options_description command("Settings");
    command.add_options()
        ("config,c", po::wvalue<std::wstring>(), "Configure file")
        ("ip,i", po::wvalue<std::wstring>(), "IP Address(s)")
        ("subnet,s", po::wvalue<std::wstring>(), "Subnet Mask(s) - You must specify the subnet mask when setting IP address.")
        ("getway,g", po::wvalue<std::wstring>(), "Getway(s)")
        ("metric,m", po::value<int>()->default_value(0,"0"), "Getway Metric")
        ("dns,d", po::wvalue<std::wstring>(), "DNS(s)")
        ("wins,w", po::wvalue<std::wstring>(), "WINS(s)")
        ("reset,r", "reset the network settings as dhcp. (ignore other settings)")
        ;

    po::options_description target("Target");
    target.add_options()
        ("mac,a", po::wvalue<std::wstring>(), "network adapter mac address")
        ;

    po::options_description general("General");
    general.add_options()
        ("boot,b", "run command on next windows boot up")
        ("time,t", po::value<int>()->default_value(0, "0"), "wait seconds before changing the ip settings")
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;

    po::options_description all("Allowed options");
    all.add(general).add(command).add(target);

    try{
        std::wstring c = GetCommandLine();
#if _UNICODE
        po::store(po::wcommand_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#else
        po::store(po::command_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#endif
        po::notify(vm);
        if (vm.count("help") || (vm["level"].as<int>() > 5) || (vm["level"].as<int>() < 0)){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("ip")){
                if (!vm.count("subnet"))
                    std::cout << title << all << std::endl;
                else
                    result = true;
            }
            else if (vm.count("getway") || vm.count("dns") || vm.count("wins") || vm.count("reset") || vm.count("config"))
                result = true;
            else
                std::cout << title << all << std::endl;
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << command << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << command << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e){
        std::cout << title << command << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...){
        std::cout << title << command << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

void set_ip_settings_by_netsh_command(std::wstring settings_id, std::wstring connection, std::vector<std::wstring> tbIPAddress, std::vector<std::wstring> tbSubnetMask, std::vector<std::wstring> tbDNSServers, std::vector<std::wstring> tbGateways, std::wstring gatewaymetric, std::vector<std::wstring> tbWINSs, bool is_enable_dhcp){

    std::wstring command;
    bool bIsWin2k8r2orLater = IsWindows7OrGreater();
    bool bIsVisterorLater = IsWindowsVistaOrGreater();
    std::wstring    szRegKeyPath = _T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\") + settings_id;
    if (tbIPAddress.size() && tbSubnetMask.size()){
        if (tbIPAddress.size() != tbSubnetMask.size())
            return ;
        for (size_t index = 0; index < tbIPAddress.size(); index++){
            if ( index ==  0 )
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
        //exec_console_application(command);

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
                command += stringutils::format(L"%d", index + 1);
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

void set_ip_settings_by_netsh_command(std::wstring settings_id, std::wstring connection, std::vector<std::string> tbIPAddress, std::vector<std::string> tbSubnetMask, std::vector<std::string> tbDNSServers, std::vector<std::string> tbGateways, std::wstring gatewaymetric, bool is_enable_dhcp){
    std::vector<std::wstring> _tbIPAddress, _tbSubnetMask, _tbDNSServers, _tbGateways, _tbWINSs;
    foreach(std::string addr, tbIPAddress)
        _tbIPAddress.push_back(macho::stringutils::convert_ansi_to_unicode(addr));
    foreach(std::string addr, tbSubnetMask)
        _tbSubnetMask.push_back(macho::stringutils::convert_ansi_to_unicode(addr));
    foreach(std::string addr, tbDNSServers)
        _tbDNSServers.push_back(macho::stringutils::convert_ansi_to_unicode(addr));
    foreach(std::string addr, tbGateways)
        _tbGateways.push_back(macho::stringutils::convert_ansi_to_unicode(addr));
    set_ip_settings_by_netsh_command(settings_id, connection, _tbIPAddress, _tbSubnetMask, _tbDNSServers, _tbGateways, gatewaymetric, _tbWINSs, is_enable_dhcp);
}

void remove_duplicated_ip_address(std::vector<std::wstring> tbIPAddress, std::vector< std::wstring > tbOnlineAdaptersList){
    registry reg;
    if (tbIPAddress.size()){
        if (reg.open(_T("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces"), HKEY_LOCAL_MACHINE)){
            if (reg.refresh_subkeys()){
                for (size_t index = 0; index < tbIPAddress.size(); index++){
                    bool IsFound = false;
                    for (int iSubKeyCount = 0; iSubKeyCount < reg.subkeys_count(); iSubKeyCount++){
                        bool IsOnline = false;
                        std::wstring keypath = reg.subkey(iSubKeyCount).key_path();
                        for (size_t iOnlineCount = 0; iOnlineCount < tbOnlineAdaptersList.size(); iOnlineCount++){
                            if (keypath.find(tbOnlineAdaptersList[iOnlineCount]) != std::wstring::npos){
                                IsOnline = true;
                                LOG(LOG_LEVEL_INFO, TEXT("Found online adapter (%s)\n"), reg.subkey(iSubKeyCount).key_path().c_str());
                                break;
                            }
                        }
                        if (IsOnline)
                            continue;

                        LOG(LOG_LEVEL_INFO, TEXT("Offline adapter (%s)\n"), reg.subkey(iSubKeyCount).key_path().c_str());

                        if (reg.subkey(iSubKeyCount)[_T("IPAddress")].exists()){
                            for (size_t iMultiCount = 0; iMultiCount < reg.subkey(iSubKeyCount)[_T("IPAddress")].get_multi_count(); iMultiCount++){
                                LOG(LOG_LEVEL_INFO, TEXT("Offline IPAddress (%s)\n"), reg.subkey(iSubKeyCount)[_T("IPAddress")].get_multi_at(iMultiCount));

                                if (wcscmp(reg.subkey(iSubKeyCount)[_T("IPAddress")].get_multi_at(iMultiCount), tbIPAddress[index].c_str()) == 0){
                                    LOG(LOG_LEVEL_INFO, TEXT("Find and remove the duplicated and orphan ip address (%s) from the key (%s).\n"), tbIPAddress[index].c_str(), reg.subkey(iSubKeyCount).key_path().c_str());

                                    reg.subkey(iSubKeyCount)[_T("IPAddress")].remove_multi_at(iMultiCount);
                                    reg.subkey(iSubKeyCount)[_T("SubnetMask")].remove_multi_at(iMultiCount);

                                    IsFound = true;
                                    break;
                                }

                                if (IsFound)
                                    break;
                            }
                        }
                    }
                }
            }
            else{
                LOG(LOG_LEVEL_ERROR, TEXT("Enumerate registry key error (%d)"), GetLastError());
            }
        }
        else{
            LOG(LOG_LEVEL_ERROR, TEXT("Open registry key error (%d)"), GetLastError());
        }
    }
    else{
        LOG(LOG_LEVEL_INFO, TEXT("tbIPAddress is empty"));
    }
}

void remove_duplicated_ip_address(std::vector<std::string> tbIPAddress, std::vector< std::wstring > tbOnlineAdaptersList){
    std::vector<std::wstring> _tbIPAddress;
    foreach(std::string addr, tbIPAddress)
        _tbIPAddress.push_back(macho::stringutils::convert_ansi_to_unicode(addr));
    remove_duplicated_ip_address(_tbIPAddress, tbOnlineAdaptersList);
}

int _tmain(int argc, _TCHAR* argv[])
{
    try{
        setlocale(LC_ALL, "");
        po::variables_map vm;
        boost::filesystem::path logfile;
        std::wstring app = macho::windows::environment::get_execution_full_path();
        if (macho::windows::environment::is_running_as_local_system())
            logfile = boost::filesystem::path(macho::windows::environment::get_windows_directory()) / (boost::filesystem::path(app).filename().wstring() + L".log");
        else
            logfile = macho::windows::environment::get_execution_full_path() + L".log";
        macho::set_log_file(logfile.wstring());
        macho::windows::com_init com;
        if (command_line_parser(vm)){
            macho::set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());
            LOG(LOG_LEVEL_RECORD, L"%s running...", app.c_str());
            if ((!vm.count("boot")) && vm["time"].as<int>()){
                std::wcout << L"Waitting (" << vm["time"].as<int>() << L") seconds..." << std::endl;
                LOG(LOG_LEVEL_RECORD, L"Waitting (%d) seconds...", vm["time"].as<int>());
                boost::this_thread::sleep(boost::posix_time::seconds(vm["time"].as<int>()));
            }
            std::vector<std::wstring> tbIPAddress;
            std::vector<std::wstring> tbSubnet;
            std::vector<std::wstring> tbGateway;
            std::vector<std::wstring> tbDNSs;
            std::vector<std::wstring> tbWins;
            std::wstring metric = stringutils::format(L"%d", vm["metric"].as<int>());
            if (!vm.count("reset") && !vm.count("config")){
                if (vm.count("ip")) tbIPAddress = stringutils::tokenize2(vm["ip"].as<std::wstring>(), L",;", 0, false);
                if (vm.count("subnet")) tbSubnet = stringutils::tokenize2(vm["subnet"].as<std::wstring>(), L",;", 0, false);
                if (vm.count("getway")) tbGateway = stringutils::tokenize2(vm["getway"].as<std::wstring>(), L",;", 0, false);
                if (vm.count("dns")) tbDNSs = stringutils::tokenize2(vm["dns"].as<std::wstring>(), L",;", 0, false);
                if (vm.count("wins")) tbWins = stringutils::tokenize2(vm["wins"].as<std::wstring>(), L",;", 0, false);          
            }

            if (tbIPAddress.size() > 0 && tbIPAddress.size() != tbSubnet.size()){
                std::wcout << L"The number of ip address and subnet mask must be the same." << std::endl;
                LOG(LOG_LEVEL_ERROR, L"The number of ip address and subnet mask must be the same.");
                return 1;
            }

            macho::windows::task_scheduler::ptr scheduler = macho::windows::task_scheduler::get();
            if (scheduler){
                macho::windows::task_scheduler::task::ptr t = scheduler->get_task(L"configure_network_settings");
                if (t){
                    if (scheduler->delete_task(L"configure_network_settings")){
                        LOG(LOG_LEVEL_RECORD, L"succeeded to remove the old \"configure_network_settings\" task.");
                        std::wcout << L"succeeded to remove the old \"configure_network_settings\" task." << std::endl;
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, L"failed to remove the old \"configure_network_settings\" task.");
                        std::wcout << L"failed to remove the old \"configure_network_settings\" task." << std::endl;
                    }
                }
                if (vm.count("boot")){
                    std::wstring parameters;
                    parameters.append(stringutils::format(L"-t %d", vm["time"].as<int>()));
                    parameters.append(stringutils::format(L" -l %d", vm["level"].as<int>()));
                    if (vm.count("reset")){
                        parameters.append(L" -r");
                    }
                    else if (vm.count("config")){
                        parameters.append(stringutils::format(L" -c %s", vm["config"].as<std::wstring>().c_str()));
                    }
                    else{
                        if (vm.count("ip")) parameters.append(stringutils::format(L" -i %s", vm["ip"].as<std::wstring>().c_str()));
                        if (vm.count("subnet")) parameters.append(stringutils::format(L" -s %s", vm["subnet"].as<std::wstring>().c_str()));
                        if (vm.count("getway")) parameters.append(stringutils::format(L" -g %s", vm["getway"].as<std::wstring>().c_str()));
                        if (vm.count("dns")) parameters.append(stringutils::format(L" -d %s", vm["dns"].as<std::wstring>().c_str()));
                        if (vm.count("wins")) parameters.append(stringutils::format(L" -w %s", vm["wins"].as<std::wstring>().c_str()));
                        parameters.append(stringutils::format(L" -m %d", vm["metric"].as<int>()));
                    }

                    t = scheduler->create_task(L"configure_network_settings", app, parameters);
                    if (t){
                        LOG(LOG_LEVEL_RECORD, L"succeeded to create a new \"configure_network_settings\" task (%s %s).", app.c_str(), parameters.c_str());
                        std::wcout << L"succeeded to create a new \"configure_network_settings\" task (" << app << L" " << parameters << L")." << std::endl;
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, L"failed to create a new \"configure_network_settings\" task (%s %s).", app.c_str(), parameters.c_str());
                        std::wcout << L"failed to create a new \"configure_network_settings\" task (" << app << L" " << parameters << L")." << std::endl;
                    }
                    return 0;
                }
            }

            macho::windows::network::adapter::vtr adapters = macho::windows::network::get_network_adapters();
            if (vm.count("config")){
                std::string default_mtu;
                std::set<network_info> network_infos = network_settings::load(boost::filesystem::path(vm["config"].as<std::wstring>()), default_mtu);
                std::set<std::wstring> configured_networks;

                if (!IsWindowsVistaOrGreater()){
                    std::vector<std::wstring> tbOnlineAdapters;
                    foreach(macho::windows::network::adapter::ptr adapter, adapters){
                        macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                        if (!adapter_settings->ip_enabled())
                            continue;
                        std::wstring setting_id = adapter_settings->setting_id();
                        if (setting_id.length())
                            tbOnlineAdapters.push_back(setting_id);
                    }
                    foreach(network_info info, network_infos){
                        if (info.ip_addresses.size())
                            remove_duplicated_ip_address(info.ip_addresses, tbOnlineAdapters);
                    }
                }

                foreach(network_info info, network_infos){
                    if (info.mac_address.length()){
                        std::wstring mac_address = macho::stringutils::convert_ansi_to_unicode(info.mac_address);
                        foreach(macho::windows::network::adapter::ptr adapter, adapters){
                            macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                            std::wstring caption = stringutils::tolower(adapter_settings->caption());
                            if ((!adapter_settings->ip_enabled()) || (caption.find(_T("loopback")) != std::wstring::npos) || (caption.find(_T("loopback")) != std::wstring::npos))
                                continue;               
                            if (0 == wcsicmp(mac_address.c_str(), adapter->mac_address().c_str())){
                                configured_networks.insert(adapter->mac_address());
                                std::wcout << L"Apply network settings on the following adapter : " << std::endl;
                                std::wcout << L"adapter name :      " << adapter->name() << std::endl;
                                std::wcout << L"net connection id : " << adapter->net_connection_id() << std::endl;
                                std::wcout << L"setting id :        " << adapter_settings->setting_id() << std::endl;
                                std::wcout << L"mac address :       " << adapter_settings->mac_address() << std::endl << std::endl;
                                LOG(LOG_LEVEL_RECORD, L"Apply network settings on the following adapter :");
                                LOG(LOG_LEVEL_RECORD, L"adapter name :      %s", adapter->name().c_str());
                                LOG(LOG_LEVEL_RECORD, L"net connection id : %s", adapter->net_connection_id().c_str());
                                LOG(LOG_LEVEL_RECORD, L"setting id :        %s", adapter_settings->setting_id().c_str());
                                LOG(LOG_LEVEL_RECORD, L"mac address :       %s", adapter_settings->mac_address().c_str());
                                set_ip_settings_by_netsh_command(adapter_settings->setting_id(), adapter->net_connection_id(), info.ip_addresses, info.subnet_masks, info.dnss, info.gateways, metric, info.is_dhcp_v4);
                                break;
                            }
                        }
                    }
                }

                foreach(network_info info, network_infos){
                    if (0 == info.mac_address.length()){
                        foreach(macho::windows::network::adapter::ptr adapter, adapters){
                            macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                            std::wstring caption = stringutils::tolower(adapter_settings->caption());
                            if ((!adapter_settings->ip_enabled()) || (caption.find(_T("loopback")) != std::wstring::npos) || (caption.find(_T("loopback")) != std::wstring::npos))
                                continue;
                            std::wstring mac_address = adapter->mac_address();
                            auto it = std::find_if(configured_networks.begin(), configured_networks.end(), [&mac_address](const std::wstring& obj){return obj == mac_address; });
                            if (it != configured_networks.end())
                                continue;
                            configured_networks.insert(adapter->mac_address());
                            std::wcout << L"Apply network settings on the following adapter : " << std::endl;
                            std::wcout << L"adapter name :      " << adapter->name() << std::endl;
                            std::wcout << L"net connection id : " << adapter->net_connection_id() << std::endl;
                            std::wcout << L"setting id :        " << adapter_settings->setting_id() << std::endl;
                            std::wcout << L"mac address :       " << adapter_settings->mac_address() << std::endl << std::endl;
                            LOG(LOG_LEVEL_RECORD, L"Apply network settings on the following adapter :");
                            LOG(LOG_LEVEL_RECORD, L"adapter name :      %s", adapter->name().c_str());
                            LOG(LOG_LEVEL_RECORD, L"net connection id : %s", adapter->net_connection_id().c_str());
                            LOG(LOG_LEVEL_RECORD, L"setting id :        %s", adapter_settings->setting_id().c_str());
                            LOG(LOG_LEVEL_RECORD, L"mac address :       %s", adapter_settings->mac_address().c_str());
                            set_ip_settings_by_netsh_command(adapter_settings->setting_id(), adapter->net_connection_id(), info.ip_addresses, info.subnet_masks, info.dnss, info.gateways, metric, info.is_dhcp_v4);
                            break;
                        }
                    }
                }

                if (default_mtu.length()){
                    std::wstring wdefault_mtu = macho::stringutils::convert_ansi_to_unicode(default_mtu);
                    foreach(macho::windows::network::adapter::ptr adapter, adapters){
                        macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                        std::wstring caption = stringutils::tolower(adapter_settings->caption());
                        if ((!adapter_settings->ip_enabled()) || (caption.find(_T("loopback")) != std::wstring::npos) || (caption.find(_T("loopback")) != std::wstring::npos))
                            continue;
                        LOG(LOG_LEVEL_RECORD, L"Apply network settings on the following adapter :");
                        LOG(LOG_LEVEL_RECORD, L"net connection id : %s", adapter->net_connection_id().c_str());
                        LOG(LOG_LEVEL_RECORD, L"mtu : %s", wdefault_mtu.c_str());

                        std::wstring command = _T("cmd.exe /C netsh interface ipv4 set subinterface \"");
                        command += adapter->net_connection_id();
                        command += _T("\" mtu=");
                        command += wdefault_mtu;
                        command += _T(" store=persistent");
                        macho::windows::process::exec_console_application_with_retry(command);
                    }
                }
            }
            else{
                if ((!IsWindowsVistaOrGreater()) && tbIPAddress.size()){
                    std::vector<std::wstring> tbOnlineAdapters;
                    foreach(macho::windows::network::adapter::ptr adapter, adapters){
                        macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                        if (!adapter_settings->ip_enabled())
                            continue;
                        std::wstring setting_id = adapter_settings->setting_id();
                        if (setting_id.length())
                            tbOnlineAdapters.push_back(setting_id);
                    }
                    remove_duplicated_ip_address(tbIPAddress, tbOnlineAdapters);
                }
                bool is_found = false;
                if (vm.count("mac")){
                    foreach(macho::windows::network::adapter::ptr adapter, adapters){
                        macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                        if (!adapter_settings->ip_enabled())
                            continue;
                        if (0 == wcsicmp( vm["mac"].as<std::wstring>().c_str(), adapter->mac_address().c_str())){
                            is_found = true;
                            std::wcout << L"Apply network settings on the following adapter : " << std::endl;
                            std::wcout << L"adapter name :      " << adapter->name() << std::endl;
                            std::wcout << L"net connection id : " << adapter->net_connection_id() << std::endl;
                            std::wcout << L"setting id :        " << adapter_settings->setting_id() << std::endl;
                            std::wcout << L"mac address :       " << adapter_settings->mac_address() << std::endl << std::endl;
                            LOG(LOG_LEVEL_RECORD, L"Apply network settings on the following adapter :");
                            LOG(LOG_LEVEL_RECORD, L"adapter name :      %s", adapter->name().c_str());
                            LOG(LOG_LEVEL_RECORD, L"net connection id : %s", adapter->net_connection_id().c_str());
                            LOG(LOG_LEVEL_RECORD, L"setting id :        %s", adapter_settings->setting_id().c_str());
                            LOG(LOG_LEVEL_RECORD, L"mac address :       %s", adapter_settings->mac_address().c_str());
                            set_ip_settings_by_netsh_command(adapter_settings->setting_id(), adapter->net_connection_id(), tbIPAddress, tbSubnet, tbDNSs, tbGateway, metric, tbWins, vm.count("reset") == 1);
                            break;
                        }
                    }
                }
                else{
                    foreach(macho::windows::network::adapter::ptr adapter, adapters){
                        macho::windows::network::adapter_config::ptr adapter_settings = adapter->get_setting();
                        std::wstring caption = stringutils::tolower(adapter_settings->caption());
                        if ((!adapter_settings->ip_enabled()) || (caption.find(_T("loopback")) != std::wstring::npos) || (caption.find(_T("loopback")) != std::wstring::npos))
                            continue;
                        is_found = true;
                        std::wcout << L"Apply network settings on the following adapter : " << std::endl;
                        std::wcout << L"adapter name :      " << adapter->name() << std::endl;
                        std::wcout << L"net connection id : " << adapter->net_connection_id() << std::endl;
                        std::wcout << L"setting id :        " << adapter_settings->setting_id() << std::endl;
                        std::wcout << L"mac address :       " << adapter_settings->mac_address() << std::endl << std::endl;
                        LOG(LOG_LEVEL_RECORD, L"Apply network settings on the following adapter :");
                        LOG(LOG_LEVEL_RECORD, L"adapter name :      %s", adapter->name().c_str());
                        LOG(LOG_LEVEL_RECORD, L"net connection id : %s", adapter->net_connection_id().c_str());
                        LOG(LOG_LEVEL_RECORD, L"setting id :        %s", adapter_settings->setting_id().c_str());
                        LOG(LOG_LEVEL_RECORD, L"mac address :       %s", adapter_settings->mac_address().c_str());
                        set_ip_settings_by_netsh_command(adapter_settings->setting_id(), adapter->net_connection_id(), tbIPAddress, tbSubnet, tbDNSs, tbGateway, metric, tbWins, vm.count("reset") == 1);
                        if (0 == vm.count("reset"))
                            break;
                    }
                }
                if (!is_found){
                    if (vm.count("mac")){
                        std::wcout << L"Can't find the network adapter with mac address : " << vm["mac"].as<std::wstring>() << std::endl;
                        LOG(LOG_LEVEL_ERROR, L"Can't find the network adapter with mac address : %s", vm["mac"].as<std::wstring>().c_str());
                    }
                    else{
                        std::wcout << L"Can't find any available network adapter." << std::endl;
                        LOG(LOG_LEVEL_ERROR, L"Can't find any available network adapter.");
                    }
                }
            }
        }
    }
    catch (macho::exception_base& ex){
        std::wcout << macho::get_diagnostic_information(ex) << std::endl;
        LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        std::wcout << macho::stringutils::convert_ansi_to_unicode(ex.what()) << std::endl;
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (const boost::exception &ex){
        std::wcout << macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")) << std::endl;
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
    }
    catch (const std::exception& ex){
        std::wcout << macho::stringutils::convert_ansi_to_unicode(ex.what()) << std::endl;
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        std::wcout << L"Unknown exception" << std::endl;
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return 0;
}


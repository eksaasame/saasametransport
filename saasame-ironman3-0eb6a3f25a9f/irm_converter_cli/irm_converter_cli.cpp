// irm_converter_cli.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "macho.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "boost\thread.hpp"
#include "boost\date_time.hpp"
#include <io.h>
#include <stdlib.h>
#include "irm_converter.h"
#include "..\irm_host_mgmt\vhdx.h"
#include "..\gen-cpp\network_settings.h"

#pragma comment(lib, "irm_converter.lib")

using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;

typedef struct _win_service{
    TCHAR name[128];
    DWORD type;
}win_service, *pwin_service;

static std::wstring const _services[] = {
    //VMWARE
    _T("VMTools"),
    _T("VMUpgradeHelper"),
    _T("VMUSBArbService"),
    //Hyper-V
    _T("vmicheartbeat"),
    _T("vmickvpexchange"),
    _T("vmicshutdown"),
    _T("vmictimesync"),
    _T("vmicvss"),
    //IBM
    _T("tier1slp"),
    _T("wmicimserver"),
    _T("cimlistener"),
    _T("dirserver"),
    _T("ibmsa"),
    _T("IBMTivoliCommonAgent0"),
    //HP
    _T("#CIMnotify"),
    _T("CqMgHost"),
    _T("CpqNicMgmt"),
    _T("CqMgServ"),
    _T("CqMgStor"),
    _T("CpqRcmc"),
    _T("sysdown"),
    _T("Cissesrv"),
    _T("SysMgmtHp"),
    _T("cpqvcagent"),
    _T("HPWMISTOR"),
    //DELL
    _T("Server Administrator"),
    _T("dcstor32"),
    _T("dcevt32"),
    _T("omsad"),
    _T("mr2kserv"),
    _T("#AeXAgentSrvHost"),
    _T("AltirisClientMsgDispatcher"),
    _T("ctdataloader"),
    _T("EventEngine"),
    _T("EventReceiver"),
    _T("AltirisReceiverService"),
    _T("atrshost"),
    _T("AeXSvc"),
    _T("AltirisServiceHoster"),
    _T("AltirisSupportService"),
    _T("MetricProvider"),
    _T("#AltirisAgentProvider"),
    _T("#AMTRedirectionService"),
    _T("AeXNSClient")
};

bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    //guid_ g = guid_::create();
    po::options_description target("Virtual Disk File");
    target.add_options()
        ("vhdx,f", po::wvalue<std::wstring>(), "VHDX file for conversion.")
        ("number,d", po::value<int>(), "Disk number for conversion.")
        ("style,s", po::wvalue<std::wstring>()->default_value(L"mbr", "mbr"), "Partition style. (gpt or mbr)")
        ;
    po::options_description general("General");
    general.add_options()
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;
    po::options_description all("Allowed options");
    all.add(target).add(general);

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
            if (vm.count("vhdx") && vm.count("number"))
                std::cout << title << all << std::endl;
            else if ( vm.count("vhdx") || vm.count("number") )
                result = true;
            else
                std::cout << title << all << std::endl;
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e){
        std::cout << title << all << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...){
        std::cout << title << all << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

void initialize_conv_parameter(irm_conv_parameter& parameter, const int& disk_number){
    storage::ptr stg = storage::get();
    std::string default_mtu = "";
    const std::set<saasame::transport::network_info> infos;

    parameter.sectors_per_track = 63;
    parameter.tracks_per_cylinder = 255;

    for (int i = 0; i < 44; i++){
        parameter.services[_services[i]] = 4;
    }

    wmi_services wmi;
    wmi.connect(L"CIMV2");
    wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
    std::wstring manufacturer = macho::stringutils::tolower((std::wstring)computer_system[L"Manufacturer"]);
    std::wstring model = macho::stringutils::tolower((std::wstring)computer_system[L"Model"]);
    LOG(LOG_LEVEL_RECORD, L"Machine manufacturer : %s, model : %s", manufacturer.c_str(), model.c_str());
    if (macho::windows::environment::is_winpe()){
        storage::disk::ptr _d = stg->get_disk(disk_number);
        if (_d){
            parameter.sectors_per_track = _d->sectors_per_track();
            parameter.tracks_per_cylinder = _d->tracks_per_cylinder();
        }
        parameter.type = conversion::type::any_to_any;
        /*
        Serial number: SELECT IdentifyingNumber FROM Win32_ComputerSystemProduct
        System Manufacturer: SELECT Manufacturer FROM Win32_ComputerSystem
        System Model: SELECT Model FROM Win32_ComputerSystem
        System Model Name: SELECT Name FROM Win32_ComputerSystemProduct
        System Model Version: SELECT Version FROM Win32_ComputerSystemProduct
        */
        if (std::wstring::npos != manufacturer.find(L"vmware")){
            for (int i = 0; i < 3; i++){
                parameter.services[_services[i]] = 2;
            }
        }
        else if (std::wstring::npos != manufacturer.find(L"microsoft")){
            for (int i = 3; i < 5; i++){
                parameter.services[_services[i]] = 2;
            }
        }
        else if (std::wstring::npos != manufacturer.find(L"ibm")){
            for (int i = 8; i < 6; i++){
                parameter.services[_services[i]] = 2;
            }
        }
        else if (std::wstring::npos != manufacturer.find(L"hewlett-packard")){ // Hewlett-Packard
            for (int i = 15; i < 10; i++){
                parameter.services[_services[i]] = 2;
            }
        }
        else if (std::wstring::npos != manufacturer.find(L"dell")){
            for (int i = 25; i < 19; i++){
                if (_services[i][0] == _T('#'))
                    parameter.services[_services[i]] = 3;
                else
                    parameter.services[_services[i]] = 2;
            }
        }
        else if (std::wstring::npos != manufacturer.find(L"openstack") || std::wstring::npos != model.find(L"openstack")){
            parameter.type = conversion::type::openstack;
            default_mtu = "1400";
        }
        else if (std::wstring::npos != manufacturer.find(L"bochs"))
            parameter.type = conversion::type::openstack;
        else if (std::wstring::npos != manufacturer.find(L"alibaba"))
            parameter.type = conversion::type::openstack;
        else if (std::wstring::npos != manufacturer.find(L"xen"))
            parameter.type = conversion::type::xen;
        else if (std::wstring::npos != manufacturer.find(L"red hat") && std::wstring::npos != model.find(L"kvm"))
            parameter.type = conversion::type::openstack;
    }
    else{
#ifdef _DEBUG
        parameter.type = conversion::type::openstack;
        default_mtu = "1400";
#else
        if (std::wstring::npos != manufacturer.find(L"vmware"))
            parameter.type = conversion::type::vmware;
        else if (std::wstring::npos != manufacturer.find(L"microsoft"))
            parameter.type = conversion::type::hyperv;
        else if (std::wstring::npos != manufacturer.find(L"openstack") || std::wstring::npos != model.find(L"openstack")){
            parameter.type = conversion::type::openstack;
            default_mtu = "1400";
        }
        else if (std::wstring::npos != manufacturer.find(L"bochs"))
            parameter.type = conversion::type::openstack;
        else if (std::wstring::npos != manufacturer.find(L"alibaba"))
            parameter.type = conversion::type::openstack;
        else if (std::wstring::npos != manufacturer.find(L"xen"))
            parameter.type = conversion::type::xen;
        else if (std::wstring::npos != manufacturer.find(L"red hat") && std::wstring::npos != model.find(L"kvm"))
            parameter.type = conversion::type::openstack;
#endif

    }

    {
        registry reg_local;
        if (reg_local.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg_local[L"DrvRoot"].exists()){
                parameter.drivers_path = reg_local[L"DrvRoot"].wstring();
            }
            if (reg_local[L"SkipDriverInjection"].exists() && (DWORD)reg_local[L"SkipDriverInjection"] > 0){
                parameter.drivers_path.clear();
                parameter.type = conversion::type::any_to_any;
            }
            else if (reg_local[L"ConversionType"].exists() && reg_local[L"ConversionType"].is_dword()){
                parameter.type = (conversion::type)(DWORD)reg_local[L"ConversionType"];
            }
            if (reg_local[L"DefaultMTU"].exists() && reg_local[L"DefaultMTU"].is_string()){
                default_mtu = reg_local[L"DefaultMTU"].string();
            }
            reg_local.close();
        }
    }
    parameter.config = network_settings::to_string(infos, default_mtu);
}

int _tmain(int argc, _TCHAR* argv[])
{
    DWORD exit_code = 0;
    po::variables_map vm;
    boost::filesystem::path logfile;
    std::wstring app = macho::windows::environment::get_execution_full_path();
    logfile = macho::windows::environment::get_execution_full_path() + L".log";
    macho::set_log_file(logfile.wstring());
    if (command_line_parser(vm)){
        macho::set_log_level((macho::TRACE_LOG_LEVEL)vm["level"].as<int>());
        macho::windows::mutex running(boost::filesystem::path(app).filename().wstring());
        if (!running.trylock()){
            std::wstring out = boost::str(boost::wformat(L"The application (%s) is already running.") % boost::filesystem::path(app).filename().wstring());
            LOG(LOG_LEVEL_RECORD, out.c_str());
            std::wcout << out << std::endl;
        }
        else{
            try{
                com_init com;
                
                if (!macho::windows::environment::is_running_as_local_system()) {
                    macho::windows::task_scheduler::ptr scheduler = macho::windows::task_scheduler::get();
                    if (scheduler){
                        std::wstring parameters;
                        if (vm.count("vhdx"))
                            parameters.append(stringutils::format(L"-f %s", vm["vhdx"].as<std::wstring>().c_str()));
                        else if (vm.count("number"))
                            parameters.append(stringutils::format(L"-d %d", vm["number"].as<int>()));
                        if (vm.count("style"))
                            parameters.append(stringutils::format(L" -s %s", vm["style"].as<std::wstring>().c_str()));

                        parameters.append(stringutils::format(L" -l %d", vm["level"].as<int>()));
                        if (scheduler->get_task(L"convert_cli")){
                            scheduler->delete_task(L"convert_cli");
                        }
                        macho::windows::task_scheduler::task::ptr t = scheduler->create_task(L"convert_cli", macho::windows::environment::get_execution_full_path(), parameters);
                        if (t){
                            t->run();
                            do {
                                boost::this_thread::sleep(boost::posix_time::seconds(5));
                            } while (scheduler->is_running_task(L"convert_cli", exit_code));

                            scheduler->delete_task(L"convert_cli");
                            if (exit_code){
                                std::cout << "Failed to convert the disk." << std::endl;
                            }
                            else{
                                std::cout << "Succeeded to convert the disk." << std::endl;
                            }
                        }
                    }
                }
                else{
                    auto_lock lock(running);
#if _DEBUG
                    boost::this_thread::sleep(boost::posix_time::seconds(20));
#endif
                    irm_conv_parameter parameter;
                    bool to_mbr = false, to_gpt = false;
                    if (vm.count("style")){
                        to_mbr = (0 == _wcsicmp(L"mbr", vm["style"].as<std::wstring>().c_str()));
                        to_gpt = (0 == _wcsicmp(L"gpt", vm["style"].as<std::wstring>().c_str()));
                    }
                    if (vm.count("vhdx")){
                        boost::filesystem::path vhdx_file = vm["vhdx"].as<std::wstring>();
                        boost::filesystem::path temp_vhdx_file = vhdx_file.parent_path() / boost::str(boost::wformat(L"%s_diff.vhdx") % vhdx_file.filename().stem().wstring());
                        LOG(LOG_LEVEL_RECORD, L"Convert the virtual disk : %s", vhdx_file.wstring().c_str());
                        vhd_disk_info disk_info;
                        if (!(ERROR_SUCCESS == win_vhdx_mgmt::get_virtual_disk_information(vhdx_file.wstring(), disk_info) &&
                            ERROR_SUCCESS == win_vhdx_mgmt::create_vhdx_file(temp_vhdx_file.wstring(), CREATE_VIRTUAL_DISK_FLAG_NONE, disk_info.virtual_size, disk_info.block_size, disk_info.logical_sector_size, disk_info.physical_sector_size, vhdx_file.wstring()))){
                            LOG(LOG_LEVEL_ERROR, L"Failed to create the temp virtual disk file (%s) for conversion.", vhdx_file.wstring().c_str());
                            exit_code = 1;
                        }
                        else{
                            DWORD disk_number = -1;
                            if (!(ERROR_SUCCESS == win_vhdx_mgmt::attach_virtual_disk(temp_vhdx_file.wstring())
                                && ERROR_SUCCESS == win_vhdx_mgmt::get_attached_physical_disk_number(temp_vhdx_file.wstring(), disk_number))){
                                boost::filesystem::remove(temp_vhdx_file);
                                LOG(LOG_LEVEL_ERROR, L"Failed to attach the temp virtual disk file (%s) for conversion.", vhdx_file.wstring().c_str());
                                exit_code = 1;
                            }
                            else{
                                irm_converter converter;
                                bool result = false;
                                if (!(result = converter.initialize(disk_number))){
                                    LOG(LOG_LEVEL_ERROR, L"Failed to find any system information.");
                                    win_vhdx_mgmt::detach_virtual_disk(temp_vhdx_file.wstring());
                                    exit_code = 1;
                                }
                                else if (to_mbr && !(result = converter.gpt_to_mbr())){
                                    LOG(LOG_LEVEL_ERROR, L"Failed to gpt to mbr.");
                                    win_vhdx_mgmt::detach_virtual_disk(temp_vhdx_file.wstring());
                                    exit_code = 1;
                                }
                                else if (to_gpt && !(result = converter.remove_hybird_mbr())){
                                    LOG(LOG_LEVEL_ERROR, L"Failed to remove hybird mbr.");
                                    win_vhdx_mgmt::detach_virtual_disk(temp_vhdx_file.wstring());
                                    exit_code = 1;
                                }
                                else{
                                    initialize_conv_parameter(parameter, disk_number);
                                    if (!(result = converter.convert(parameter))){
                                        LOG(LOG_LEVEL_ERROR, L"Failed to convert the disk.");
                                        win_vhdx_mgmt::detach_virtual_disk(temp_vhdx_file.wstring());
                                        exit_code = 1;
                                    }
                                    else{
                                        LOG(LOG_LEVEL_RECORD, L"Succeeded to convert the disk.");
                                    }
                                }
                                win_vhdx_mgmt::detach_virtual_disk(temp_vhdx_file.wstring());
#ifndef _DEBUG
                                if (result) win_vhdx_mgmt::merge_virtual_disk(temp_vhdx_file.wstring());
#endif  
                                boost::filesystem::remove(temp_vhdx_file);
                            }
                        }
                    }
                    else if (vm.count("number")){
                        int disk_number = vm["number"].as<int>();
                        LOG(LOG_LEVEL_RECORD, L"Convert the disk : %d ", disk_number);
                        irm_converter converter;
                        if (!converter.initialize(disk_number)){
                            LOG(LOG_LEVEL_ERROR, L"Failed to find any system information.");
                            exit_code = 1;
                        }
                        else if (to_mbr && !converter.gpt_to_mbr()){
                            LOG(LOG_LEVEL_ERROR, L"Failed to convert gpt to mbr.");
                            exit_code = 1;
                        }
                        else if (to_gpt && !converter.remove_hybird_mbr()){
                            LOG(LOG_LEVEL_ERROR, L"Failed to remove hybird mbr.");
                            exit_code = 1;
                        }
                        else{
                            initialize_conv_parameter(parameter, disk_number);
                            if (!converter.convert(parameter)){
                                LOG(LOG_LEVEL_ERROR, L"Failed to convert the disk.");
                                exit_code = 1;
                            }
                            else{
                                LOG(LOG_LEVEL_RECORD, L"Succeeded to convert the disk.");
                            }
                        }
                    }
                }
            }
            catch (macho::exception_base& ex){
                std::wcout << macho::get_diagnostic_information(ex);
                LOG(LOG_LEVEL_ERROR, L"%s", macho::get_diagnostic_information(ex).c_str());
                exit_code = 1;
            }
            catch (const boost::filesystem::filesystem_error& ex){
                std::cout << ex.what() << std::endl;
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                exit_code = 1;
            }
            catch (const boost::exception &ex){
                std::cout << boost::exception_detail::get_diagnostic_information(ex, "error:") << std::endl;
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
                exit_code = 1;
            }
            catch (const std::exception& ex){
                std::cout << ex.what() << std::endl;
                LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
                exit_code = 1;
            }
            catch (...){
                exit_code = 1;
            }
        }
    }
    return exit_code;
}


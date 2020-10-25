// irm_agent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "boost\thread.hpp"
#include "boost\date_time.hpp"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "macho.h"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>

#include <codecvt>
#include "..\gen-cpp\temp_drive_letter.h"
using boost::property_tree::ptree;
using boost::property_tree::basic_ptree;
using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;

struct partition_path {
    partition_path() : partition_style(macho::windows::storage::ST_PST_UNKNOWN), signature(0), partition_number(0), is_system_disk(false), partition_size(0), partition_offset(0){}
    typedef boost::shared_ptr<partition_path> ptr;
    typedef  std::vector<ptr>                 vtr;
    macho::windows::storage::ST_PARTITION_STYLE							  partition_style;
    DWORD                                                                 signature;
    std::wstring                                                          guid;
    DWORD									                              partition_number;
    std::wstring							                              path;
    bool																  is_system_disk;
    uint64_t															  partition_size;
    uint64_t                                                              partition_offset;
    static bool							      save(const boost::filesystem::path& cfg, const partition_path::vtr& partitions);
    static partition_path::vtr                load(const boost::filesystem::path& cfg);
};

partition_path::vtr  partition_path::load(const boost::filesystem::path& cfg){
    partition_path::vtr parts;
    if (boost::filesystem::exists(cfg)){
        try{
            ptree root;
            std::ifstream is(cfg.wstring());
            boost::property_tree::read_json(is, root);
            for (ptree::value_type &part : root.get_child("partitions")) {
                partition_path::ptr _p(new partition_path());
                _p->partition_number = part.second.get<uint32_t>("partition_number");
                _p->partition_style = (macho::windows::storage::ST_PARTITION_STYLE)part.second.get<uint32_t>("partition_style");
                _p->signature = part.second.get<uint32_t>("signature");
                _p->path = macho::stringutils::convert_utf8_to_unicode(part.second.get<std::string>("path"));
                _p->guid = macho::stringutils::convert_utf8_to_unicode(part.second.get<std::string>("guid"));
                _p->is_system_disk = part.second.get<bool>("is_system_disk");
                _p->partition_size = part.second.get<uint64_t>("partition_size");
                _p->partition_offset = part.second.get<uint64_t>("partition_offset");
                if (!_p->path.empty()){
                    if (_p->path[_p->path.length() - 1] == L'\\' || _p->path[_p->path.length() - 1] == L'/')
                        _p->path.erase(_p->path.length() - 1);
                    if (1 == _p->path.length())
                        _p->path.append(L":");
                    _p->path.append(L"\\");
                }
                parts.push_back(_p);
            }
        }
        catch (boost::exception& ex){
            LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "can't read config info.")));
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"Cannot read the config file.");
        }
    }
    return parts;
}

bool partition_path::save(const boost::filesystem::path& cfg, const partition_path::vtr& partitions){
    bool   result = false;
    try{
        ptree root;
        ptree parts_node;
        for (partition_path::ptr p : partitions) {
            ptree part;
            part.put<uint32_t>("partition_style", p->partition_style);
            part.put<uint32_t>("signature", p->signature);
            part.put<std::string>("guid", macho::stringutils::convert_unicode_to_utf8(p->guid));
            part.put<uint32_t>("partition_number", p->partition_number);
            part.put<std::string>("path", macho::stringutils::convert_unicode_to_utf8(p->path));
            part.put<uint64_t>("partition_size", p->partition_size);
            part.put<uint64_t>("partition_offset", p->partition_offset);
            part.put<bool>("is_system_disk", p->is_system_disk);
            parts_node.push_back(std::make_pair("", part));
        }
        root.add_child("partitions", parts_node);
        boost::filesystem::path temp = cfg.string() + ".tmp";
        {
            std::ofstream output(temp.string(), std::ios::out | std::ios::trunc);
            std::locale loc(std::locale::classic(), new std::codecvt_utf8<wchar_t>);
            output.imbue(loc);
            boost::property_tree::write_json(output, root);
        }
        if (!MoveFileEx(temp.wstring().c_str(), cfg.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
            LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), cfg.wstring().c_str(), GetLastError());
        }
        else{
            result = true;
        }
    }
    catch (boost::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "Cannot output config info.")).c_str());
    }
    catch (...){
    }
    return result;
}

bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);

    po::options_description command("General");
    command.add_options()
        ("boot,b", "run command on next windows boot up")
        ("terminate,u", "terminate to run command on next windows boot up")
        ("time,t", po::value<int>()->default_value(0, "0"), "wait seconds")
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;

    po::options_description all("Allowed options");
    all.add(command);

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
            result = true;
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


std::wstring read_file(boost::filesystem::path p)
{
    std::wifstream wif(p.wstring());
    wif.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));
    std::wstringstream wss;
    wss << wif.rdbuf();
    return wss.str();
}

void remove_excluded_paths(storage::volume::ptr& v)
{
    boost::filesystem::path excluded = boost::filesystem::path(v->path()) / L".excluded";
    if (boost::filesystem::exists(excluded)){
        std::vector<std::wstring> paths = stringutils::tokenize2(read_file(excluded), L"\r\n", 0, false);
        foreach(std::wstring path, paths){
            boost::filesystem::path p = boost::filesystem::path(v->path()) / path.substr(1);
            if (boost::filesystem::exists(p)){
                if (boost::filesystem::is_directory(p)){
                    LOG(LOG_LEVEL_WARNING, _T("Remove subfolders and files from excluded folder (%s)."), p.wstring().c_str());
                    std::vector<boost::filesystem::path> sub_folders = environment::get_sub_folders(p, true);
                    std::vector<boost::filesystem::path> files = environment::get_files(p, L"*", true);
                    foreach(boost::filesystem::path file, files){
                        SetFileAttributesW(file.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                        try{
                            boost::filesystem::remove(file);
                        }
                        catch (...){
                            LOG(LOG_LEVEL_ERROR, L"failed to remove %s.", excluded.wstring().c_str());
                            std::wcout << L"failed to remove " << excluded.wstring() << std::endl;
                        }
                    }
                    std::sort(sub_folders.begin(), sub_folders.end(), [](boost::filesystem::path const& lhs, boost::filesystem::path const& rhs) { return lhs.wstring().length() > rhs.wstring().length(); });
                    foreach(boost::filesystem::path folder, sub_folders){
                        SetFileAttributesW(folder.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                        try{
                            boost::filesystem::remove_all(folder);
                        }
                        catch (...){
                            LOG(LOG_LEVEL_ERROR, L"failed to remove %s.", excluded.wstring().c_str());
                            std::wcout << L"failed to remove " << excluded.wstring() << std::endl;
                        }
                    }
                }
                else{
                    LOG(LOG_LEVEL_WARNING, _T("Remove excluded file (%s)."), p.wstring().c_str());
                    SetFileAttributesW(p.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
                    try{
                        boost::filesystem::remove(p);
                    }
                    catch (...){
                        LOG(LOG_LEVEL_ERROR, L"failed to remove %s.", excluded.wstring().c_str());
                        std::wcout << L"failed to remove " << excluded.wstring() << std::endl;
                    }
                }
            }
        }
        SetFileAttributesW(excluded.wstring().c_str(), FILE_ATTRIBUTE_NORMAL);
        try{
            boost::filesystem::remove(excluded);
        }
        catch (...){
            LOG(LOG_LEVEL_ERROR, L"failed to remove %s.", excluded.wstring().c_str());
            std::wcout << L"failed to remove " << excluded.wstring() << std::endl;
        }
    }
}

int _tmain(int argc, _TCHAR* argv[])
{
    po::variables_map vm;
    boost::filesystem::path logfile, config, batch;
    std::wstring app = macho::windows::environment::get_execution_full_path();
    logfile = macho::windows::environment::get_execution_full_path() + L".log";
    config = boost::filesystem::path(macho::windows::environment::get_execution_full_path()).parent_path() / L"agent.cfg";
    batch = boost::filesystem::path(macho::windows::environment::get_execution_full_path()).parent_path() / L"run.cmd";
    macho::set_log_file(logfile.wstring());
    macho::windows::com_init com;

    if (!boost::filesystem::exists(config)){
        LOG(LOG_LEVEL_ERROR, L"%s does not exist.", config.c_str());
#if _DEBUG
        partition_path::vtr parts;
        macho::windows::storage::ptr stg = macho::windows::storage::get();
        if (stg){
            macho::windows::storage::partition::vtr partitions = stg->get_partitions();
            foreach(macho::windows::storage::partition::ptr p, partitions){
                partition_path::ptr _p(new partition_path());
                _p->path = p->drive_letter();
                _p->partition_number = p->partition_number();
                macho::windows::storage::disk::ptr d = stg->get_disk(p->disk_number());
                if (d){
                    _p->guid = d->guid();
                    _p->signature = d->signature();
                    _p->partition_style = d->partition_style();
                    _p->partition_size = p->size();
                    _p->partition_offset = p->offset();
                    _p->is_system_disk = d->is_system();
                    parts.push_back(_p);
                }
            }
            partition_path::save(config, parts);
        }
#endif
    }
    else if (command_line_parser(vm)){
        mutex m(L"irm_agent");
        if (m.trylock()){
            macho::set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());
            LOG(LOG_LEVEL_RECORD, L"%s running...", app.c_str());
            if ((!vm.count("boot")) && vm["time"].as<int>()){
                std::wcout << L"Waitting (" << vm["time"].as<int>() << L") seconds..." << std::endl;
                LOG(LOG_LEVEL_RECORD, L"Waitting (%d) seconds...", vm["time"].as<int>());
                boost::this_thread::sleep(boost::posix_time::seconds(vm["time"].as<int>()));
            }

            macho::windows::task_scheduler::ptr scheduler = macho::windows::task_scheduler::get();
            if (scheduler){		
                if (vm.count("boot")){
                    macho::windows::task_scheduler::task::ptr t = scheduler->get_task(L"machine_wake_up_agent");
                    if (t){
                        if (scheduler->delete_task(L"machine_wake_up_agent")){
                            LOG(LOG_LEVEL_RECORD, L"succeeded to remove the old \"machine_wake_up_agent\" task.");
                            std::wcout << L"succeeded to remove the old \"machine_wake_up_agent\" task." << std::endl;
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, L"failed to remove the old \"machine_wake_up_agent\" task.");
                            std::wcout << L"failed to remove the old \"machine_wake_up_agent\" task." << std::endl;
                        }
                    }
                    std::wstring parameters;
                    parameters.append(stringutils::format(L"-t %d", vm["time"].as<int>()));
                    parameters.append(stringutils::format(L" -l %d", vm["level"].as<int>()));

                    t = scheduler->create_task(L"machine_wake_up_agent", app, parameters);
                    if (t){
                        LOG(LOG_LEVEL_RECORD, L"succeeded to create a new \"machine_wake_up_agent\" task (%s %s).", app.c_str(), parameters.c_str());
                        std::wcout << L"succeeded to create a new \"machine_wake_up_agent\" task (" << app << L" " << parameters << L")." << std::endl;
                    }
                    else{
                        LOG(LOG_LEVEL_ERROR, L"failed to create a new \"machine_wake_up_agent\" task (%s %s).", app.c_str(), parameters.c_str());
                        std::wcout << L"failed to create a new \"machine_wake_up_agent\" task (" << app << L" " << parameters << L")." << std::endl;
                    }
                    scheduler->run_task(L"machine_wake_up_agent");
                    return 0;
                }
            }
            partition_path::vtr parts = partition_path::load(config);
            while (parts.size()){
                macho::windows::storage::ptr stg = macho::windows::storage::get();
                if (stg){
                    macho::windows::storage::disk::vtr disks = stg->get_disks();
                    std::map<int, macho::windows::storage::disk::ptr > disks_map;
                    if (disks.size() > 1){
                        foreach(macho::windows::storage::disk::ptr d, disks){
                            if (d->is_system())
                                continue;	
                            partition_path::vtr::iterator p = parts.begin();
                            while (p != parts.end()){
                                bool done = false;
                                if ((*p)->partition_style == d->partition_style()){
                                    bool matched = false;
                                    switch (d->partition_style()){
                                    case macho::windows::storage::ST_PST_MBR:
                                        if ((*p)->guid.empty()){
                                            matched = (int)d->signature() == (int)(*p)->signature;
                                            if (!matched)
                                                LOG(LOG_LEVEL_ERROR, L"Does not match (%d,%d)", (int)d->signature(), (int)(*p)->signature);
                                        }
                                        break;
                                    case macho::windows::storage::ST_PST_GPT:
                                        if (!(*p)->guid.empty()){
                                            matched = macho::guid_(d->guid()) == macho::guid_((*p)->guid);
                                            if (!matched)
                                                LOG(LOG_LEVEL_ERROR, L"Does not match (%s,%s)", (*p)->guid.c_str(), d->guid().c_str());
                                        }
                                        break;
                                    }									
                                    if (matched){
                                        d->online();
                                        d->clear_read_only_flag();
                                        disks_map[d->number()] = d;
                                        macho::windows::storage::partition::vtr partitions = d->get_partitions();
                                        macho::windows::storage::volume::vtr volumes = d->get_volumes();
                                        foreach(macho::windows::storage::volume::ptr v, volumes){
                                            remove_excluded_paths(v);
                                        }
                                        foreach(macho::windows::storage::partition::ptr _p, partitions){
                                            if (/*_p->partition_number() == (*p)->partition_number &&*/
                                                _p->size() == (uint64_t)(*p)->partition_size &&
                                                _p->offset() == (uint64_t)(*p)->partition_offset){
                                                string_array paths = _p->access_paths();
                                                foreach(std::wstring &path, paths){
                                                    if (std::wstring::npos != path.find(L"\\\\?\\Volume{")){
                                                        foreach(macho::windows::storage::volume::ptr& v, volumes){
                                                            if (std::wstring::npos != path.find(v->id())){
                                                                v->mount();
                                                                if (!v->drive_letter().empty()){
                                                                    std::wstring drive_letter = v->drive_letter().append(L":\\");
                                                                    if (DeleteVolumeMountPoint(drive_letter.c_str())){
                                                                        LOG(LOG_LEVEL_RECORD, L"DeleteVolumeMountPoint(%s).", drive_letter.c_str());
                                                                    }
                                                                    else{
                                                                        LOG(LOG_LEVEL_ERROR, L"Failed to DeleteVolumeMountPoint(%s). 0x%08X", drive_letter.c_str(), GetLastError());
                                                                    }
                                                                }
                                                                if (!(*p)->path.empty()){
                                                                    if (boost::filesystem::exists((*p)->path)){
                                                                        LOG(LOG_LEVEL_RECORD, L"The path (%s) is already existed.", (*p)->path.c_str());
                                                                        done = true;
                                                                    }
                                                                    else{
                                                                        std::wstring volume_name = boost::str(boost::wformat(L"\\\\?\\Volume{%s}\\") % v->id());
                                                                        if (SetVolumeMountPoint((*p)->path.c_str(), volume_name.c_str())){
                                                                            LOG(LOG_LEVEL_RECORD, L"SetVolumeMountPoint(%s,%s).", (*p)->path.c_str(), volume_name.c_str());
                                                                            done = true;
                                                                        }
                                                                        else{
                                                                            LOG(LOG_LEVEL_ERROR, L"Failed to SetVolumeMountPoint(%s,%s). 0x%08X", (*p)->path.c_str(), volume_name.c_str(), GetLastError());
                                                                        }
                                                                    }
                                                                }
                                                                else{
                                                                    done = true;
                                                                }
                                                                break;
                                                            }
                                                        }												
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                if (done)
                                    p = parts.erase(p);
                                else
                                    p++;
                            }
                        }

                        foreach(macho::windows::storage::disk::ptr d, disks){
                            if (d->is_system())
                                continue;
                            if (disks_map.count(d->number()))
                                continue;
                            partition_path::vtr::iterator p = parts.begin();
                            while (p != parts.end()){
                                bool done = false;
                                if ((*p)->is_system_disk && (*p)->partition_style == d->partition_style()){
                                    macho::windows::storage::partition::vtr partitions = d->get_partitions();
                                    macho::windows::storage::volume::vtr volumes = d->get_volumes();
                                    foreach(macho::windows::storage::partition::ptr _p, partitions){
                                        if (/*_p->partition_number() == (*p)->partition_number &&*/
                                            _p->size() == (uint64_t)(*p)->partition_size &&
                                            _p->offset() == (uint64_t)(*p)->partition_offset ){
                                            LOG(LOG_LEVEL_RECORD, L"Match partition(%d, %I64u,%I64u)", (*p)->partition_number, (*p)->partition_offset, (*p)->partition_size);
                                            d->online();
                                            d->clear_read_only_flag();
                                            disks_map[d->number()] = d;
                                            string_array paths = _p->access_paths();
                                            foreach(std::wstring &path, paths){
                                                if (std::wstring::npos != path.find(L"\\\\?\\Volume{")){
                                                    foreach(macho::windows::storage::volume::ptr& v, volumes){
                                                        if (std::wstring::npos != path.find(v->id())){
                                                            v->mount();
                                                            if (temp_drive_letter::is_drive_letter((*p)->path) && !v->drive_letter().empty()){
                                                                std::wstring drive_letter = v->drive_letter().append(L":\\");
                                                                if (DeleteVolumeMountPoint(drive_letter.c_str())){
                                                                    LOG(LOG_LEVEL_RECORD, L"DeleteVolumeMountPoint(%s).", drive_letter.c_str());
                                                                }
                                                                else{
                                                                    LOG(LOG_LEVEL_ERROR, L"Failed to DeleteVolumeMountPoint(%s). 0x%08X", drive_letter.c_str(), GetLastError());
                                                                }
                                                            }
                                                            std::wstring volume_name = boost::str(boost::wformat(L"\\\\?\\Volume{%s}\\") % v->id());
                                                            if (SetVolumeMountPoint((*p)->path.c_str(), volume_name.c_str())){
                                                                LOG(LOG_LEVEL_RECORD, L"SetVolumeMountPoint(%s,%s).", (*p)->path.c_str(), volume_name.c_str());
                                                                done = true;
                                                            }
                                                            else{
                                                                LOG(LOG_LEVEL_ERROR, L"Failed to SetVolumeMountPoint(%s,%s). 0x%08X", (*p)->path.c_str(), volume_name.c_str(), GetLastError());
                                                            }
                                                            break;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        else{
                                            LOG(LOG_LEVEL_DEBUG, L"Failed to match partition(%d, %I64u,%I64u)", (*p)->partition_number, (*p)->partition_offset, (*p)->partition_size);
                                        }
                                    }
                                }
                                if (done)
                                    p = parts.erase(p);
                                else
                                    p++;
                            }
                        }
                    }
                }
                boost::this_thread::sleep(boost::posix_time::seconds(15));
            }
            if (boost::filesystem::exists(batch)){
                std::wstring ret;
                LOG(LOG_LEVEL_RECORD, L"Command: %s", batch.wstring().c_str());
                bool result = process::exec_console_application_with_timeout(batch.wstring(), ret);
                LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
            }
            if (vm.count("terminate")){
                scheduler = macho::windows::task_scheduler::get();
                if (scheduler){
                    macho::windows::task_scheduler::task::ptr t = scheduler->get_task(L"machine_wake_up_agent");
                    if (t){
                        if (scheduler->delete_task(L"machine_wake_up_agent")){
                            LOG(LOG_LEVEL_RECORD, L"succeeded to remove the old \"machine_wake_up_agent\" task.");
                            std::wcout << L"succeeded to remove the old \"machine_wake_up_agent\" task." << std::endl;
                        }
                        else{
                            LOG(LOG_LEVEL_ERROR, L"failed to remove the old \"machine_wake_up_agent\" task.");
                            std::wcout << L"failed to remove the old \"machine_wake_up_agent\" task." << std::endl;
                        }
                    }
                }
            }
            LOG(LOG_LEVEL_RECORD, L"Leave....");
            m.unlock();
        }
    }
    return 0;
}


// notice.cpp : Defines the entry point for the console application.
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
#include "http_client.h"

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
    po::options_description command("Command");
    command.add_options()
        ("url,u", po::wvalue<std::wstring>(), "Notify Url")
        ;

    po::options_description general("General");
    general.add_options()
        ("boot,b", "run command on next windows boot up")
        ("time,t", po::value<int>()->default_value(0, "0"), "wait seconds before notice")
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;

    po::options_description all("Allowed options");
    all.add(general).add(command);

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
            if (vm.count("url"))
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

int _tmain(int argc, _TCHAR* argv[])
{
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

        macho::windows::task_scheduler::ptr scheduler = macho::windows::task_scheduler::get();
        if (scheduler){
            macho::windows::task_scheduler::task::ptr t = scheduler->get_task(L"machine_wake_up_notice");
            if (t){
                if (scheduler->delete_task(L"machine_wake_up_notice")){
                    LOG(LOG_LEVEL_RECORD, L"succeeded to remove the old \"machine_wake_up_notice\" task.");
                    std::wcout << L"succeeded to remove the old \"machine_wake_up_notice\" task." << std::endl;
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"failed to remove the old \"machine_wake_up_notice\" task.");
                    std::wcout << L"failed to remove the old \"machine_wake_up_notice\" task." << std::endl;
                }
            }
            if (vm.count("boot")){
                std::wstring parameters;
                parameters.append(stringutils::format(L"-t %d", vm["time"].as<int>()));
                parameters.append(stringutils::format(L" -l %d", vm["level"].as<int>()));
                parameters.append(stringutils::format(L" -u %s", vm["url"].as<std::wstring>().c_str()));

                t = scheduler->create_task(L"machine_wake_up_notice", app, parameters);
                if (t){
                    LOG(LOG_LEVEL_RECORD, L"succeeded to create a new \"machine_wake_up_notice\" task (%s %s).", app.c_str(), parameters.c_str());
                    std::wcout << L"succeeded to create a new \"machine_wake_up_notice\" task (" << app << L" " << parameters << L")." << std::endl;
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"failed to create a new \"machine_wake_up_notice\" task (%s %s).", app.c_str(), parameters.c_str());
                    std::wcout << L"failed to create a new \"machine_wake_up_notice\" task (" << app << L" " << parameters << L")." << std::endl;
                }
                return 0;
            }
        }

        std::wstring ret;
        long      response;
        std::wstring url = vm["url"].as<std::wstring>();
        http_client client(url.find(L"https://") != std::wstring::npos);
        client.get(url, L"", L"", response, ret);
        std::wstring message = macho::stringutils::format(_T("sent notify url (%s) and return code is %d and message is %s"), url.c_str(), response, ret.c_str());
        LOG(LOG_LEVEL_RECORD, L"%s", message.c_str() );
        std::wcout << message << std::endl;
    }
	return 0;
}


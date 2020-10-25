// uninstall.cpp : Defines the entry point for the console application.
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
        ("name,n", po::wvalue<std::wstring>(), "application display name for uninstall")
        ("timeout,t", po::value<int>()->default_value(300, "300"), "wait seconds before timeout.")
        ("runonce,r", "add run once reg command for uninstallation (option)")
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
        if (vm.count("help")){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("name"))
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
    macho::windows::com_init com;
    if (command_line_parser(vm)){
        std::wstring app = macho::windows::environment::get_execution_full_path();
        std::wcout << app << L" running..." << std::endl;
        std::wstring app_display_name = vm["name"].as<std::wstring>();
        macho::stringutils::tolower(app_display_name);
        registry reg;
        if (reg.open(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall")){
            reg.refresh_subkeys();
            int subkey_count = reg.subkeys_count();
            for (int i = 0; i < subkey_count; i++){
                if (reg.subkey(i)[L"WindowsInstaller"].exists() && ((DWORD)reg.subkey(i)[L"WindowsInstaller"]) > 0){
                    std::wstring display_name = reg.subkey(i)[L"DisplayName"].wstring();
                    if (display_name.length() && macho::stringutils::tolower(display_name) == app_display_name){
                        std::wstring result;
                        std::wstring cmd = boost::str(boost::wformat(L"%s\\msiexec.exe /X%s /quiet /norestart") % environment::get_system_directory() % reg.subkey(i).key_name());
                        std::wstring runonce_cmd = boost::str(boost::wformat(L"%s\\msiexec.exe /X%s /passive /norestart") % environment::get_system_directory() % reg.subkey(i).key_name());
                        std::wcout << L"Uninstall" << reg.subkey(i)[L"DisplayName"].wstring() << L" : " << cmd << std::endl;
                        process::exec_console_application_with_timeout(cmd, result, vm["timeout"].as<int>(), true);
                        std::wcout << L"Result : " << result << std::endl;
                        if (vm.count("runonce")){
                            if (reg.open(_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"))){
                                std::wstring run_ouce_key_name = boost::str(boost::wformat(L"!%s") % reg.subkey(i).key_name());
                                reg[run_ouce_key_name] = runonce_cmd;
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
    return 0;
}



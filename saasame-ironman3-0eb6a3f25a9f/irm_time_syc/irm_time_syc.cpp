// irm_rescan_cli.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common\ntp_client.hpp"
#include <string>
#include <vector>
#include <Windows.h>
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
namespace po = boost::program_options;

bool command_line_parser(po::variables_map &vm){
    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    //guid_ g = guid_::create();
    po::options_description input("Input");
    input.add_options()
        ("ntp,n", po::value<std::string>(), "NTP server")
        ;
    po::options_description general("General");
    general.add_options()
        ("help,h", "produce help message (option)");
    ;
    po::options_description all("Allowed options");
    all.add(input).add(general);

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
            result = true;
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

int _tmain(int argc, _TCHAR* argv[])
{
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    std::vector<std::string> time_servers;
    po::variables_map vm;
    command_line_parser(vm);
    if (vm.count("ntp")){
        time_servers.push_back(vm["ntp"].as<std::string>());
    }
    else{       
        time_servers.push_back("0.cn.pool.ntp.org");
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
        time_servers.push_back("1.cn.pool.ntp.org");
        time_servers.push_back("2.cn.pool.ntp.org");
        time_servers.push_back("3.cn.pool.ntp.org");
    }
    foreach ( std::string var, time_servers ){
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
            
            printf("Succeed to get time from time server(%s). UTC time (%s)\n", var.c_str(), boost::posix_time::to_simple_string(pt).c_str() );

            if (SetSystemTime(&st)){
                printf("Succeed to set system time.\n");
                break;
            }
        }
        else{
            printf("Failed to get time from time server(%s).\n", var.c_str());
        }
    }
        
    return 0;
}


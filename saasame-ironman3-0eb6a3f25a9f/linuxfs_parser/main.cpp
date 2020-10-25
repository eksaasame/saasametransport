// linuxfs_parser.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "macho.h"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include "linuxfs_parser.h"

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
        ("number,n", po::value<int>()->default_value(2, "2"), "disk number")
        ("target,t", po::value<int>()->default_value(3, "3"), "target disk number")
        ;

    po::options_description general("General");
    general.add_options()
        ("level,l", po::value<int>()->default_value(5, "5"), "log level ( 0 ~ 5 )")
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
            if (vm.count("number"))
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
    macho::windows::com_init com;
    po::variables_map vm;
    DWORD dwRet = 0;
    boost::filesystem::path exepath = macho::windows::environment::get_execution_full_path();
    std::wstring working_dir = macho::windows::environment::get_working_directory();
    boost::filesystem::path logfile = macho::windows::environment::get_execution_full_path() + L".log";
    macho::set_log_file(logfile.wstring());
    macho::set_log_level(LOG_LEVEL_DEBUG);
    if (command_line_parser(vm)){
        macho::set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());
        LOG(LOG_LEVEL_RECORD, L"%s running...", exepath.wstring().c_str());
        storage::ptr stg = storage::get();
        if (!stg){
            LOG(LOG_LEVEL_ERROR, L"Can't open storage management interface");
        }
        else{
            std::vector<universal_disk_rw::ptr> rws;
            for (int i = 4; i <= 6; i++){
                rws.push_back(general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % i)));
            }
            linuxfs::io_range::map ranges = linuxfs::volume::get_file_system_ranges(rws);
            if (ranges.size()){

            }
            //DWORD disk_number = vm["number"].as<int>();
            //storage::disk::ptr d = stg->get_disk(disk_number);
            //if (!d){
            //    LOG(LOG_LEVEL_ERROR, L"Can't find the disk (%d)", disk_number);
            //}
            //else{
            //    
            //    std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number);
            //    universal_disk_rw::ptr io = general_io_rw::open(path);
            //    if (!io){
            //        LOG(LOG_LEVEL_ERROR, L"Can't open the device (%s)", path.c_str());
            //    }
            //    else{
            //       /* std::vector<universal_disk_rw::ptr> rws;
            //        rws.push_back(io);
            //        linuxfs::io_range::map ranges = linuxfs::volume::get_file_system_ranges(rws);
            //        ranges.size();*/
            //        /*path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % 5);
            //        universal_disk_rw::ptr io2 = general_io_rw::open(path);
            //        size_t s = 1024 * 1024;
            //        std::auto_ptr<BYTE> buff = std::auto_ptr<BYTE>(new BYTE[s]);
            //        std::auto_ptr<BYTE> buff2 = std::auto_ptr<BYTE>(new BYTE[s]);

            //        uint64_t offset = ranges.begin()->second.at(0).start;
            //        linuxfs::io_range::vtr rr;
            //        while (offset < d->size()){
            //            memset(buff.get(), 0, s);
            //            memset(buff2.get(), 0, s);
            //            uint32_t r = 0;
            //            io->read(offset, s, buff.get(), r);
            //            io2->read(offset, s, buff2.get(), r);
            //            if (memcmp(buff.get(), buff2.get(), s))
            //                rr.push_back(linuxfs::io_range(offset, s));
            //            offset += s;
            //        }
            //        rr.size();*/

            //        linuxfs::volume::vtr volumes = linuxfs::volume::get(io);
            //        foreach(linuxfs::volume::ptr v, volumes){
            //            LOG(LOG_LEVEL_INFO, L"linuxfs volume: offset(%I64u), length(%I64u)", v->start(), v->length());

            //            linuxfs::io_range::vtr changes;
            //            linuxfs::io_range::vtr ranges = v->file_system_ranges();
            //        }

            //        ULONG             full_mini_block_size = 8 * 1024 * 1024;
            //        std::auto_ptr<BYTE> buff = std::auto_ptr<BYTE>(new BYTE[full_mini_block_size]);
            //        universal_disk_rw::ptr target = general_io_rw::open(boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % vm["target"].as<int>()), false);
            //        memset(buff.get(), 0, full_mini_block_size);
            //        {
            //            uint32_t read_bytes = 0;
            //            uint32_t write_bytes = 0;
            //            uint64_t start = 0;
            //            uint64_t end = full_mini_block_size;
            //            while (start < end){
            //                uint64_t length = end - start;
            //                if (length > full_mini_block_size)
            //                    length = full_mini_block_size;
            //                if (io->read(start, length, buff.get(), read_bytes))
            //                    target->write(start, buff.get(), length, write_bytes);
            //                start += length;
            //            }
            //        }

            //        foreach(linuxfs::volume::ptr v, volumes){
            //            LOG(LOG_LEVEL_INFO, L"linuxfs volume: offset(%I64u), length(%I64u)", v->start(), v->length());
            //            
            //            linuxfs::io_range::vtr changes;
            //            linuxfs::io_range::vtr ranges = v->file_system_ranges();
            //            foreach(linuxfs::io_range &r, ranges){
            //                uint64_t start_offset = 0;
            //                uint64_t start = r.start;
            //                uint64_t end = r.start + r.length;
            //                while (end > start_offset && start < end){
            //                    uint64_t length = end - start;
            //                    if (length > full_mini_block_size)
            //                        length = full_mini_block_size;
            //                    linuxfs::io_range changed(start, length);
            //                    changes.push_back(changed);
            //                    start += length;
            //                }
            //            }
            //            uint32_t read_bytes = 0;
            //            uint32_t write_bytes = 0;
            //            foreach(linuxfs::io_range &w, changes){
            //                memset(buff.get(), 0, full_mini_block_size);
            //                if (io->read(w.start, w.length, buff.get(), read_bytes)){
            //                    target->write(w.start, buff.get(), w.length, write_bytes);
            //                }
            //            }
            //        }
            //    }
            //}
        }
    }

    return 0;
}
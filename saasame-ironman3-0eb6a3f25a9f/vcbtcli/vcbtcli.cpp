// vcbtcli.cpp : Defines the entry point for the console application.
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
#include "..\vcbt\vcbt\journal.h"
#include "Ntddscsi.h"

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

    po::options_description command("Commands");
    command.add_options()
        ("check,c", "Check CBT")
        ("enable,e", "Enable CBT")
        ("disable,d", "Disable CBT")
        ("snapshot,s", "Snapshot CBT")
        ("post,p", "Post Snapshot")
        ("undo,u", "Undo Snapshot")
        ("runtime,r", "DUMP CBT Runtime Info")
        ("journal,j", "DUMP Journal File location info")
        ("mb,m", po::value<int>()->default_value(0, "0"), "journal file size (MB).")
        ;
    po::options_description target("Target");
    target.add_options()
        ("volume,v", po::wvalue<std::wstring>(), "volume path")
        ("drive", po::wvalue<std::wstring>(), "drive letter")
        ("disk,n", po::value<int>(), "disk number")
        ("all,a", "all disk volumes")
        ;
    po::options_description general("General");
    general.add_options()
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
            if (vm.count("volume") || vm.count("drive") || vm.count("disk") || vm.count("all")){
                if (vm.count("enable") || vm.count("disable") || vm.count("snapshot") || vm.count("post") || vm.count("undo") || vm.count("runtime") || vm.count("check")){
                    result = true;
                }
                else{
                    std::cout << title << command << std::endl;
                }
            }               
            else
                std::cout << title << target << std::endl;
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

BOOL SendDeviceIoControl(std::wstring path, DWORD ioctl, LPVOID input, LONG size_of_input, LPVOID out, DWORD& size_of_out)
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
    BOOL bResult = FALSE;                 // results flag
    DWORD err = ERROR_SUCCESS;
    hDevice = CreateFileW(path.c_str(),          // drive to open
        0,                // no access to the drive
        FILE_SHARE_READ | // share mode
        FILE_SHARE_WRITE,
        NULL,             // default security attributes
        OPEN_EXISTING,    // disposition
        0,                // file attributes
        NULL);            // do not copy file attributes

    if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
    {
        LOG(LOG_LEVEL_ERROR, L"Failed to CreateFileW(%s), Error(0x%08x).", path.c_str(), GetLastError());
        return (FALSE);
    }

    if (!(bResult = DeviceIoControl(hDevice,                       // device to be queried
        ioctl,                        // operation to perform
        input, size_of_input,                      // input buffer
        out, size_of_out,             // output buffer
        &size_of_out,                   // # bytes returned
        (LPOVERLAPPED)NULL)))          // synchronous I/O
    {
        err = GetLastError();
        LOG(LOG_LEVEL_ERROR, L"Failed to DeviceIoControl(%s, 0x%08x), Error(0x%08x).", path.c_str(), ioctl, err);            
    }

    CloseHandle(hDevice);
    SetLastError(err);
    return (bResult);
}

BOOL SendDeviceIoControl(HANDLE hDevice, DWORD ioctl, LPVOID input, LONG size_of_input, LPVOID out, DWORD& size_of_out){
    BOOL bResult = FALSE;                 // results flag
    DWORD err = ERROR_SUCCESS;
    if (!(bResult = DeviceIoControl(hDevice,                       // device to be queried
        ioctl,                        // operation to perform
        input, size_of_input,                      // input buffer
        out, size_of_out,             // output buffer
        &size_of_out,                   // # bytes returned
        (LPOVERLAPPED)NULL)))          // synchronous I/O
    {
        err = GetLastError();
        LOG(LOG_LEVEL_ERROR, L"Failed to DeviceIoControl(0x%08x), Error(0x%08x).", ioctl, err);
    }
    SetLastError(err);
    return (bResult);
}

std::vector<macho::guid_> get_guids(po::variables_map& vm){
    std::vector<macho::guid_> guids;
    macho::windows::storage::ptr stg = macho::windows::storage::get();
    if (vm.count("volume")){
        macho::windows::storage::volume::vtr volumes = stg->get_volumes();
        std::wstring mount_point = vm["volume"].as<std::wstring>();
        if ((5 > mount_point.length()) ||
            (mount_point[0] != _T('\\') ||
            mount_point[1] != _T('\\') ||
            mount_point[2] != _T('?') ||
            mount_point[3] != _T('\\'))){
            LOG(LOG_LEVEL_ERROR, L"Invalid volume path %s.", mount_point.c_str());
        }
        else{
            if (mount_point[mount_point.length() - 1] == _T('\\'))
                mount_point.erase(mount_point.length() - 1);
            macho::windows::storage::volume::ptr _v;
            foreach(macho::windows::storage::volume::ptr v, volumes){
                foreach(std::wstring path, v->access_paths()){
                    if (wcsstr(path.c_str(), mount_point.c_str())){
                        _v = v; 
                        break;
                    }
                }
            }
            if (_v) guids.push_back(_v->id());
        }
    }
    else if (vm.count("drive")){
        std::wstring mount_point = vm["drive"].as<std::wstring>();
        stringutils::toupper(mount_point);
        if ((0 < mount_point.length()) &&
            (IsCharAlpha(mount_point[0]))){
            macho::windows::storage::volume::vtr volumes = stg->get_volumes();
            macho::windows::storage::volume::ptr _v;
            foreach(macho::windows::storage::volume::ptr v, volumes){
                if (!v->drive_letter().empty()){
                    std::wstring path = v->drive_letter();
                    stringutils::toupper(path);
                    if (path[0] == mount_point[0]){
                        _v = v;
                        break;
                    }
                }
            }
            if (_v) guids.push_back(_v->id());
        }
        else{
            LOG(LOG_LEVEL_ERROR, L"Invalid drive letter %s.", mount_point.c_str());
        }
    }
    else if (vm.count("disk")){
        macho::windows::storage::volume::vtr volumes = stg->get_volumes(vm["disk"].as<int>());
        foreach(macho::windows::storage::volume::ptr v, volumes){
            guids.push_back(v->id());
        }
    }
    else if (vm.count("all")){
        macho::windows::storage::ptr stg = macho::windows::storage::get();
        macho::windows::storage::disk::vtr disks = stg->get_disks();
        foreach(macho::windows::storage::disk::ptr d, disks){
            macho::windows::storage::volume::vtr volumes = d->get_volumes();
            foreach(macho::windows::storage::volume::ptr v, volumes){
                if (v->access_paths().size()){
                    guids.push_back(v->id());
                }
            }
        }
    }
    if (guids.size() == 0)
        LOG(LOG_LEVEL_ERROR, L"Invalid target path.");
    return guids;
}


DWORD get_command(po::variables_map& vm){
    DWORD cmd = 0;
    if (vm.count("check"))
        cmd = IOCTL_VCBT_IS_ENABLE;
    else if (vm.count("enable"))
        cmd = IOCTL_VCBT_ENABLE;
    else if( vm.count("disable") )
        cmd = IOCTL_VCBT_DISABLE;
    else if (vm.count("snapshot"))
        cmd = IOCTL_VCBT_SNAPSHOT;
    else if ( vm.count("post") )
        cmd = IOCTL_VCBT_POST_SNAPSHOT;
    else if ( vm.count("undo"))
        cmd = IOCTL_VCBT_UNDO_SNAPSHOT;
    else if (vm.count("runtime"))
        cmd = IOCTL_VCBT_RUNTIME;
    else {
        LOG(LOG_LEVEL_ERROR, L"Invalid Command.");
    }
    return cmd;
}

int _tmain(int argc, _TCHAR* argv[])
{
    int size = sizeof(VCBT_RECORD);

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
        std::vector<macho::guid_> guids = get_guids(vm);
        DWORD                     ioctl = get_command(vm);
        LOG(LOG_LEVEL_RECORD, L"Command : 0x%08x", ioctl);
        HANDLE hDevice = INVALID_HANDLE_VALUE;  // handle to the drive to be examined 
        BOOL bResult = FALSE;                 // results flag
        DWORD err = ERROR_SUCCESS;
        hDevice = CreateFileW(VCBT_WIN32_DEVICE_NAME,          // drive to open
            0,                // no access to the drive
            FILE_SHARE_READ | // share mode
            FILE_SHARE_WRITE,
            NULL,             // default security attributes
            OPEN_EXISTING,    // disposition
            0,                // file attributes
            NULL);            // do not copy file attributes

        if (hDevice == INVALID_HANDLE_VALUE)    // cannot open the drive
        {
            LOG(LOG_LEVEL_ERROR, L"Failed to CreateFileW(%s), Error(0x%08x).", VCBT_WIN32_DEVICE_NAME, GetLastError());
            return (FALSE);
        }
        else{
            if (IOCTL_VCBT_RUNTIME == ioctl){
                macho::windows::storage::ptr stg = macho::windows::storage::get();
                macho::windows::storage::volume::vtr volumes = stg->get_volumes();
           
                for (int i = 0; i < guids.size(); i++){
                    macho::windows::storage::volume::ptr _v;
                    foreach(macho::windows::storage::volume::ptr v, volumes){
                        if (macho::guid_(v->id()) == guids[i]){
                            _v = v;
                            break;
                        }
                    }

                    VCBT_RUNTIME_COMMAND cmd;
                    DWORD output_length = sizeof(VCBT_RUNTIME_RESULT);
                    std::auto_ptr<VCBT_RUNTIME_RESULT> result = std::auto_ptr<VCBT_RUNTIME_RESULT>((PVCBT_RUNTIME_RESULT)new BYTE[output_length]);
                    cmd.VolumeId = guids[i];
                    cmd.Flag = JOURNAL;
                    BOOL   res = FALSE;
                    std::cout << "=======================================================================" << std::endl << std::endl;
                    std::cout << "[Volume ID] : " << (std::string)macho::guid_(cmd.VolumeId) << std::endl;
                    std::wcout << L"Drive Letter : " << _v->drive_letter() << std::endl;
                    if (_v->access_paths().size()){
                        std::wcout << L"Access Paths  : " << std::endl;
                        foreach(std::wstring path, _v->access_paths())
                            std::wcout << L"  " << path << std::endl;
                        std::cout << std::endl;
                    }
                    LOG(LOG_LEVEL_RECORD, L"Volume Id: %s", macho::guid_(cmd.VolumeId).wstring().c_str());
                    while (!(res = SendDeviceIoControl(hDevice, ioctl, &cmd, sizeof(VCBT_RUNTIME_COMMAND), (LPVOID)result.get(), output_length)) &&
                        ERROR_INSUFFICIENT_BUFFER == GetLastError()){
                        output_length += sizeof(VCBT_RUNTIME_RESULT);
                        result = std::auto_ptr<VCBT_RUNTIME_RESULT>((PVCBT_RUNTIME_RESULT)new BYTE[output_length]);
                    }
                    if (res){
                        std::cout << "CBT Initialized       : " << (result->Initialized ? "true" : "false") << std::endl;
                        std::cout << "CBT Ready             : " << (result->Ready ? "true" : "false") << std::endl;
                        std::cout << "CBT Check             : " << (result->Check ? "true" : "false") << std::endl;
                        std::cout << "CBT UMap Resolution   : " << result->Resolution << std::endl;
                        std::cout << "Journal Id            : " << result->JournalMetaData.Block.j.JournalId << std::endl;
                        ULONG size = result->JournalMetaData.Block.j.Size >> 20;
                        std::cout << "Journal size          : " << size << " MBs" << std::endl;
                        std::cout << "Journal First Key     : " << result->JournalMetaData.FirstKey << std::endl;
                        std::cout << "Journal Latest Key    : " << result->JournalMetaData.Block.r.Key << std::endl;
                        std::cout << "Journal Latest Start  : " << result->JournalMetaData.Block.r.Start << std::endl;
                        std::cout << "Journal Latest Length : " << result->JournalMetaData.Block.r.Length << std::endl;
                        std::cout << "Journal View Size     : " << result->JournalViewSize << std::endl;
                        std::cout << "Journal View Offset   : " << result->JournalViewOffset << std::endl;
                        std::cout << "Disk Bytes Per Sector : " << result->BytesPerSector << std::endl;
                        std::cout << "Volume File Area Offset : " << result->FileAreaOffset << std::endl;
                        std::cout << "File System Cluster Size : " << result->FsClusterSize << std::endl << std::endl;
                        if (vm.count("journal")){
                            std::cout << "Runtime Journal File Locations : " << std::endl;
                            std::cout << std::setw(21) << std::right << "Start";
                            std::cout << std::setw(21) << std::right << "Length";
                            std::cout << std::setw(21) << std::right << "Lcn" << std::endl;

                            LARGE_INTEGER               liVcnPrev = result->RetrievalPointers.StartingVcn;
                            for (DWORD extent = 0; extent < result->RetrievalPointers.ExtentCount; extent++){
                                LONGLONG _Start = liVcnPrev.QuadPart * result->FsClusterSize;
                                LONG     _Length = (ULONG)(result->RetrievalPointers.Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * result->FsClusterSize;
                                LONGLONG _Lcn = result->RetrievalPointers.Extents[extent].Lcn.QuadPart * result->FsClusterSize;
                                std::cout << std::setw(21) << std::right << _Start;
                                std::cout << std::setw(21) << std::right << _Length;
                                std::cout << std::setw(21) << std::right << _Lcn << std::endl;
                                liVcnPrev = result->RetrievalPointers.Extents[extent].NextVcn;
                            }
                        }
                    }
                    if (res){
                        if (vm.count("journal")){
                            cmd.Flag = JOURNAL_FILE;
                            while (!(res = SendDeviceIoControl(hDevice, ioctl, &cmd, sizeof(VCBT_RUNTIME_COMMAND), (LPVOID)result.get(), output_length)) &&
                                ERROR_INSUFFICIENT_BUFFER == GetLastError()){
                                output_length += sizeof(VCBT_RUNTIME_RESULT);
                                result = std::auto_ptr<VCBT_RUNTIME_RESULT>((PVCBT_RUNTIME_RESULT)new BYTE[output_length]);
                            }
                            if (res){
                                std::cout << std::endl << "Real Journal File Locations : " << std::endl;
                                LARGE_INTEGER               liVcnPrev = result->RetrievalPointers.StartingVcn;
                                std::cout << std::setw(21) << std::right << "Start";
                                std::cout << std::setw(21) << std::right << "Length";
                                std::cout << std::setw(21) << std::right << "Lcn" << std::endl;
                                for (DWORD extent = 0; extent < result->RetrievalPointers.ExtentCount; extent++){
                                    LONGLONG _Start = liVcnPrev.QuadPart * result->FsClusterSize;
                                    LONG     _Length = (ULONG)(result->RetrievalPointers.Extents[extent].NextVcn.QuadPart - liVcnPrev.QuadPart) * result->FsClusterSize;
                                    LONGLONG _Lcn = result->RetrievalPointers.Extents[extent].Lcn.QuadPart * result->FsClusterSize;
                                    std::cout << std::setw(21) << std::right << _Start;
                                    std::cout << std::setw(21) << std::right << _Length;
                                    std::cout << std::setw(21) << std::right << _Lcn << std::endl;
                                    liVcnPrev = result->RetrievalPointers.Extents[extent].NextVcn;
                                }
                            }
                            std::cout << std::endl;
                        }
                    }
                    else{
                        std::cout << "Failed to query the runtime info." << std::endl;
                    }
                }

            }
            else if (ioctl){
                DWORD input_length = sizeof(VCBT_COMMAND_INPUT) + (sizeof(VCBT_COMMAND) * guids.size());
                DWORD output_length = sizeof(VCBT_COMMAND_RESULT) + (sizeof(VCOMMAND_RESULT) * guids.size());
                std::auto_ptr<VCBT_COMMAND_INPUT> command = std::auto_ptr<VCBT_COMMAND_INPUT>((PVCBT_COMMAND_INPUT)new BYTE[input_length]);
                std::auto_ptr<VCBT_COMMAND_RESULT> result = std::auto_ptr<VCBT_COMMAND_RESULT>((PVCBT_COMMAND_RESULT)new BYTE[output_length]);
                memset(command.get(), 0, input_length);
                memset(result.get(), 0, output_length);

                command->NumberOfCommands = guids.size();
                for (int i = 0; i < guids.size(); i++){
                    command->Commands[i].VolumeId = guids[i];
                    if (ioctl == IOCTL_VCBT_ENABLE)
                        command->Commands[i].Detail.JournalSizeInMegaBytes = vm["mb"].as<int>();
                }
                if (SendDeviceIoControl(hDevice, ioctl, (LPVOID)command.get(), input_length, (LPVOID)result.get(), output_length)){
                    LOG(LOG_LEVEL_RECORD, L"Succeed to send command(0x%08x).", ioctl);
                    std::cout << "Succeed to send command." << std::endl;
                    for (int i = 0; i < result->NumberOfResults; i++){
                        std::cout << "[Volume ID] : " << (std::string)macho::guid_(result->Results[i].VolumeId) << std::endl;
                        if (result->Results[i].Status >= 0){
                            switch (ioctl){
                            case IOCTL_VCBT_ENABLE:
                                std::cout << "Journal Id : " << result->Results[i].JournalId << std::endl;
                                std::cout << "Journal size : " << result->Results[i].Detail.JournalSizeInMegaBytes << " MBs" << std::endl;
                                break;
                            case IOCTL_VCBT_DISABLE:
                                std::cout << "Done : " << (result->Results[i].Detail.Done ? true : false) << std::endl;
                                break;
                            case IOCTL_VCBT_SNAPSHOT:
                                std::cout << "Journal Id : " << result->Results[i].JournalId << std::endl;
                                std::cout << "Done : " << (result->Results[i].Detail.Done ? true : false) << std::endl;
                                break;
                            case IOCTL_VCBT_POST_SNAPSHOT:
                                std::cout << "Journal Id : " << result->Results[i].JournalId << std::endl;
                                std::cout << "Done : " << (result->Results[i].Detail.Done ? true : false) << std::endl;
                                break;
                            case IOCTL_VCBT_UNDO_SNAPSHOT:
                                std::cout << "Journal Id : " << result->Results[i].JournalId << std::endl;
                                std::cout << "Done : " << (result->Results[i].Detail.Done ? true : false) << std::endl;
                                break;
                            case IOCTL_VCBT_IS_ENABLE:
                                std::cout << "Journal Id : " << result->Results[i].JournalId << std::endl;
                                std::cout << "Enable : " << (result->Results[i].Detail.Enabled ? true : false) << std::endl;
                                break;
                            }
                        }
                        else{
                            std::cout << "Error : " << result->Results[i].Status << std::endl;
                        }
                        std::cout << std::endl;
                    }
                }
                else{
                    LOG(LOG_LEVEL_ERROR, L"Failed to send command(0x%08x).", ioctl);
                    std::cout << "Failed to send command." << std::endl;
                }
            }
            CloseHandle(hDevice);
        }
    }
	return 0;
}


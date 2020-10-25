// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "macho.h"
#include <ntddscsi.h>
#include <Davclnt.h>
#include <boost/lexical_cast.hpp>
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"

#undef MACHO_HEADER_ONLY
#include "windows\ssh_client.hpp"
#define MACHO_HEADER_ONLY


using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;

using std::cout; using std::endl;
#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "Rpcrt4.lib")
#include <time.h>
//#define  _INF
//#define _HW_DEVICE
#if 0
class async_io : public macho::interruptable_job{
public:   
    async_io(int i) : _i(i), interruptable_job(macho::guid_::create(), macho::guid_(GUID_NULL)){
    }
    typedef boost::shared_ptr<async_io> ptr;
    struct cmp_async_io_ptr {
        bool operator() (async_io::ptr const & lhs, async_io::ptr const & rhs) const {
            //return _wcsicmp(lhs->_id.c_str(), rhs->_id.c_str()) < 0;
            return lhs->_i < rhs->_i;
        }
    };
    
    ~async_io() { std::cout << "Destructing a async_io with i=" << _i << "\n"; }
    void execute(){
        //std::srand(std::time(0)); //use current time as seed for random generator
        //int random_variable = std::rand();
        //int _seconds = random_variable % 4;
        std::string s = boost::str(boost::format("ID: %1% - %2% ") % boost::this_thread::get_id() % _i);
        std::cout << s << std::endl;
        //boost::this_thread::sleep(boost::posix_time::seconds(_seconds));
    }
    void interrupt(){}
private:
    int _i;
};
#endif

class general_io_rw {
public:
    typedef boost::shared_ptr<general_io_rw> ptr;
    general_io_rw(HANDLE handle, std::wstring path) : _is_duplicated(true), _readonly(false), _path(path){
        HANDLE hHandleDup;
        DuplicateHandle(GetCurrentProcess(),
            handle,
            GetCurrentProcess(),
            &hHandleDup,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS);
        _sector_size = _get_sector_size(handle);
        _handle = hHandleDup;
    }
    virtual ~general_io_rw(){ FlushFileBuffers(_handle); }

    static general_io_rw::ptr open(const std::string path, bool readonly = true){
        general_io_rw* rw = new general_io_rw();
        if (rw){
            rw->_handle = CreateFileA(path.c_str(), readonly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
                );
            if (rw->_handle.is_valid()){
                rw->_path = macho::stringutils::convert_ansi_to_unicode(path);
                rw->_sector_size = _get_sector_size(rw->_handle);
                rw->_readonly = readonly;
                return general_io_rw::ptr(rw);
            }
            delete rw;
        }
        return NULL;
    }

    static general_io_rw::ptr open(const std::wstring path, bool readonly = true){
        general_io_rw* rw = new general_io_rw();
        if (rw){
            rw->_handle = CreateFileW(path.c_str(),
                readonly ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE,                           // dwShareMode
                NULL,                                                         // lpSecurityAttributes
                OPEN_EXISTING,                                                // dwCreationDistribution
                readonly ? 0 : FILE_FLAG_NO_BUFFERING,                        // dwFlagsAndAttributes
                NULL                                                          // hTemplateFile
                );
            if (rw->_handle.is_valid()){
                rw->_path = path;
                rw->_sector_size = _get_sector_size(rw->_handle);
                rw->_readonly = readonly;
                return general_io_rw::ptr(rw);
            }
            delete rw;
        }
        return NULL;
    }

    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read){
        using namespace macho;
        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        DWORD opStatus = ERROR_SUCCESS;
        LARGE_INTEGER offset;
        offset.QuadPart = start;
        overlapped.Offset = offset.LowPart;
        overlapped.OffsetHigh = offset.HighPart;

        if (!ReadFile(
            _handle,
            buffer,
            number_of_bytes_to_read,
            (LPDWORD)&number_of_bytes_read,
            &overlapped)){
            if (ERROR_IO_PENDING == (opStatus = GetLastError())){
                if (!GetOverlappedResult(_handle, &overlapped, (LPDWORD)&number_of_bytes_read, TRUE)){
                    opStatus = GetLastError();
                }
            }

            if (opStatus != ERROR_SUCCESS){
                LOG(LOG_LEVEL_ERROR, _T(" (%s) error = %u"), _path.c_str(), opStatus);
                return false;
            }
        }
        return true;
    }
    virtual bool write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written){
        using namespace macho;
        OVERLAPPED overlapped;
        memset(&overlapped, 0, sizeof(overlapped));
        DWORD opStatus = ERROR_SUCCESS;
        LARGE_INTEGER offset;
        offset.QuadPart = start;
        overlapped.Offset = offset.LowPart;
        overlapped.OffsetHigh = offset.HighPart;
        if (!WriteFile(
            _handle,
            buffer,
            number_of_bytes_to_write,
            (LPDWORD)&number_of_bytes_written,
            &overlapped)){
            if (ERROR_IO_PENDING == (opStatus = GetLastError())){
                opStatus = ERROR_SUCCESS;
                if (!GetOverlappedResult(_handle, &overlapped, (LPDWORD)&number_of_bytes_written, TRUE)){
                    opStatus = GetLastError();
                }
            }

            if (opStatus != ERROR_SUCCESS){
                LOG(LOG_LEVEL_ERROR, _T("(%s) error = %u"), _path.c_str(), opStatus);
                return false;
            }

            if (number_of_bytes_written != number_of_bytes_to_write){
                opStatus = ERROR_HANDLE_EOF;
                LOG(LOG_LEVEL_ERROR, _T("(%s) error = %u"), _path.c_str(), opStatus);
                return false;
            }
        }
        return true;
    }
    virtual std::wstring path() const { return _path; }

    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
        uint32_t number_of_bytes_read = 0;
        number_of_sectors_read = 0;
        if (read(start_sector *_sector_size, number_of_sectors_to_read *_sector_size, buffer, number_of_bytes_read)){
            number_of_sectors_read = number_of_bytes_read / _sector_size;
            return true;
        }
        return false;
    }

    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){
        uint32_t number_of_bytes_written = 0;
        number_of_sectors_written = 0;
        if (write(start_sector *_sector_size, buffer, number_of_sectors_to_write*_sector_size, number_of_bytes_written)){
            number_of_sectors_written = number_of_bytes_written / _sector_size;
            return true;
        }
        return false;
    }

    virtual general_io_rw::ptr clone(){
        if (_is_duplicated)
            return general_io_rw::ptr(new general_io_rw(_handle, _path));
        else
            return open(_path, _readonly);
    }
    virtual int                    sector_size() { return _sector_size; }
protected:
    #define SECTOR_SIZE	512
    int                              _sector_size;
    static int                       _get_sector_size(HANDLE handle){
        DISK_GEOMETRY               dsk;
        DWORD junk;
        if (DeviceIoControl(handle,  // device we are querying
            IOCTL_DISK_GET_DRIVE_GEOMETRY,  // operation to perform
            NULL, 0, // no input buffer, so pass zero
            &dsk, sizeof(dsk),  // output buffer
            &junk, // discard count of bytes returned
            (LPOVERLAPPED)NULL)  // synchronous I/O
            ){
            if (dsk.BytesPerSector >= SECTOR_SIZE)
                return dsk.BytesPerSector;
        }
        return SECTOR_SIZE;
    }
private:
    general_io_rw() : _is_duplicated(false), _readonly(false){}
    macho::windows::auto_file_handle _handle;
    std::wstring                     _path;
    bool                             _readonly;
    bool                             _is_duplicated;
};


bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    //guid_ g = guid_::create();
    po::options_description target("Target");
    target.add_options()
        ("host,t", po::value<std::string>(), "VMware ESX server host IP/Name")
        ("user,u", po::value<std::string>(), "User name")
        ("password,p", po::value<std::string>(), "Password")
        ;
    po::options_description general("General");
    general.add_options()
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
        if (vm.count("help")){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("host") && vm.count("user") && vm.count("password"))
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

bool command_line_parser_commands(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    //guid_ g = guid_::create();
    po::options_description target("Allowed options");
    target.add_options()
        ("cmds,c", po::wvalue<std::vector<std::wstring>>(), "Commands")
        ;
    po::options_description general("General");
    general.add_options()
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
        if (vm.count("help")){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("cmds"))
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

class thread_exec{
public:
    void run(std::wstring cmd){
        std::wstring ret;
        {
            macho::windows::auto_lock lck(_cs);
            std::wcout << L"Running Command : " << cmd << std::endl;
        }
        macho::windows::process::exec_console_application_with_timeout(cmd, ret, -1, false);
        {
            macho::windows::auto_lock lck(_cs);
            std::wcout << L"Command (" << cmd << L") Result : " << std::endl;
            std::wcout << ret << std::endl;
        }
    }
private:
    macho::windows::critical_section _cs;
};

int _tmain(int argc, _TCHAR* argv[])
{
    try{
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed: %d\n", iResult);
            return 1;
        }
     /*
    rm -r /etc/vmware/license.cfg
    cp /etc/vmware/.#license.cfg /etc/vmware/license.cfg
    etc/init.d/vpxa restart
    */
    
    /*
        boost::filesystem::path logfile = macho::windows::environment::get_execution_full_path() + L".log";
        macho::set_log_file(logfile.wstring());
        DWORD exit_code = 0;
        po::variables_map vm;
        if (command_line_parser(vm)){
            ssh_client::ptr ssh = ssh_client::connect(vm["host"].as<std::string>(), vm["user"].as<std::string>(), vm["password"].as<std::string>());
            if (ssh){
                std::string ret;
                if (ssh->run("rm -r /etc/vmware/license.cfg", ret)){
                    std::cout << ret << std::endl;
                    if (ssh->run("cp /etc/vmware/.#license.cfg /etc/vmware/license.cfg", ret)){
                        std::cout << ret << std::endl;
                        if (ssh->run("/etc/init.d/vpxa restart", ret)){
                            std::cout << ret << std::endl;
                        }
                        else{
                            std::cout << "Failed to run command \"/etc/init.d/vpxa restart\"!" << std::endl;
                            exit_code = 1;
                        }
                    }
                    else{
                        std::cout << "Failed to run command \"cp /etc/vmware/.#license.cfg /etc/vmware/license.cfg\"!" << std::endl;
                        exit_code = 1;
                    }
                }
                else{
                    std::cout << "Failed to run command \"rm -r /etc/vmware/license.cfg\"!" << std::endl;
                    exit_code = 1;
                }
            }
            else{
                std::cout << "Failed to connect!" << std::endl;
                exit_code = 1;
            }
        }
        return exit_code;


        scheduler s;
        for (int i = 0; i < 6; i++)
        {
            async_io::ptr p = async_io::ptr(new async_io(i));
            s.schedule_job(p, macho::interval_trigger(boost::posix_time::seconds(15)));
        }
        s.start();

        boost::this_thread::sleep(boost::posix_time::minutes(5));

        return 1;

        service irm_host_packer = service::get_service(L"irm_host_packer");
        irm_host_packer.set_recovery_policy({ 60000, 60000, 60000 }, 86400);
        return 0;
    
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            printf("WSAStartup failed: %d\n", iResult);
            return 1;
        }
        ssh_client::ptr ssh = ssh_client::connect("10.10.10.36", "root", "abc@123");
        if (ssh){
            std::string ret;
            if (ssh->run("lsblk -P", ret))
                std::cout << ret << std::endl;
            if (ssh->upload_file("/root/test", 0644, ret)){
                std::cout << "success" << std::endl;
            }
            else{
                std::cout << "falied" << std::endl;
            }
        }
        return 1;
        async_io::ptr p = async_io::ptr(new async_io(1));

        scheduler s;
        s.schedule_job(p, macho::interval_trigger(boost::posix_time::seconds(15)));
        s.start();

        boost::this_thread::sleep(boost::posix_time::minutes(5));
        return 1;
        */
        //thread_exec exec;
        //po::variables_map vm;
        //if (command_line_parser_commands(vm)){
        //    boost::thread_group tg;
        //    foreach(std::wstring cmd, vm["cmds"].as<std::vector<std::wstring>>()){
        //        tg.add_thread(new boost::thread(boost::bind(&thread_exec::run, &exec,cmd)));
        //    }
        //    tg.join_all();
        //}
#if 0
        com_init _com;
        storage::ptr stg = storage::get(); //storage::local();
        storage::disk::vtr disks = stg->get_disks();
        foreach(storage::disk::ptr d, disks){
            d->clear_read_only_flag();
            d->online();
        }

#endif
#if 1
        TCHAR temp[MAX_PATH];
        LPTSTR drive;
        memset(temp, 0, sizeof(temp));
        DWORD ret = GetLogicalDriveStrings(MAX_PATH - 1, temp);
        drive = temp;
        while (ret && drive < (temp + ret)){
            if ( DRIVE_CDROM == GetDriveType(drive) ){
            }
            drive += _tcslen( drive ) + 1;
        }
#endif

#if 0
        com_init _com;
        storage::ptr stg = storage::get(); //storage::local();
        storage::disk::vtr disks = stg->get_disks();
        foreach(storage::disk::ptr d, disks){
            std::wcout << L"number : " << d->number() << std::endl;
            std::wcout << L"size : " << d->size() << std::endl;
            if (d->partition_style() == storage::ST_PARTITION_STYLE::ST_PST_MBR)
                std::wcout << L"signature : " << d->signature() << std::endl;
            if (d->partition_style() == storage::ST_PARTITION_STYLE::ST_PST_GPT)
                std::wcout << L"guid : " << d->guid() << std::endl;
            std::wcout << L"friendly_name : " << d->friendly_name() << std::endl;
            std::wcout << L"\t is boot:" << (d->is_boot() ? L"true" : L"false") << std::endl;
            std::wcout << L"\t is system:" << (d->is_system() ? L"true" : L"false") << std::endl;
            std::wcout << L"\t is offline:" << (d->is_offline() ? L"true" : L"false") << std::endl;
            std::wcout << L"\t is clustered:" << (d->is_clustered() ? L"true" : L"false") << std::endl;
            std::wcout << L"\t is read only:" << (d->is_read_only() ? L"true" : L"false") << std::endl;
            std::wcout << L"\t boot from disk:" << (d->boot_from_disk() ? L"true" : L"false") << std::endl;
            std::wcout << L"unique_id : " << d->unique_id() << std::endl;
            std::wcout << L"unique_id_format : " << d->unique_id_format() << std::endl;
            std::wcout << L"partition_style : " << d->partition_style() << std::endl;
            std::wcout << L"serial_number : " << d->serial_number() << std::endl;
            std::wcout << L"scsi address : " << boost::str(boost::wformat(L"%d:%d:%d:%d")%d->scsi_port() %d->scsi_bus()% d->scsi_target_id()% d->scsi_logical_unit()) << std::endl;
            general_io_rw::ptr rd = general_io_rw::open(boost::str(boost::format("\\\\.\\PhysicalDrive%d") % d->number()));
            if (rd){
                uint32_t r = 0;
                std::auto_ptr<BYTE> data(std::auto_ptr<BYTE>(new BYTE[512]));
                memset(data.get(), 0, 512);
                if (rd->sector_read((d->size() >> 9) - 40, 1, data.get(), r)){
                    GUID guid;
                    memcpy(&guid, &(data.get()[512 - 16]), 16);
                    std::wcout << L"customized_id : " << (std::wstring)macho::guid_(guid) << std::endl;
                }
            }
            
            std::wcout << L"partitions:" << std::endl;
            foreach(storage::partition::ptr p, d->get_partitions()){
                std::wcout << L"\t number:" << p->partition_number() << std::endl;
                std::wcout << L"\t offset:" << p->offset() << std::endl;
                std::wcout << L"\t size:" << p->size() << std::endl;
                std::wcout << L"\t is boot:" << (p->is_boot() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t is system:" << (p->is_system() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t is active:" << (p->is_active() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t is hidden:" << (p->is_hidden() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t is offline:" << (p->is_offline() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t is read only:" << (p->is_read_only() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t is shadow copy:" << (p->is_shadow_copy() ? L"true" : L"false") << std::endl;
                std::wcout << L"\t mbr_type:" << p->mbr_type() << std::endl;
                std::wcout << L"\t gpt_type:" << p->gpt_type() << std::endl;
                std::wcout << L"\t access_paths size:" << std::endl;
                foreach(std::wstring access_path, p->access_paths()){
                    std::wcout << L"\t\t" << access_path << std::endl;
                }
                std::wcout << std::endl;
            }

            std::wcout << L"volumes:" << std::endl;
            foreach(storage::volume::ptr v, d->get_volumes()){
                std::wcout << L"\t path:" << v->path() << std::endl;

                std::wstring system_volume_path = v->path();
                std::wstring szSystemDevice;
                TCHAR      szDeviceName[MAX_PATH];
                memset(szDeviceName, 0, sizeof(szDeviceName));
                if (system_volume_path[0] == TEXT('\\') ||
                    system_volume_path[1] == TEXT('\\') ||
                    system_volume_path[2] == TEXT('?') ||
                    system_volume_path[3] == TEXT('\\')){
                    DWORD  CharCount = 0;
                    if (system_volume_path[system_volume_path.length() - 1] == TEXT('\\'))
                        system_volume_path.erase(system_volume_path.length() - 1);
                    if (CharCount = QueryDosDevice(&system_volume_path[4], szDeviceName, ARRAYSIZE(szDeviceName)))
                        szSystemDevice = szDeviceName;
                }
                std::wcout << L"\t drive type:" << v->drive_type() << std::endl;
                std::wcout << L"\t drive letter:" << v->drive_letter() << std::endl;
                std::wcout << L"\t file_system:" << v->file_system() << std::endl;
                std::wcout << L"\t file_system_label:" << v->file_system_label() << std::endl;
                std::wcout << std::endl;
            }
            std::wcout << std::endl;
        }
        std::wcout << L"volumes:" << std::endl;
        foreach(storage::volume::ptr v, stg->get_volumes()){
            std::wcout << L"\t path:" << v->path() << std::endl;

            std::wstring system_volume_path = v->path();
            std::wstring szSystemDevice;
            TCHAR      szDeviceName[MAX_PATH];
            memset(szDeviceName, 0, sizeof(szDeviceName));
            if (system_volume_path[0] == TEXT('\\') ||
                system_volume_path[1] == TEXT('\\') ||
                system_volume_path[2] == TEXT('?') ||
                system_volume_path[3] == TEXT('\\')){
                DWORD  CharCount = 0;
                if (system_volume_path[system_volume_path.length() - 1] == TEXT('\\'))
                    system_volume_path.erase(system_volume_path.length() - 1);
                if (CharCount = QueryDosDevice(&system_volume_path[4], szDeviceName, ARRAYSIZE(szDeviceName)))
                    szSystemDevice = szDeviceName;
            }
            std::wcout << L"\t drive type:" << v->drive_type() << std::endl;
            std::wcout << L"\t drive letter:" << v->drive_letter() << std::endl;
            std::wcout << L"\t file_system:" << v->file_system() << std::endl;
            std::wcout << L"\t file_system_label:" << v->file_system_label() << std::endl;
            std::wcout << std::endl;
        }

#endif
#if 0 

        macho::windows::network::adapter::vtr adapters = macho::windows::network::get_network_adapters();
        if (adapters.size()){
            foreach(macho::windows::network::adapter::ptr adapter, adapters){
                if (adapter->physical_adapter()){
                    std::wstring name = adapter->caption();
                    std::string md5_checksum = md5(macho::stringutils::convert_unicode_to_utf8(adapter->mac_address()));
                    std::string n = md5_checksum.substr(0, 8) + "-" + md5_checksum.substr(8, 4) + "-" + md5_checksum.substr(12, 4) + "-" + md5_checksum.substr(16, 4) + "-" + md5_checksum.substr(20, -1);
                    std::string g = macho::guid_::create();
                    std::wstring x = macho::guid_(n);
                    std::wstring connection = adapter->net_connection_id();
                    if (connection == L"°Ï°ì³s½u"){
                        macho::windows::network::adapter_config::ptr config = adapter->get_setting();
                        if (config){
                            //config->enable_dhcp();
                            string_array_w address;
                            string_array_w subnet;
                            string_array_w dns;
                            string_array_w getways;
                            uint16_table   metric;
                            address.push_back(L"192.168.31.123");
                            subnet.push_back(L"255.255.255.0");
                            dns.push_back(L"168.95.1.1");
                            getways.push_back(L"192.168.31.254");
                            metric.push_back(1000);
                            if (config->enable_static(address, subnet))
                            {
                                if (config->set_dns_server_search_order(dns))
                                {
                                    if (config->set_gateways(getways, metric)){
                                    }
                                }
                            }
                        }
                    }
                }
            }
         }
#endif
     /*   cimv2.connect(L"cimv2", L"10.90.0.69", L"administrator", L"!qaz2wsx", RPC_C_IMP_LEVEL_DEFAULT);
        wmi_object cpuobj = cimv2.query_wmi_object(L"Win32_Processor");
        std::wstring Caption = cpuobj[L"Caption"];
        std::wstring OtherFamilyDescription = cpuobj[L"OtherFamilyDescription"];
        std::wstring ProcessorId = cpuobj[L"ProcessorId"];
        std::wstring UniqueId = cpuobj[L"UniqueId"];
        std::wcout << L"Caption : " << Caption << std::endl;
        std::wcout << L"OtherFamilyDescription : " << OtherFamilyDescription << std::endl;
        std::wcout << L"ProcessorId : " << ProcessorId << std::endl;*/
    }
    catch (macho::exception_base &e){
        std::wcout << macho::get_diagnostic_information(e) << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(e).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (boost::exception &e){
        std::cout << boost::exception_detail::get_diagnostic_information(e, "") << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(boost::exception_detail::get_diagnostic_information(e, "")).c_str());
    }
    catch (const std::exception& ex){
        std::cout << ex.what() << std::endl;
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
    }


    //wnet_connection::ptr n = wnet_connection::connect(_T("\\\\192.168.13.188\\mnt\\nfsshare"), _T("administrator"), _T("abc@123"));


    return 0;

#ifdef _INF
    operating_system os = environment::get_os_version();
    setup_inf_file inf;
    inf.load(_T("C:\\WorkSpace\\HotHatch\\bin\\virtio-win\\WNET\\AMD64\\VIOSTOR.INF"));
    stdstring provider = inf.provider();
    setup_inf_file::source_disk::map s_disks = inf.get_source_disks(os);
    setup_inf_file::source_disk_file::map s_disk_files = inf.get_source_disk_files(os);
    setup_inf_file::destination_directory::map d_dirs = inf.get_destination_directories();
    setup_inf_file::manufacture_identifier::vtr identifiers = inf.get_manufacture_identifiers(os);
    setup_inf_file::device::vtr                 devices = inf.get_devices(identifiers[0]);
    setup_inf_file::match_device_info::ptr match_dev = inf.verify(os, { _T("PCI\\VEN_1AF4&DEV_1001&SUBSYS_00021AF4&REV_00") }, {});
    setup_inf_file::setup_action::vtr actions = inf.get_setup_actions(match_dev, environment::get_windows_directory());

#endif
//
//
//    storage::ptr local_st = storage::get();
//    storage::partition::vtr partitions = local_st->get_partitions(3);
//    storage::volume::vtr volumes = local_st->get_volumes(3);
//    foreach(storage::volume::ptr v, volumes){
//        int64_t s = v->size();
//        s /= (1024 * 1024 );
//    }
//
//    wmi_services cimv2;
//    cimv2.connect(L"cimv2");
//    wmi_object cpuobj = cimv2.query_wmi_object(L"Win32_Processor");
//    std::wstring Caption = cpuobj[L"Caption"];
//    std::wstring OtherFamilyDescription = cpuobj[L"OtherFamilyDescription"];
//    std::wstring ProcessorId = cpuobj[L"ProcessorId"];
//    std::wstring UniqueId = cpuobj[L"UniqueId"];
//
//    macho::windows::cluster::ptr c = macho::windows::cluster::get(L"192.168.200.58", L"test.falc", L"administrator", L"abc@123");
//    if (c){
//        //macho::windows::cluster::resource_type::vtr rts = c->get_resource_types();
//        //foreach(macho::windows::cluster::resource_type::ptr rt, rts){
//        //    macho::windows::cluster::resource::vtr rs = rt->get_resources();
//        //    foreach(macho::windows::cluster::resource::ptr r, rs){
//        //        std::wstring name = r->name();
//        //        std::wstring description = r->description();
//        //        macho::windows::cluster::resource_type::ptr r_t = r->get_resource_type();
//        //    }
//        //}
//        macho::windows::cluster::node::vtr nodes = c->get_nodes();
//        foreach(macho::windows::cluster::node::ptr n, nodes){
//            macho::windows::cluster::resource_group::vtr groups = n->get_active_groups();
//            foreach(macho::windows::cluster::resource_group::ptr g, groups){
//                macho::windows::cluster::resource::vtr resources = g->get_resources();
//                foreach(macho::windows::cluster::resource::ptr r, resources){
//                    macho::windows::cluster::disk::vtr disks = r->get_disks();
//                    foreach(macho::windows::cluster::disk::ptr d, disks){
//                        std::wstring id = d->id();
//                        std::wstring guid;
//                        DWORD        signature, number;
//                        uint64_t     size;
//                        try{
//                            guid = d->gpt_guid();
//                        }
//                        catch (...){}
//                        try{
//                            number = d->number();
//                            size = d->size();
//                        }
//                        catch (...){}
//                        try{
//                            signature = d->signature();
//                        }
//                        catch (...){}
//                    }
//                }
//            }
//        }
//    }
//
//    macho::windows::network::adapter::vtr networks = macho::windows::network::get_network_adapters();
//    foreach(macho::windows::network::adapter::ptr net, networks){
//        if (net->mac_address().length()){
//            macho::windows::network::adapter_config::ptr setting = net->get_setting();
//            string_array_w ipaddress = setting->ip_address();
//            string_array_w ipsubnet = setting->ip_subnet();
//            string_array_w ipgetway = setting->default_ip_gateway();
//            bool           is_dhcp_enable = setting->dhcp_enabled();
//            std::wstring   mac = setting->mac_address();        
//            std::wstring   description = setting->description();
//
//            std::wstring   guid = net->guid();
//            //string_array_w ipaddress = net->network_addresses();
//            //std::wstring   description = net->description();
//            //std::wstring   mac = net->mac_address();
//        }
//    }
//
//    registry reg;
//    if (reg.open(L"SOFTWARE\\MWDC\\IronMan\\d3223b96-83cf-46be-95d6-4259d212c95e\\00-15-5D-64-01-00")){
//        reg[L"IPAddresses"].clear_multi();
//        reg[L"SubNetMasks"].clear_multi();
//        reg[L"IPAddresses"].add_multi(std::wstring(L"127.0.0.1"));
//        reg[L"SubNetMasks"].add_multi(std::wstring(L"255.255.255.0"));
//    }
//    LPSTR p = "30d7b26a-2da6-4355-ba1a-bd08a5567fb7";
//    guid_ g = p;
//    std::string sg = g;
//    std::wstring wsg = g;
//    std::wstring newid = guid_::create();
//
//    guid_ newg(newid);
//    com_init init;
//
//   // storage::ptr st = storage::get(L"172.22.6.137", L"A1\\administrator", L"abc=123");
//    storage::ptr st = storage::get(L"192.168.205.31", L"administrator", L"abc@123");
//    st->rescan();
//    
//    wmi_services wmi;
//    HRESULT hr = wmi.connect(L"cimv2", L"192.168.205.31", L"administrator", L"abc@123");
//
//    wmi_object process_startup, process, input, output;
//    DWORD rtn;
//    std::wstring error;
//    hr = wmi.get_wmi_object_with_spawn_instance(L"Win32_ProcessStartup", process_startup);
////    process_startup[L"ShowWindow"].set_value( (char16_t) 0);
//    hr = wmi.get_wmi_object(L"Win32_Process", process);
//    hr = process.get_input_parameters(L"Create", input);
//    input[L"CommandLine"] = L"notepad.exe";
//    input[L"ProcessStartupInformation"] = process_startup;
//    hr = process.exec_method(L"Create", input, output, rtn, error);
//    int ProcessId = output[L"ProcessId"];
//    environment::log_on_user_and_impersonated logon(L"Falc\\administrator", L"abc@12345", LOGON32_LOGON_NEW_CREDENTIALS, LOGON32_PROVIDER_WINNT50);
//    service_table svcs = service::get_all_services(L"192.168.205.11");
//
//	cout << "md5 of 'grape': " << md5("grape") << endl;
//
//    environment::auto_disable_wow64_fs_redirection auto_disable_wow64;

#ifdef _CAB
    cabinet::compress cabfile;
    if ( cabfile.create(_T("C:\\share\\x.cab") ) ){
        if ( cabfile.add( "C:\\Users\\Administrator\\Desktop\\Profile\\Date_2013_05_06_Time_17_14_40" ) ){
            cabfile.close();
        }
    }
#endif
    
    stdstring name = environment::get_full_domain_name();
    if ( name.length() == 0 ) name = environment::get_computer_name(); 

#ifdef _EXE
    executable_file_info info = executable_file_info::get_executable_file_info( _T("C:\\Windows\\System32\\ntoskrnl.exe") );
    stdstring os;
    if ( info.is_x86() ) os = _T("x86");
    else if ( info.is_ia64() ) os = _T("ia64");
    else if ( info.is_x64() ) os = _T("amd64");
#endif

#ifdef _SERVICE
    service_table services = service::get_all_services();
    service_table non_ms_services, non_ms_drivers;
    foreach( service svc, services ){
        /*if ( svc.name() == _T("disksafe"))*/{  
            SERVICE_STATES_ENUM state = svc.current_state();
            if ( svc.path_name().length() > 0 && 
                ( ( svc.start_mode() == SERVICE_AUTO_START_MODE ) || ( svc.start_mode() == SERVICE_SYSTEM_START_MODE ) || ( svc.start_mode() == SERVICE_BOOT_START_MODE ) ) && 
                svc.name() == _T("EFS") /*&&
                stdstring::npos == svc.company_name().find(_T("Microsoft"))*/ ){  
                stdstring n = svc.company_name();
                if ( !svc.is_driver() )
                    non_ms_services.push_back( svc );
                else
                    non_ms_drivers.push_back( svc );
            }
        }
    }
    foreach( service svc, non_ms_services ){

    }
#endif

    
#ifdef _HW_DEVICE

    device_manager devmgr;
    hardware_classes_table classes = devmgr.get_classes();
    foreach( hardware_class _class, classes ){
        hardware_devices_table devices = devmgr.get_devices( _class.name );
        if ( devices.size() ){
            foreach( hardware_device dev, devices ){
                hardware_driver drv = devmgr.get_device_driver( dev );
                if ( drv.is_oem() && drv.driver_files.size()){
                    if ( drv.original_inf_name.length()){}
                }
            }
        }
    }

#endif

#ifdef _WMI_HV
    
    wmi_services wmi;
    wmi.connect( L"virtualization", L"192.168.205.19",L"administrator",L"abc@123" );
    wmi_object_table vms = wmi.exec_query( L"SELECT * FROM Msvm_ComputerSystem" );
    
    foreach( wmi_object vm, vms ){
        std::wstring caption        = vm[L"Caption"];
        std::wstring description    = vm[L"Description"];
        std::wstring name           = vm[L"Name"];
        std::wstring element_name   = vm[L"ElementName"];

        UUID guid;
#if _UNICODE
        if ( RPC_S_INVALID_STRING_UUID ==  UuidFromString( ( RPC_WSTR ) name.c_str(), &guid ) ){
#else
        if ( RPC_S_INVALID_STRING_UUID ==  UuidFromString( ( RPC_CSTR ) name.c_str(), &guid ) ){
#endif
            continue;
        }

        if ( description == L"Microsoft Hosting Computer System" ){
            // Hypervision
        }
        else if (  description == L"Microsoft Virtual Machine" ){
            // Virtual Machine
        }
        
    }
#endif   
    
#ifdef _LIC_TEST
    std::string user    = "hq\\100002";
    std::string passowr = "abc@123";
    std::string encode = xor_crypto::encrypt2( user, lickey, user );
    std::string decode = xor_crypto::decrypt2( encode, lickey, user );
#endif    

#ifdef MACHO_GLOBAL_TRACE
    //set_log_file( _T("c:\\trace.log") );
    set_log_level( macho::LOG_LEVEL_WARNING );
#endif

#ifdef _AUTO_HANDLE_TEST
    
    auto_file_handle device = CreateFile( _T("\\\\.\\PhysicalDrive0"), GENERIC_READ | GENERIC_WRITE,       // dwDesiredAccess
                FILE_SHARE_READ | FILE_SHARE_WRITE, // dwShareMode
                NULL,                               // lpSecurityAttributes
                OPEN_EXISTING,                      // dwCreationDistribution
                0,                                  // dwFlagsAndAttributes
                NULL                                // hTemplateFile
                );
    if ( device.is_valid() ){
    
    }

#endif

#ifdef _VOL_TEST
    volume_table volumes = volume::get_volumes();
    foreach( volume _volume, volumes ){
        _volume.refresh();

    }
#endif

#ifdef _DISK_TEST

    disk_table disks = disk::get_disks();
    foreach( disk _disk, disks ){
        
        MEDIA_TYPE type      = _disk.media_type();
        ULONGLONG  cylinders = _disk.cylinders();
    }
#endif

#ifdef _WMI_TEST
    com_init _com_init;
    stdstring user_name_ex        = environment::get_user_name_ex(NameSamCompatible);
    wmi_services wmi;
#ifdef _WMI_BCD
    wmi_object input_open_store, output;
    HRESULT hr = wmi.connect( L"wmi" );
    hr = wmi.get_input_parameters( L"BCDStore", L"OpenStore", input_open_store );
    input_open_store[L"File"] = L"";
    DWORD return_value;
    std::wstring error ;
    hr = wmi.exec_method( L"BCDStore", L"OpenStore", input_open_store, output, return_value, error );
    if ( SUCCEEDED(hr ) && output[L"ReturnValue"] ){
        wmi_object store = output[L"Store"].get_wmi_object();
        wmi_object input_open_object = store.get_input_parameters( L"OpenObject" );
        input_open_object[L"Id"] = L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}";
        wmi_object output_open_object = store.exec_method( L"OpenObject", input_open_object );
         if ( output_open_object[L"ReturnValue"] ){
             wmi_object bcd_obj = output_open_object[L"Object"].get_wmi_object();
             wmi_object input_get_element = bcd_obj.get_input_parameters( L"GetElement" );
             input_get_element[L"Type"] = (LONG) 0x25000004;
             wmi_object output_get_element = bcd_obj.exec_method( L"GetElement", input_get_element );
             wmi_object element_obj = output_get_element[L"Element"].get_wmi_object();
             ULONGLONG timeout = element_obj[L"Integer"];
             wmi_object input_set_integer_element = bcd_obj.get_input_parameters( L"SetIntegerElement" );
             input_set_integer_element[L"Type"] = (LONG) 0x25000004;
             timeout = 10;
             input_set_integer_element[L"Integer"] = timeout;
             wmi_object output_set_integer_element = bcd_obj.exec_method(  L"SetIntegerElement", input_set_integer_element );
             if ( output_set_integer_element[L"ReturnValue"] ){
                bool rest = output_set_integer_element[L"ReturnValue"];
             }
         }
    }

#endif
#ifdef _WMI_OP
    HRESULT hr = wmi.connect( L"cimv2" );
    if ( !FAILED(hr) ){
        wmi_object_table objects = wmi.exec_query(L"SELECT * FROM Win32_OperatingSystem");
        wmi_object obj = objects[0];
        std::wstring caption = obj[L"Caption"];
        
        wmi_object_table services = wmi.exec_query( boost::str( boost::wformat(L"Select * from Win32_Service where Name= '%s'") %L"SNMPTRAP" ) );
        if ( services.size() ){
            wmi_object output  = services[0].exec_method( L"StartService" );
            DWORD return_value = output[L"ReturnValue"];
            wmi_object return_obj = output[L"Object"].get_wmi_object();
            if ( output.is_valid() ){
            
            }
        }

        if ( caption.length() ){
            
        }
    } 
#endif

#endif

#ifdef _REG_TEST
    //reg_native_edit edit;
    //registry reg(edit);
    bytes _bytes;
    _bytes.set(_T("10,11,12,13"), _T(","));
    registry  reg;
    DWORD     test= 0;
    LONGLONG  qtest = 0;
    if ( reg.open( _T("SOFTWARE\\SabreCloud\\aaa") )){
        //reg[_T("10,11,12,13")] = _bytes;
        bytes newbytes = reg[_T("10,11,12,13")];
        stdstring a = reg[_T("a")];
        reg.refresh_subkeys();
        reg.subkey(_T("1\\a\\2\\3\\4\\5"), true )[_T("test")] = (DWORD) 0x80000000; 
        reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("abc")]        = (DWORD) 0x80000000;
        reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("test1")]      = stdstring( _T("0x80000000") );         
        if (reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("test")].exists() &&
            reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("test")].is_dword() ){
            test        =  reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("test")];
        }
        for( int i = 0 ; i < reg.subkey(_T("1\\a\\2\\3\\4\\5") ).count(); ++i ){
            stdstring name = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i].name();
            reg.subkey(_T("1") )[reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i].name()] = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i];
            reg.subkey(_T("1\\a") )[reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i].name()] = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i];
            reg.subkey(_T("1\\a\\2") )[reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i].name()] = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i];
            reg.subkey(_T("1\\a\\2\\3") )[reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i].name()] = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i];
            reg.subkey(_T("1\\a\\2\\3\\4") )[reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i].name()] = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[i];
        }
        DWORD abc       = (DWORD) reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("abc")];
        stdstring test1 = reg.subkey(_T("1\\a\\2\\3\\4\\5") )[_T("test1")];
        reg.subkey(_T("1\\a\\2\\3\\4") ).delete_key();
        reg.delete_subkeys();
    }
#endif

#ifdef _BYTES_TEST
    bytes _bytes;
    _bytes.set(_T("10,11,12,13"), _T(","));
    stdstring str = _bytes.get(_T(":") );
#endif

#ifdef _ENV_TEST
    stdstring app_directory       = environment::get_working_directory();
    stdstring exe_filename        = environment::get_execution_filename();
    stdstring commandline         = environment::get_command_line();
    operating_system os           = environment::get_os_version();
    stdstring computer_name       = environment::get_machine_name();
    stdstring computer_name_ex    = environment::get_machine_name_ex( ComputerNameNetBIOS );
    stdstring user_name           = environment::get_user_name_ex();
    stdstring user_name_ex        = environment::get_user_name_ex(NameSamCompatible);
    stdstring sys_directory       = environment::get_system_directory();  
    stdstring windows_directory   = environment::get_windows_directory(); 
    stdstring path                = environment::get_environment_variable( _T("path") );
    std::wstring workgroup_name   = environment::get_workgroup_name();
    bool is_wow64                 = environment::is_wow64_process();
    bool is_64bit_os              = environment::is_64bit_operating_system();
    bool is_64bit_process         = environment::is_64bit_process();
#endif

#ifdef _STRING_UTLS_TEST
    std::string ansi = "ABC$123";
    std::wstring unicode = stringutils::convert_ansi_to_unicode(ansi);
    std::string new_ansi = stringutils::convert_unicode_to_ansi(unicode);
#endif

#ifdef _LOG_TEST
    FUN_TRACE_ENTER;
    mutex _mutex;
    auto_lock lock( _mutex );
    FUN_TRACE_LEAVE;
#endif
    return 0;
}


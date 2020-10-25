#pragma once
#ifndef __IRM_UNIVERSAL_DISK_RW__
#define __IRM_UNIVERSAL_DISK_RW__
#include "macho.h"
#include "..\irm_converter\irm_disk.h"

#define SECTOR_SIZE	512

struct io_range{
	typedef std::vector<io_range> vtr;
	typedef std::map< std::wstring, io_range::vtr, macho::stringutils::no_case_string_less_w> map;
	io_range(ULONGLONG s, ULONGLONG l) : start(s), length(l) {}
	ULONGLONG start;
	ULONGLONG length;
	struct compare {
		bool operator() (io_range const & lhs, io_range const & rhs) const {
			return lhs.start < rhs.start;
		}
	};
};

class universal_disk_rw {
public:
    typedef boost::shared_ptr<universal_disk_rw> ptr;
    typedef std::vector<ptr> vtr;
	typedef std::map<std::wstring, ptr> map;
    struct  exception : public macho::exception_base {};  
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output){
        uint32_t number_of_bytes_read = 0;
        bool result = false;
#if 0
        std::auto_ptr<uint8_t> buf = std::auto_ptr<uint8_t>(new uint8_t[number_of_bytes_to_read]);
        if (result = read(start, number_of_bytes_to_read, buf.get(), number_of_bytes_read)){
            output = std::string(reinterpret_cast<char const*>(buf.get()), number_of_bytes_read);
        }
#else
        output.resize(number_of_bytes_to_read);
        if (result = read(start, number_of_bytes_to_read, &output[0], number_of_bytes_read)){
            output.resize(number_of_bytes_read);
        }
#endif
        return result;
    }
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read) = 0;
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
        return write(start, buf.c_str(), (uint32_t)buf.size(), number_of_bytes_written);
    }
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written) = 0;
    virtual std::wstring path() const = 0;
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read) = 0;
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written) = 0;
    virtual universal_disk_rw::ptr clone() = 0;
    virtual int                    sector_size() { return SECTOR_SIZE; }
};

class general_io_rw : public universal_disk_rw{
public:
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

    static universal_disk_rw::ptr open(const std::string path, bool readonly = true){
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
                return universal_disk_rw::ptr(rw);
            }
            delete rw;
        }
        return NULL;
    }

    static universal_disk_rw::ptr open(const std::wstring path, bool readonly = true){
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
                return universal_disk_rw::ptr(rw);
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
    
    virtual universal_disk_rw::ptr clone(){
        if (_is_duplicated)
            return universal_disk_rw::ptr(new general_io_rw(_handle, _path));
        else
            return open(_path, _readonly);
    }
    virtual int                    sector_size() { return _sector_size; }
protected:
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

#endif
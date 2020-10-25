#pragma once
#ifndef __IRM_UNIVERSAL_DISK_RW__
#define __IRM_UNIVERSAL_DISK_RW__
//#include "..\irm_converter\irm_disk.h"
#include "system_tools.h"
#include <boost/exception/all.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//#if defined(__linux__) 
#define __in
#define __inout
typedef void *LPVOID;
typedef const void *LPCVOID;
//#endif

#define SECTOR_SIZE	512
class universal_disk_rw {
public:
    struct read_block_error : public boost::exception, public std::exception
    {
        read_block_error(string msg) :error_msg(msg) {}
        string error_msg;
        const char *what() const noexcept { return string("read_block_error:" + error_msg).c_str(); }
    };
    typedef boost::shared_ptr<universal_disk_rw> ptr;
    typedef std::vector<ptr> vtr;
    struct  exception : virtual public boost::exception, virtual public std::exception {};
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read) = 0;
    virtual bool read_mt(__in int index, __in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read) {}
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
        return write(start, buf.c_str(), (uint32_t)buf.size(), number_of_bytes_written);
    }
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written) = 0;
    virtual bool write_ex(__in uint64_t start, __in const std::string& buf, __in uint32_t original_number_of_bytes, __in bool compressed, __inout uint32_t& number_of_bytes_written) = 0;
    virtual bool write_ex(__in uint64_t start, __in const void *buffer, __in uint32_t compressed_byte, __in uint32_t number_of_bytes_to_write, __in bool compressed, __inout uint32_t& number_of_bytes_written) = 0;
    virtual std::string path() const = 0;

    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read) = 0;
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written) = 0;
    virtual universal_disk_rw::ptr clone() = 0;
    virtual int sector_size() { return SECTOR_SIZE; }
};

class general_io_rw : public universal_disk_rw {
public:
    general_io_rw(int input_fd, std::string path) : _is_duplicated(true), _readonly(false), _path(path) {
        fd = dup(input_fd);
        _sector_size = system_tools::get_disk_sector_size(fd);
    }
    ~general_io_rw() { 
        LOG_TRACE("_path = %s", _path.c_str());
        LOG_TRACE("_fd = %d", fd);
        if (fd != 0)
        {
            close(fd);
        }
        fd = 0;
        for (int i = 0; i < 16; i++)
        {
            if (dup_fds[i] !=-1)
                close(dup_fds[i]);
            dup_fds[i] = -1;
        }
    }
    static universal_disk_rw::ptr open_rw(const std::string path, bool readonly = true);
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool read_mt(__in int index,__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written);
    virtual bool write_ex(__in uint64_t start, __in const std::string& buf, __in uint32_t original_number_of_bytes, __in bool compressed, __inout uint32_t& number_of_bytes_written) {
        return false;
    }
    virtual bool write_ex(__in uint64_t start, __in const void *buffer, __in uint32_t compressed_byte, __in uint32_t number_of_bytes_to_write, __in bool compressed, __inout uint32_t& number_of_bytes_written) {
        return false;
    }

    virtual std::string path() const { return _path; }
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    virtual universal_disk_rw::ptr clone();

protected:
    int                              _sector_size;
private:
    general_io_rw() : _is_duplicated(false), _readonly(false), fd(0) {}
    int                              fd;
    int                              dup_fds[16];
    std::string                      _path;
    bool                             _readonly;
    bool                             _is_duplicated;
};
#endif
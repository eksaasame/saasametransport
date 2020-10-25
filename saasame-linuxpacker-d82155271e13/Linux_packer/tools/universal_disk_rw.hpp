#pragma once
#ifndef __IRM_UNIVERSAL_DISK_RW__
#define __IRM_UNIVERSAL_DISK_RW__
//#include "..\irm_converter\irm_disk.h"
#include "system_tools.hpp"
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
    typedef boost::shared_ptr<universal_disk_rw> ptr;
    typedef std::vector<ptr> vtr;
    struct  exception : virtual public boost::exception, virtual public std::exception {};
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output){
        std::auto_ptr<uint8_t> buf = std::auto_ptr<uint8_t>(new uint8_t[number_of_bytes_to_read]);
        uint32_t number_of_bytes_read = 0;
        bool result = false;
        if (result = read(start, number_of_bytes_to_read, buf.get(), number_of_bytes_read)){
            output = std::string(reinterpret_cast<char const*>(buf.get()), number_of_bytes_read);
        }
        return result;
    }
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read) = 0;
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
        return write(start, buf.c_str(), (uint32_t)buf.size(), number_of_bytes_written);
    }
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written) = 0;
    virtual std::string path() const = 0;

    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read) = 0;
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written) = 0;
    virtual universal_disk_rw::ptr clone() = 0;
    virtual int                    sector_size() { return SECTOR_SIZE; }
};

class general_io_rw : public universal_disk_rw {
public:
    general_io_rw(int input_fd, std::string path) : _is_duplicated(true), _readonly(false), _path(path) {
        fd = dup(input_fd);
        _sector_size = system_tools::get_disk_sector_size(fd);
    }
    ~general_io_rw() { 
        if (fd != 0)
            close(fd); 
        fd = 0; }
    static universal_disk_rw::ptr open_rw(const std::string path, bool readonly = true) {
        general_io_rw* rw = new general_io_rw();
        rw->fd = open(path.c_str(), ((readonly) ? O_RDONLY | O_NONBLOCK | O_CREAT : O_RDWR | O_NONBLOCK | O_CREAT));
        rw->_sector_size = system_tools::get_disk_sector_size(rw->fd);
        if(!rw->fd)
            return NULL;
        return universal_disk_rw::ptr(rw);;
    }
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read) {
        number_of_bytes_read = pread(fd, buffer, number_of_bytes_to_read, start);
        if (number_of_bytes_read != number_of_bytes_to_read)
        {
            printf("%d, %d\r\n", number_of_bytes_read, number_of_bytes_to_read);
            return false;
        }
        return true;
    }
    virtual bool write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written) {
        number_of_bytes_written = pwrite(fd, buffer, number_of_bytes_to_write, start);
        if (number_of_bytes_written != number_of_bytes_to_write)
        {
            return false;
        }
        return true;
    }
    virtual std::string path() const { return _path; }

    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read) {
        uint32_t number_of_bytes_read = 0;
        number_of_sectors_read = 0;
        if (read(start_sector *_sector_size, number_of_sectors_to_read *_sector_size, buffer, number_of_bytes_read)) {
            number_of_sectors_read = number_of_bytes_read / _sector_size;
            return true;
        }
        return false;
    }

    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written) {
        uint32_t number_of_bytes_written = 0;
        number_of_sectors_written = 0;
        if (write(start_sector *_sector_size, buffer, number_of_sectors_to_write*_sector_size, number_of_bytes_written)) {
            number_of_sectors_written = number_of_bytes_written / _sector_size;
            return true;
        }
        return false;
    }

    virtual universal_disk_rw::ptr clone() {
        if (_is_duplicated)
            return universal_disk_rw::ptr(new general_io_rw(fd, _path));
        else
            return open_rw(_path, _readonly);
    }
protected:
    int                              _sector_size;
private:
    general_io_rw() : _is_duplicated(false), _readonly(false), fd(0) {}
    int                              fd;
    std::string                      _path;
    bool                             _readonly;
    bool                             _is_duplicated;

};
#endif
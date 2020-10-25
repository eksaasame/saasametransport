#include "universal_disk_rw.h"
bool universal_disk_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout std::string& output) {
    std::unique_ptr<uint8_t> buf = std::unique_ptr<uint8_t>(new uint8_t[number_of_bytes_to_read]);
    uint32_t number_of_bytes_read = 0;
    bool result = false;
    if (result = read(start, number_of_bytes_to_read, buf.get(), number_of_bytes_read)) {
        output = std::string(reinterpret_cast<char const*>(buf.get()), number_of_bytes_read);
    }
    return result;
}
universal_disk_rw::ptr general_io_rw::open_rw(const std::string path, bool readonly) {
    FUNC_TRACER;
    LOG_TRACE("path = %s", path.c_str());
    general_io_rw* rw = new general_io_rw();
    rw->fd = open(path.c_str(), ((readonly) ? O_RDONLY : O_RDWR ) | O_NONBLOCK);
    if (!rw->fd)
    {
        LOG_ERROR("rw(%s)->fd open failed", path.c_str());
        LOG_ERROR("errno = %d", errno);
        return NULL;
    }
    LOG_TRACE("fd = %d", rw->fd);
    rw->_sector_size = system_tools::get_disk_sector_size(rw->fd);
    if (!rw->_sector_size == -1)
    {
        LOG_ERROR("_sector_size = -1\r\n");
    }
    rw->_readonly = readonly;
    /**/
    /*close(rw->fd);
    rw->fd = 0;*/
    /**/
    for (int i = 0; i < 16; ++i)
        rw->dup_fds[i] = -1;
    rw->_path = path;
    /*if (!rw->fd)
        return NULL;*/
    return universal_disk_rw::ptr(rw);;
}
bool general_io_rw::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read) {
    int ret = 0;
    //LOG_TRACE("number_of_bytes_to_read = %d", number_of_bytes_to_read);
    while (!fd)
    {
        fd = open(_path.c_str(), ((_readonly) ? O_RDONLY : O_RDWR) | O_NONBLOCK);
        if (!fd)
        {
            LOG_TRACE("errno = %d", errno);
        }
    }
    if (!fd)
    {
        LOG_ERROR("rw(%s)->fd open failed", _path.c_str());
    }
    int error = 0;
    do {
        ret = pread(fd, buffer, number_of_bytes_to_read, start);
        error = errno;
        if (ret == -1 && error == EAGAIN)
            sleep(0);
    } while (ret == -1 && error == EAGAIN);
    /**/
    /*close(fd);
    fd = 0;*/
    /**/

    if (ret != -1)
    {
        number_of_bytes_read = ret;
        if (number_of_bytes_read != number_of_bytes_to_read)
        {
            LOG_ERROR("read count error!! number_of_bytes_read = %lu , number_of_bytes_to_read = %lu,start = %llu\r\n", number_of_bytes_read, number_of_bytes_to_read, start);
            LOG_ERROR("read %s errno %d\r\n", path().c_str(), errno);
            return false;
        }
    }
    else
    {
        LOG_ERROR("read %s errno %d\r\n",path().c_str(),errno);
        return false;
    }

    return true;
}
/*don't use it*/
bool general_io_rw::read_mt(__in int index, __in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read)
{
    int ret = 0;
    while (!fd)
    {
        fd = open(_path.c_str(), ((_readonly) ? O_RDONLY : O_RDWR) | O_NONBLOCK);
        if (!fd)
        {
            LOG_TRACE("errno = %d", errno);
        }
    }
    if (!fd)
    {
        LOG_ERROR("rw(%s)->fd open failed", _path.c_str());
    }
    int error = 0;
    //LOG_TRACE("number_of_bytes_to_read = %d", number_of_bytes_to_read);
    if (dup_fds[index] == -1)
        dup_fds[index] = dup(fd);
    ret = pread(dup_fds[index], buffer, number_of_bytes_to_read, start);
    if (ret != -1)
    {
        number_of_bytes_read = ret;
        if (number_of_bytes_read != number_of_bytes_to_read)
        {
            LOG_ERROR("read count error!! number_of_bytes_read = %llu , number_of_bytes_to_read = %llu\r\n", number_of_bytes_read, number_of_bytes_to_read);
            LOG_ERROR("read %s errno %d\r\n", path().c_str(), errno);
            return false;
        }
    }
    else
    {
        LOG_ERROR("read %s errno %d\r\n", path().c_str(), errno);
        return false;
    }
    return true;
}

bool general_io_rw::write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written) {
    while (!fd)
    {
        fd = open(_path.c_str(), ((_readonly) ? O_RDONLY : O_RDWR ) | O_NONBLOCK);
        if (!fd)
        {
            LOG_TRACE("errno = %d", errno);
        }
    }
    if (!fd)
    {
        LOG_ERROR("rw(%s)->fd open failed", _path.c_str());
    }
    int error = 0;
    do {
        number_of_bytes_written = pwrite(fd, buffer, number_of_bytes_to_write, start);
        error = errno;
        if (number_of_bytes_written == -1 && error == EAGAIN)
            sleep(0);
    } while (number_of_bytes_written == -1 && error == EAGAIN);
    /**/
    /*close(fd);
    fd = 0;*/
    /**/

    if (number_of_bytes_written != number_of_bytes_to_write)
    {
        return false;
    }
    return true;
}
bool general_io_rw::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read) {
    uint32_t number_of_bytes_read = 0;
    /*LOG_TRACE("rw->path() = %s\r\n", path().c_str());
    LOG_TRACE("number_of_sectors_to_read = %llu", number_of_sectors_to_read);
    LOG_TRACE("_sector_size = %llu", _sector_size);*/
    number_of_sectors_read = 0;
    if (read(start_sector *_sector_size, number_of_sectors_to_read *_sector_size, buffer, number_of_bytes_read)) {
        //LOG_TRACE("general_io_rw read finish");
        number_of_sectors_read = number_of_bytes_read / _sector_size;
        return true;
    }
    return false;
}

bool general_io_rw::sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written) {
    uint32_t number_of_bytes_written = 0;
    number_of_sectors_written = 0;
    if (write(start_sector *_sector_size, buffer, number_of_sectors_to_write*_sector_size, number_of_bytes_written)) {
        number_of_sectors_written = number_of_bytes_written / _sector_size;
        return true;
    }
    return false;
}

universal_disk_rw::ptr general_io_rw::clone() {
    if (_is_duplicated)
        return universal_disk_rw::ptr(new general_io_rw(fd, _path));
    else
        return open_rw(_path, _readonly);
}
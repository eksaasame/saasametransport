#pragma once
#ifndef linuxfs_parser_H
#define linuxfs_parser_H

#include <Windows.h>
#include "..\gen-cpp\universal_disk_rw.h"
#include <vector>
#include <map>

#define  PARTITION_EXT2	        0x83
#define  PARTITION_LINUX_LVM	0x8e
#define  PARTITION_SWAP	        0x82

namespace linuxfs{

    struct lvm_mgmt : public boost::noncopyable{
        typedef std::map<std::string,std::set<std::wstring>> groups_map;
        static groups_map get_groups(std::vector<universal_disk_rw::ptr> rws);
        static groups_map get_groups(std::map<std::string, universal_disk_rw::ptr> rws);
    };

    class volume : public boost::noncopyable{
    public:
        typedef boost::shared_ptr<volume> ptr;
        typedef std::vector<volume::ptr> vtr;
        static io_range::map get_file_system_ranges(std::map<std::string, universal_disk_rw::ptr> rws);
        static io_range::map get_file_system_ranges(std::vector<universal_disk_rw::ptr> rws);
        static volume::vtr   get(universal_disk_rw::ptr rw);
        static volume::ptr   get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size);
        virtual io_range::vtr file_system_ranges() = 0;
        ULONGLONG          start() const { return offset; } //physcial offset, if rw=disk handle; 0, if rw=volume handle
        ULONGLONG          length() const { return total_size; }
    protected:
        volume(universal_disk_rw::ptr &_rw, ULONGLONG _offset, ULONGLONG _size) : rw(_rw), offset(_offset), total_size(_size){}
        ULONGLONG               offset;
        ULONGLONG               total_size;
        universal_disk_rw::ptr  rw;
    private:
        static volume::vtr   get(std::vector<universal_disk_rw::ptr> rws);
    };
};

#endif
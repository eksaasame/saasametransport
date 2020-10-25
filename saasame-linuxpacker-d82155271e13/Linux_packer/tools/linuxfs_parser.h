#pragma once
#ifndef linuxfs_parser_H
#define linuxfs_parser_H

//#include <Windows.h>
#include "universal_disk_rw.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include "storage.h"
#include "snapshot.h"
//#include "lvm.h"
#include <vector>
#include <map>
typedef unsigned long long ULONGLONG;
#define  PARTITION_EXT2	        0x83
#define  PARTITION_LINUX_LVM	0x8e
#define  PARTITION_SWAP	        0x82
using namespace linux_storage;

/*copy form winioctl.h*/
#define PARTITION_ENTRY_UNUSED          0x00      // Entry unused
#define PARTITION_FAT_12                0x01      // 12-bit FAT entries
#define PARTITION_XENIX_1               0x02      // Xenix
#define PARTITION_XENIX_2               0x03      // Xenix
#define PARTITION_FAT_16                0x04      // 16-bit FAT entries
#define PARTITION_EXTENDED              0x05      // Extended partition entry
#define PARTITION_HUGE                  0x06      // Huge partition MS-DOS V4
#define PARTITION_IFS                   0x07      // IFS Partition
#define PARTITION_OS2BOOTMGR            0x0A      // OS/2 Boot Manager/OPUS/Coherent swap
#define PARTITION_FAT32                 0x0B      // FAT32
#define PARTITION_FAT32_XINT13          0x0C      // FAT32 using extended int13 services
#define PARTITION_XINT13                0x0E      // Win95 partition using extended int13 services
#define PARTITION_XINT13_EXTENDED       0x0F      // Same as type 5 but uses extended int13 services
#define PARTITION_PREP                  0x41      // PowerPC Reference Platform (PReP) Boot Partition
#define PARTITION_LDM                   0x42      // Logical Disk Manager partition
#define PARTITION_DM                    0x54      // OnTrack Disk Manager partition
#define PARTITION_EZDRIVE               0x55      // EZ-Drive partition
#define PARTITION_UNIX                  0x63      // Unix
#define PARTITION_SPACES                0xE7      // Storage Spaces protective partition
#define PARTITION_GPT_                   0xEE      // Gpt protective partition

#define VALID_NTFT                      0xC0      // NTFT uses high order bits

enum {
    VOLUME_TYPE_EXT,
    VOLUME_TYPE_XFS
};

namespace linuxfs{

    struct no_case_string_less_a {
        bool operator()(const std::string& Left, const std::string& Right) const {
            return (strcasecmp(Left.c_str(), Right.c_str()) < 0);
        }
    };


    struct io_range{
        typedef std::vector<io_range> vtr;
        typedef std::map< std::string, io_range::vtr, no_case_string_less_a> map;
        io_range(ULONGLONG s, ULONGLONG l, ULONGLONG s_s = 0, universal_disk_rw::ptr rw = NULL) : start(s), length(l), src_start(s_s),_rw(rw){}
        ULONGLONG start;
        ULONGLONG length;
        ULONGLONG src_start;
        universal_disk_rw::ptr _rw;
        struct compare {
            bool operator() (io_range const & lhs, io_range const & rhs) const {
                return lhs.start < rhs.start;
            }
        };
    };

    struct lvm_mgmt : public boost::noncopyable{
        typedef std::map<std::string,std::set<std::string>> groups_map;
        static groups_map get_groups(std::vector<universal_disk_rw::ptr> rws);
        static groups_map get_groups(std::map<std::string, universal_disk_rw::ptr> rws);
    };

    class volume : public boost::noncopyable{
    public:
        typedef boost::shared_ptr<volume> ptr;
        typedef std::vector<volume::ptr> vtr;
        static io_range::map get_file_system_ranges(std::map<std::string, universal_disk_rw::ptr> rws , snapshot_manager::ptr sh, map<string, set<string>> & excluded_paths_map, map<string, set<string>> & resync_paths_map);
        static io_range::map get_file_system_ranges(std::vector<universal_disk_rw::ptr> rws, snapshot_manager::ptr sh, map<string, set<string>> & excluded_paths_map, map<string, set<string>> & resync_paths_map);
        static volume::vtr   get(universal_disk_rw::ptr rw);
        static volume::ptr   get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size);
        virtual io_range::vtr file_system_ranges() = 0;
        virtual int get_block_size() = 0;
        virtual uint64_t get_sb_agblocks() {};
        ULONGLONG          start() const { return offset; } //physcial offset, if rw=disk handle; 0, if rw=volume handle
        ULONGLONG          length() const { return total_size; }
        int get_type() {return volume_type;}
    protected:
        volume(universal_disk_rw::ptr &_rw, ULONGLONG _offset, ULONGLONG _size,int vt = -1) : rw(_rw), offset(_offset), total_size(_size), volume_type(vt){}
        ULONGLONG               offset;
        ULONGLONG               total_size;
        universal_disk_rw::ptr  rw;
        int                     volume_type;
        
    private:
        static volume::vtr   get(std::vector<universal_disk_rw::ptr> rws);
    };
}

#endif
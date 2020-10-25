#pragma once
#ifndef storage_H
#define storage_H

#include <iostream>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/exception/all.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/string_generator.hpp>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include "../gen-cpp/saasame_types.h"
#include "log.h"
#include "system_tools.h"
using namespace json_tools;
using namespace saasame::transport;


//system include
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/hdreg.h>
#include <linux/fs.h>
#include <scsi/sg.h>
#include <scsi/scsi.h>
#include <dirent.h>

#include "../tools/jobs_scheduler.h"
#include "../tools/universal_disk_rw.h"

#define NULL_UUID "00000000-0000-0000-0000-000000000000"
namespace linux_storage {
    class disk;
    class disk_universal_identify;
#define THROW_EXCEPTION( fmt, ... )                                                         \
    do{                                                                                     \
        char buf[1024];                                                                     \
        sprintf(buf, "%s(%d) : %s :" fmt "\r\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
        throw DEBUG_exception(buf);                                                         \
    }while(0)    

    struct DEBUG_exception : public boost::exception, public std::exception
    {
    public:
        DEBUG_exception(const char * error_message) :
            _what("exception :" + std::string(error_message)) {}
        const char *what() const noexcept { return _what.c_str(); }
    private:
        std::string _what;
    };


#define INQUIRY_REPLY_LEN  0x60
#define SERIALINQUIRY { INQUIRY, 0x01, 0x80, 0x00, INQUIRY_REPLY_LEN, 0x00}
#define NORMALINQUIRY { INQUIRY, 0x00, 0x00, 0x00, INQUIRY_REPLY_LEN, 0x00}
#define PROC_PARTITIONS "/proc/partitions"
#define DISK_BY_UUID "/dev/disk/by-uuid/"
#define PROC_MOUNTSTATS "/proc/self/mountstats"
#define PROC_MOUNTS "/proc/self/mounts"
#define COMMON_BUFFER_SIZE 1024
#define GPT_PART_SIZE_SECTORS   34
#define SECTOR_SIZE 512
#define PTS_MBR 0
#define PTS_GPT 1

    enum unique_id_format_type {
        Vendor_Specific = 0,
        Vendor_Id = 1,
        EUI64 = 2,
        FCPH_Name = 3,
        SCSI_Name_String = 8
    };

/*this is disk lay out  padding 1 byte*/
#pragma pack(push)
#pragma pack(1)

    typedef struct _GPT_PARTITIONTABLE_HEADER
    {
        uint64_t Signature;
        uint32_t Revision;
        uint32_t HeaderSize;
        uint32_t HeaderCRC32;
        uint32_t Reserved1;
        uint64_t MyLBA;
        uint64_t AlternateLBA;
        uint64_t FirstUsableLBA;
        uint64_t LastUsableLBA;
        uint8_t  DiskGUID[16];
        uint64_t PartitionEntryLBA;
        uint32_t NumberOfPartitionEntries;
        uint32_t SizeOfPartitionEntry;
        uint32_t PartitionEntryArrayCRC32;
    } GPT_PARTITIONTABLE_HEADER, *PGPT_PARTITIONTABLE_HEADER;

    typedef struct _GPT_PARTITION_ENTRY
    {
        uint8_t  PartitionTypeGuid[16];
        uint8_t  UniquePartitionGuid[16];
        uint64_t StartingLBA;
        uint64_t EndingLBA;
        uint64_t Attributes;
        uint16_t PartitionName[36];

    } GPT_PARTITION_ENTRY, *PGPT_PARTITION_ENTRY;

    /*define the disk layout*/
    typedef struct _PARTITION_RECORD
    {
        /* (0x80 = bootable, 0x00 = non-bootable, other = invalid) */
        uint8_t BootIndicator;

        /* Start of partition in CHS address, not used by EFI firmware. */
        uint8_t StartCHS[3];
        // [0] head
        // [1] sector is in bits 5-0; bits 9-8 of cylinder are in bits 7-6 
        // [2] bits 7-0 of cylinder
        uint8_t PartitionType;
        /* End of partition in CHS address, not used by EFI firmware. */
        uint8_t EndCHS[3];

        // [0] head
        // [1] sector is in bits 5-0; bits 9-8 of cylinder are in bits 7-6 
        // [2] bits 7-0 of cylinder

        /* Starting LBA address of the partition on the disk. Used by
        EFI firmware to define the start of the partition. */
        uint32_t StartingLBA;

        /* Size of partition in LBA. Used by EFI firmware to determine
        the size of the partition. */
        uint32_t SizeInLBA;

    } PARTITION_RECORD, *PPARTITION_RECORD;

    typedef struct _LEGACY_MBR
    {
        uint8_t BootCode[440];
        uint32_t UniqueMBRSignature;
        uint16_t Unknown;
        PARTITION_RECORD PartitionRecord[4];
        uint16_t Signature;

    } LEGACY_MBR, *PLEGACY_MBR;
#pragma pack(pop)

    class mount_point
    {
    public:
        typedef boost::shared_ptr<mount_point> ptr;
        typedef std::vector<ptr> vtr;
        typedef std::map<std::string, ptr> map;
        void set_device(std::string input) { _device = input; }
        void set_mounted_on(std::string input) {
            if (input[input.size() - 1] != '/')
                input += '/';
            _mounted_on = input;
        };
        void set_fstype(std::string input) { _fstype = input; };
        std::string get_device() { return _device; }
        std::string get_mounted_on() { return _mounted_on; };
        std::string get_fstype() { return _fstype; };
        std::string get_system_uuid() { return _system_uuid; }
        void set_system_uuid(std::string input) { _system_uuid = input; }
    private:
        std::string _system_uuid;
        std::string _device;
        std::string _mounted_on;
        std::string _fstype;
    };

    class mount_info
    {
        public:
            class filter
            {
                public:
                    typedef boost::shared_ptr<filter> ptr;
                    string block_device;
                    string mount_path;
                    string fs_type;
            };
            typedef boost::shared_ptr<mount_info> ptr;
            typedef std::vector<ptr> vtr;
            mount_info(string bd, string mp, string fs, string opt):block_device(bd), mount_path(mp), fs_type(fs), option(opt){}
            string block_device;
            string mount_path;
            string fs_type;
            string option;
    };

    class disk_baseA
    {
        public:
            struct preparsing_data //this data is only used by the disk_baseA, not clone to the inferbibt class
            {
                bool is_mpath_partition;
                int mpath_partition_number;
                string mpath_partition_parent;
            };
            disk_baseA() : disk_index(-1), blocks(-1), cleared(false), mp(NULL), filesystem_type(""), rw(NULL) {
                major = -1; minor = -1; metadata.is_mpath_partition = false;
            }
            typedef boost::shared_ptr<disk_baseA> ptr;
            typedef std::vector<ptr> vtr;
            typedef std::map<std::string, ptr> map;
            int major;
            int minor;
            int disk_index;	//this is not system info
            int block_type;
            uint64_t blocks;
            std::string device_filename;
            std::string ab_path;
            std::string sysfs_path;
            enum block_dev_type {
                disk = 0x1,
                partition = 0x2,
                lvm = 0x4
            };
            void set_disk_base(disk_baseA::ptr input);
            void print_data();
            uint64_t size(){ return blocks;}
            std::string get_ab_path() { return ab_path; }
            bool cleared;
            mount_point::ptr mp;
            mount_point::vtr mps; // for some file sysyem that can create subvolume.
            std::string filesystem_type;
            universal_disk_rw::ptr get_rw() { return rw; }
            std::string get_system_uuid() { return system_uuid; }
            void set_system_uuid(std::string input) { system_uuid = input; }
            static mount_point::ptr get_mount_point(string ab_path);
            static mount_point::vtr get_mount_points(string ab_path); //for btrfs. btrfs can have multi mount point under a 
            struct preparsing_data metadata;
            bool auto_setting_block_device_type();

        protected:
            universal_disk_rw::ptr rw;
            std::string system_uuid;;
    };
    
    class partitionA : public disk_baseA
    {
        public:
            typedef boost::shared_ptr<linux_storage::partitionA> ptr;
            typedef std::vector<ptr> vtr;
            typedef std::map<std::string, ptr> map;
            partitionA() : partition_start_offset(0), parent_disk(NULL), BootIndicator(false){}
            ~partitionA() { clear(); }
            uint64_t partition_start_offset;
            std::string lvname;
            std::string uuid;
            partitionA::vtr lvms;
            partitionA::vtr slaves;
            disk_baseA::ptr parent_disk;
            bool BootIndicator;
            int parsing_partition(disk_baseA::ptr disk_base);
            bool parsing_partition_header();
            int parsing_partition_lvm(disk_baseA::ptr disk_base);
            bool is_boot()
            {
                for (auto & p : lvms)
                {
                    if (p->is_boot())
                        return true;
                }
                return (mp && mp->get_mounted_on() == "/boot" );
            }
            bool is_root()
            {
                for (auto & p : lvms)
                {
                    if (p->is_root())
                        return true;
                }
                return (mp && mp->get_mounted_on() == "/" );
            }
            static bool compare(partitionA::ptr ptrA, partitionA::ptr ptrB);
            void clear();
    private:
        bool auto_setting_partition_offset();
        bool auto_setting_partition_offset_by_sys_path();
        bool auto_setting_partition_offset_by_fdisk();
    };

    class disk : public disk_baseA
    {
        public:
            typedef boost::shared_ptr<disk> ptr;
            typedef std::vector<ptr> vtr;
            typedef std::map<std::string, ptr> map;
            std::string serial_number;
            std::string string_uri;
            std::string bus_type;
            partitionA::vtr partitions;
            int sector_size;
            int partition_style;
            unsigned int MBRSignature;
            std::string str_guid;
            int scsi_port;
            int scsi_bus;
            int scsi_target_id;
            int scsi_logical_unit;
            std::string scsi_address;

            disk() : partition_style (-1), sector_size(-1), scsi_port(-1), scsi_bus(-1), scsi_target_id(-1), scsi_logical_unit(-1){}
            ~disk() { clear(); }
            static int insert_lvm_to_disk(partitionA::ptr partitionA, disk::vtr disks);
            static disk::vtr get_disk_info();
            static bool compare(disk::ptr diskA, disk::ptr diskB);
            int parsing_disk(disk_baseA::ptr disk_base);
            bool is_boot(){
                for (auto & p : partitions)
                {
                    if (p->is_boot())
                        return true;
                }
                return false;
            }
            bool is_root() {
                for (auto & p : partitions)
                {
                    if (p->is_root())
                        return true;
                }
                return false;
            }
            bool is_system() {
                for (auto & p : partitions)
                {
                    if (p->BootIndicator)
                        return true;
                }
                return false;
            }

            bool is_include_string(string partition_name) {
                for (auto & p : partitions)
                {
                    string ab_path = p->ab_path;
                    if (!string_op::remove_begining_whitespace(string_op::remove_trailing_whitespace(ab_path)).compare(string_op::remove_begining_whitespace(string_op::remove_trailing_whitespace(partition_name))))
                    {
                        return true;
                    }
                }
                return false;
            }
            void clear()
            {
                FUNC_TRACER;
                cleared = true;
                for (auto & p : partitions)
                {
                    if(!p->cleared)
                        p->clear();
                }
                partitions.clear();
            }
        private:
            int get_hd_ioctl_info();
            int get_sd_ioctl_info();
            int parsing_disk_header();
    };

    class storage
    {
    public:
        storage() {}
        ~storage() { FUNC_TRACER;  
        clear();
        print_disk();
        }
        typedef boost::shared_ptr<storage> ptr;
        typedef std::vector<ptr> vtr;
        typedef std::map<std::string, ptr> map;

        int scan_disk();
        static storage::ptr get_storage();
        static mount_info::vtr get_mount_info(mount_info::filter::ptr flt);
        disk::vtr get_all_disk();
        std::string get_disk_ab_path_from_uri(std::string uri);
        std::set<std::string> enum_disks_ab_path_from_uri(std::set<std::string> & uris);
        disk_baseA::vtr get_instances_from_ab_paths(std::set<std::string> ab_paths);
        disk_baseA::ptr get_instance_from_ab_path(std::string ab_path);
        disk_baseA::vtr enum_multi_instances_from_multi_ab_paths(std::set<std::string> ab_paths);
        std::set<std::string> enum_partition_ab_path_from_uri(std::set<std::string> uris);
        //disk_baseA::ptr gets
        partitionA::vtr get_all_partition();
        mount_point::vtr get_all_mount_point();
        partitionA::ptr get_lvm_instance_from_volname(std::string volname);
        boost::property_tree::ptree uri;
        void print_disk()
        {
            for (auto & a : _disks)
            {
                LOG_TRACE("_disks->ab_path = %s, use = %d", a->ab_path.c_str(), a.use_count());
                for (auto & b : a->partitions)
                {
                    LOG_TRACE("partitions->ab_path = %s, use = %d", b->ab_path.c_str(), b.use_count());
                }
            }
        }
        void clear()
        {
            FUNC_TRACER;
            for (auto & d : _disks)
            {
                if(!d->cleared)
                    d->clear();
            }
            _disks.clear();
        }
    private:
        std::set<std::string> enum_partition_ab_path_from_disk_ab_path(std::set<std::string> & disk_names);
        disk::vtr _disks;
    };
    class disk_universal_identify /*now this class is only use the serial number*/
    {
    public:
        typedef boost::shared_ptr<disk_universal_identify> ptr;
        disk_universal_identify(std::string uri);
        disk_universal_identify(disk & d);
        const disk_universal_identify &operator =(const disk_universal_identify& disk_uri);
        bool operator ==(const disk_universal_identify& disk_uri);
        bool operator !=(const disk_universal_identify& disk_uri);
        operator std::string();
        std::string    string() { return operator std::string(); }

    private:
        void copy(const disk_universal_identify& disk_uri);

        std::string                                 _cluster_id;
        std::string                                 _friendly_name;
        std::string                                 _serial_number;
        std::string                                 _address;
        int                                         _partition_style;
        std::string                                 _gpt_guid;//!?
        unsigned short                              _mbr_signature;
        std::string                                 _unique_id;
        int                                         _unique_id_format;
    };
}
#endif
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
#include <vector>
#include <map>
#include <unordered_map>
#include <string>

#include "log.hpp"
#include "system_tools.hpp"
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

#include "../tools/jobs_scheduler.hpp"
#include "../tools/universal_disk_rw.hpp"

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
#define PROC_MOUNTSTATS "/proc/self/mountstats"
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
    typedef struct _GUID {
        uint32_t  Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t  Data4[8];
    } GUID;


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
        GUID             PartitionTypeGuid;
        GUID             UniquePartitionGuid;
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


    class disk_baseA
    {
    public:
        disk_baseA() : disk_index(-1), blocks(-1) { major = -1; minor = -1; }
        typedef boost::shared_ptr<disk_baseA> ptr;
        typedef std::vector<ptr> vtr;
        typedef std::map<std::string, ptr> map;
        int major;
        int minor;
        int disk_index;	//this is not system info
        int block_type;
        unsigned long long blocks;
        std::string device_filename;
        std::string ab_path;
        std::string sysfs_path;
        enum block_dev_type {
            disk = 0x1,
            partition = 0x2,
            lvm = 0x4
        };
        void set_disk_base(disk_baseA::ptr input)
        {
            major = input->major;
            minor = input->minor;
            disk_index = input->disk_index;
            blocks = input->blocks;
            device_filename = input->device_filename;
            ab_path = input->ab_path;
            sysfs_path = input->sysfs_path;
        }
    protected:
        universal_disk_rw::ptr rw;
    };
    class mount_point
    {
    public:
        typedef boost::shared_ptr<mount_point> ptr;
        typedef std::vector<ptr> vtr;
        typedef std::map<std::string, ptr> map;
        void set_device(std::string input) { _device = input; }
        void set_mounted_on(std::string input) { _mounted_on = input; };
        void set_fstype(std::string input) { _fstype = input; };
        std::string get_device() { return _device; }
        std::string get_mounted_on() { return _mounted_on; };
        std::string get_fstype() { return _fstype; };
    private:
        std::string _device;
        std::string _mounted_on;
        std::string _fstype;
    };
    class partitionA : public disk_baseA
    {
    public:
        typedef boost::shared_ptr<partitionA> ptr;
        typedef std::vector<ptr> vtr;
        typedef std::map<std::string, ptr> map;
        partitionA() : partition_start_offset(-1), mp(NULL) {}

        int partition_start_offset;
        std::string mounted_path;
        std::string filesystem_type;
        std::string lvname;
        mount_point::ptr mp;
        partitionA::vtr lvms;
        partitionA::vtr slaves;
        disk_baseA::ptr parent_disk;

        int parsing_partition(disk_baseA::ptr disk_base)
        {
            block_type = disk_baseA::partition;
            set_disk_base(disk_base);
            int offset = 0;
            /*get the start*/
            std::string start_path = sysfs_path + "/start";
            FILE * fp = fopen(start_path.c_str(), "r");

            if (!fp)
            {
                LOG_ERROR("open %s failed.\n", start_path.c_str());
                goto error;
            }
            if (fscanf(fp, "%d", &offset) != 1)
            {
                LOG_ERROR("read offset failed.\n");
                goto error;
            }
            fclose(fp);
            fp = NULL;
            partition_start_offset = (offset << 9);
            /*get the mounted_path*/
            mp = get_mount_point();
            if (mp != NULL)
            {
                mounted_path = mp->get_mounted_on();
                filesystem_type = mp->get_fstype();
            }

            return 0;
        error:
            if (fp) { fclose(fp); fp = NULL; }
            return -1;
        }
        int parsing_partition_lvm(disk_baseA::ptr disk_base)
        {
            block_type = disk_baseA::lvm;
            char buf[COMMON_BUFFER_SIZE];
            int fd0;
            set_disk_base(disk_base);
            std::string lvname_path = sysfs_path + "/dm/name";
            FILE * fp = fopen(lvname_path.c_str(), "r");
            std::string slaves_folder_path;
            boost::filesystem::path pslave;
            if (!fp)
            {
                LOG_ERROR("open %s failed.\n", lvname_path.c_str());
                goto error;
            }
            if (fscanf(fp, "%s", buf) != 1)
            {
                LOG_ERROR("read %s failed.\n", sysfs_path.c_str());
                goto error;
            }
            fclose(fp);
            fp = NULL;
            lvname = string(buf);
            /*get the start*/

            /*get the mounted_path*/
            mp = get_mount_point();
            if (mp != NULL)
            {
                mounted_path = mp->get_mounted_on();
                filesystem_type = mp->get_fstype();
            }

            return 0;
        error:
            if (fp) fclose(fp);
            return -1;
        }
        bool is_boot()
        {
            return (mounted_path == "/boot");
        }
    private:
        mount_point::ptr get_mount_point()
        {
            char dev[COMMON_BUFFER_SIZE], mount[COMMON_BUFFER_SIZE], fsb[COMMON_BUFFER_SIZE];
            FILE * fp = fopen(PROC_MOUNTSTATS, "r");
            mount_point::ptr out;
            std::string mounted_device_name = (lvname.size() > 0) ? lvname : device_filename;
            if (!fp)
            {
                LOG_ERROR("open %s failed.\n", PROC_MOUNTSTATS);
                goto error;
            }
            //2. parsing the file to find the device name
            while (fscanf(fp, "device %s mounted on %s with fstype %s\n", dev, mount, fsb) == 3)
            {
                if (std::string(dev).find(mounted_device_name) != string::npos)
                {
                    out = mount_point::ptr(new mount_point());
                    out->set_device(std::string(dev));
                    out->set_mounted_on(std::string(mount));
                    out->set_fstype(std::string(fsb));
                }
            }
            fclose(fp);
        error:
            return out;
        }
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
        GUID Disk_UUID;
        std::string str_guid;
        int scsi_port;
        int scsi_bus;
        int scsi_target_id;
        int scsi_logical_unit;
        std::string scsi_address;


        disk() : partition_style (-1), sector_size(-1), scsi_port(-1), scsi_bus(-1), scsi_target_id(-1), scsi_logical_unit(-1){}
        static int insert_lvm_to_disk(partitionA::ptr partition, disk::vtr disks)
        {
            std::string slaves_folder_path = partition->sysfs_path + "/slaves";
            boost::filesystem::path pslave = boost::filesystem::path(slaves_folder_path.c_str()); //slaves should be here

            for (boost::filesystem::directory_iterator it = boost::filesystem::directory_iterator(pslave);
                it != boost::filesystem::directory_iterator(); ++it)
            {
                boost::filesystem::path px = it->path();
                for (auto & d : disks)
                {
                    if (d->partitions.size() > 0)
                    {
                        for (auto & p : d->partitions)
                        {
                            if (p->device_filename == px.filename())
                            {
                                partition->slaves.push_back(p);
                            }
                        }
                    }
                }
            }
        }
        static disk::vtr get_disk_info();

        int parsing_disk(disk_baseA::ptr disk_base)
        {
            block_type = disk_baseA::disk;
            set_disk_base(disk_base);
            char bus_link[COMMON_BUFFER_SIZE],scsi_link[COMMON_BUFFER_SIZE];
            char * tempchr;
            std::vector<string> strVec;
            std::string s2;

            memset(bus_link, 0, sizeof(bus_link));
            std::string bus_path;
            std::string scsi_path = sysfs_path + "/device";
            std::vector<string>::iterator it;
            int rd = readlink(scsi_path.c_str(), scsi_link, sizeof(scsi_link));
            if (rd <= 0) //error handle, readlink failed.
            {
                LOG_ERROR("link %s status : %d\r\n", scsi_path.c_str(), rd);
                goto error;
            }
            printf("just test scsi_link = %s\r\n",scsi_link);
            s2 = std::string(scsi_link);
            boost::split(strVec, s2, boost::is_any_of("/"));
            it = strVec.end() - 1;
            scsi_address = *it;
            if (4 != sscanf(scsi_address.c_str(), "%d:%d:%d:%d", &scsi_port, &scsi_bus, &scsi_target_id, &scsi_logical_unit))
            {
                LOG_ERROR("scsi_address %s error\r\n", scsi_address.c_str());
                goto error;
            }

            bus_path = sysfs_path + "/device/driver";
            LOG_TRACE("bus_path = %s\r\n", bus_path.c_str());
            rd = readlink(bus_path.c_str(), bus_link, sizeof(bus_link));
            if (rd <= 0) //error handle, readlink failed.
            {
                LOG_ERROR("link %s status : %d\r\n", bus_path.c_str(), rd);
                goto error;
            }
            tempchr = strstr(bus_link, "bus");
            s2 = string(tempchr);
            boost::split(strVec, s2, boost::is_any_of("/"));
            if (strVec.size() < 2) //error handle : the bus link have some wrong
            {
                LOG_ERROR("parsing bus link %s failed\r\n", bus_path.c_str());
                goto error;
            }
            bus_type = strVec[1];
            LOG_TRACE("bus = %s\r\n", bus_type.c_str());
            /*check the device is disk or not*/
            if (device_filename.find("hd", 0) != string::npos)
            {
                get_hd_ioctl_info();
                if (serial_number.empty())
                    goto error;
            }
            else if (device_filename.find("sd", 0) != string::npos)
            {
                get_sd_ioctl_info();
                if (serial_number.empty())
                    goto error;
            }
            parsing_disk_header();
            return 0;
        error:
            return -1;
        }

        bool is_boot()
        {
            for (auto & p : partitions)
            {
                if (p->is_boot())
                    return true;
            }
            return false;
        }



    private:
        int get_hd_ioctl_info()
        {
            FUNC_TRACER;
            int fd = open(ab_path.c_str(), O_RDONLY | O_NONBLOCK);
            if (!fd)
            {
                LOG_ERROR("open %s failed\r\n", ab_path.c_str());
                return -1;
            }
            /*if the file name is "hdx"*/

            struct hd_driveid hd;
            if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
            {
                LOG_ERROR("HDIO_GET_IDENTITY %s\r\n", hd.serial_no);
            }
            else if (errno == -ENOMSG) {
                LOG_ERROR("No serial number available\n");
            }
            else {
                perror("ERROR: HDIO_GET_IDENTITY");
            }
            serial_number = string((char*)hd.serial_no);
            sector_size = hd.sector_bytes;
            close(fd);
            return 0;
        }
        int get_sd_ioctl_info()
        {
            FUNC_TRACER;
            int fd = open(ab_path.c_str(), O_RDONLY | O_NONBLOCK);
            if (!fd)
            {
                LOG_ERROR("open %s failed\r\n", ab_path.c_str());
                return -1;
            }

            unsigned char sense[32];
            char scsi_data[INQUIRY_REPLY_LEN];
            struct sg_io_hdr io_hdr;
            memset(&io_hdr, 0, sizeof(io_hdr));
            unsigned char inq_cmd[] = SERIALINQUIRY;//{ INQUIRY, 0, 0, 0, INQUIRY_REPLY_LEN , 0 };
            io_hdr.interface_id = 'S';
            io_hdr.cmdp = inq_cmd;
            io_hdr.cmd_len = sizeof(inq_cmd);
            io_hdr.dxferp = scsi_data;
            io_hdr.dxfer_len = sizeof(scsi_data);
            io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
            io_hdr.sbp = sense;
            io_hdr.mx_sb_len = sizeof(sense);
            io_hdr.timeout = 5000;
            //int version_number = 0;
            //struct sg_scsi_id sd;
            if (!ioctl(fd, SG_IO, &io_hdr))
            {
                if (io_hdr.masked_status || io_hdr.host_status || io_hdr.driver_status) {
                    LOG_ERROR("status=0x%x\n", io_hdr.status);
                    LOG_ERROR("masked_status=0x%x\n", io_hdr.masked_status);
                    LOG_ERROR("host_status=0x%x\n", io_hdr.host_status);
                    LOG_ERROR("driver_status=0x%x\n", io_hdr.driver_status);
                }
                else
                {
                    serial_number = string(&scsi_data[4]);
                    LOG_TRACE("%s\r\n", serial_number.c_str());
                }
            }
            else if (errno == -ENOMSG) {
                LOG_ERROR("No serial number available\n");
            }
            else {
                LOG_ERROR("ERROR: SG_IO");
            }
            if (ioctl(fd, BLKSSZGET, &sector_size) != 0)
            {
                if (fd)close(fd);
                return -1;
            }

            close(fd);
            return 0;
        }

        int parsing_disk_header()
        {
            if (ab_path.empty())
            {
                LOG_ERROR("ab_path is empty\r\n");
                return -1;
            }
            bool result = false;
            rw = general_io_rw::open_rw(ab_path);
            if (!rw) {
                LOG_ERROR("error opening disk rw\n");
                return -1;
            }
            unsigned char *data_buf = new unsigned char[GPT_PART_SIZE_SECTORS * SECTOR_SIZE];
            //std::unique_ptr<unsigned char[]> data_buf(new unsigned char[GPT_PART_SIZE_SECTORS * SECTOR_SIZE]);
            uint32_t number_of_sectors_read = 0;
            PLEGACY_MBR part_table;
            if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf, number_of_sectors_read)) {
                //return 0;
                part_table = (PLEGACY_MBR)data_buf;
                MBRSignature = part_table->UniqueMBRSignature;
                int cnt = 0;
                /*printf("\r\n");
                for (int i = 440; i < 512; i++)
                {
                    printf("%02x ", data_buf[i]);
                    ++cnt;
                    if(cnt%16 == 0)
                        printf("\r\n");
                }
                printf("%d\r\n", sizeof(part_table->UniqueMBRSignature));
                printf("part_table->UniqueMBRSignature = %x\r\n", part_table->UniqueMBRSignature);
                printf("%d\r\n", sizeof(part_table->Unknown));
                printf("part_table->Unknown = %x\r\n", part_table->Unknown);
                printf("part_table->PartitionRecord[0].BootIndicator = %02x\r\n", part_table->PartitionRecord[0].BootIndicator);
                printf("part_table->PartitionRecord[0].StartCHS = %02x\r\n", part_table->PartitionRecord[0].StartCHS[0]);
                printf("part_table->PartitionRecord[0].StartCHS = %02x\r\n", part_table->PartitionRecord[0].StartCHS[1]);
                printf("part_table->PartitionRecord[0].StartCHS = %02x\r\n", part_table->PartitionRecord[0].StartCHS[2]);
                printf("part_table->PartitionRecord[0].PartitionType = %02x\r\n", part_table->PartitionRecord[0].PartitionType);*/
                boost::uuids::uuid u;
                if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
                {
                    partition_style = partition_style::PARTITION_GPT;
                    PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf[SECTOR_SIZE];
                    PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf[SECTOR_SIZE * 2];
                    printf("gpt_hdr->NumberOfPartitionEntries = %d\r\n", gpt_hdr->NumberOfPartitionEntries);
                    boost::uuids::uuid u;
                    memcpy(&u, gpt_hdr->DiskGUID, 16);
                    str_guid = boost::uuids::to_string(u);
                }
                else
                {
                    partition_style = partition_style::PARTITION_MBR;
                    uint8_t NULL_GUID[16] = { 0 };
                    memset(NULL_GUID, 0, 16);
                    memcpy(&u, NULL_GUID, 16);
                    str_guid = boost::uuids::to_string(u);
                }
                printf("GUID = %s\r\n", str_guid.c_str());
            }
            delete [] data_buf;
            return 0;
        }

    };
    class storage
    {
    public:
        storage() {}
        typedef boost::shared_ptr<storage> ptr;
        typedef std::vector<ptr> vtr;
        typedef std::map<std::string, ptr> map;

        int scan_disk()
        {
            try {
                _disks = disk::get_disk_info();
            }
            catch (boost::exception &e) {
                printf("%s", boost::diagnostic_information(e).c_str());
            }
        }

        static storage::ptr get_storage()
        {
            storage::ptr out = storage::ptr(new storage());
            out->scan_disk();
            return out;
        }

        disk::vtr get_all_disk()
        {
            return _disks;
        }

        std::string get_disk_ab_path_from_uri(std::string & uri)
        {
            std::set<std::string> out;
            for (auto & d : _disks)
            {
                if (d->string_uri == uri)
                    return d->ab_path;
            }
            return std::string();
        }

        std::set<std::string> enum_disks_ab_path_from_uri(std::set<std::string> & uris)
        {
            std::set<std::string> out;
            for (auto & d : _disks)
            {
                for (auto u : uris)
                {
                    std::string ad_path = get_disk_ab_path_from_uri(u);
                    if (ad_path.size() > 0)
                        out.insert(ad_path);
                }
            }
            return out;
        }


        disk_baseA::vtr get_instances_from_ab_paths(std::set<std::string> ab_paths)
        {
            disk_baseA::vtr out;
            for (auto & ab_path : ab_paths)
            {
                out.push_back(get_instance_from_ab_path(ab_path));
            }
            return out;
        }

        disk_baseA::ptr get_instance_from_ab_path(std::string ab_path)
        {
            disk_baseA::ptr out = NULL;
            bool found = false;
            for (auto & d : _disks)
            {
                if (d->ab_path == ab_path)
                {
                    found = true;
                    out = boost::static_pointer_cast<disk_baseA>(d);
                }
                else
                {
                    for (auto & p : d->partitions)
                    {
                        if (p->ab_path == ab_path)
                        {
                            found = true;
                            out = boost::static_pointer_cast<disk_baseA>(p);
                            break;
                        }
                    }
                }
                if (found)
                {
                    break;
                }
            }
            return out;
        }

        disk_baseA::vtr enum_multi_instances_from_multi_ab_paths(std::set<std::string> ab_paths)
        {
            disk_baseA::vtr out;
            for (auto & a : ab_paths)
            {
                disk_baseA::ptr d = get_instance_from_ab_path(a);
                out.push_back(d);
            }
            return out;
        }

        std::set<std::string> enum_partition_ab_path_from_uri(std::set<std::string> uris)
        {
            std::set<std::string> disks = enum_disks_ab_path_from_uri(uris);
            return enum_partition_ab_path_from_disk_ab_path(disks);
        }

        //disk_baseA::ptr gets

        partitionA::vtr get_all_partition()
        {
            partitionA::vtr out;
            for (auto & d : _disks)
            {
                if (d->partitions.size())
                    out.insert(out.end(), d->partitions.begin(), d->partitions.end());
            }
            return out;
        }
        mount_point::vtr get_all_mount_point()
        {
            mount_point::vtr out;
            for (auto & d : _disks)
            {
                if (d->partitions.size())
                {
                    for (auto &p : d->partitions)
                    {
                        if (p->mp != NULL)
                        {
                            out.push_back(p->mp);
                        }
                    }
                }
            }
            return out;
        }
        boost::property_tree::ptree uri;
    private:
        std::set<std::string> enum_partition_ab_path_from_disk_ab_path(std::set<std::string> & disk_names)
        {
            std::set<std::string> out;
            for (auto & d : _disks)
            {
                for (auto & dn : disk_names)
                {
                    if (d->ab_path == dn)
                    {
                        for (auto & p : d->partitions)
                        {
                            out.insert(p->ab_path);
                        }
                    }
                }
            }
            return out;
        }

        disk::vtr _disks;
    };
    class disk_universal_identify /*now this class is only use the serial number*/
    {
    public:
        typedef boost::shared_ptr<disk_universal_identify> ptr;
        disk_universal_identify(std::string uri) //i think use ptree to parsing this....  is oK? but json is the same i think use boost and other api is the same , so ~
        {
            mValue value;
            read(uri, value);
            _serial_number = find_value_string(value.get_obj(), "serial_number");
            _partition_style = find_value(value.get_obj(), "partition_style").get_int();
            _mbr_signature = find_value(value.get_obj(), "mbr_signature").get_int();
            _gpt_guid = find_value_string(value.get_obj(), "gpt_guid");
            _cluster_id = find_value_string(value.get_obj(), "cluster_id");
            _friendly_name = find_value_string(value.get_obj(), "friendly_name");
            _address = find_value_string(value.get_obj(), "address");
            _unique_id = find_value_string(value.get_obj(), "unique_id");
            _unique_id_format = (unique_id_format_type)find_value_int32(value.get_obj(), "unique_id_format");
        }
        disk_universal_identify(disk & d)
        {
            _serial_number = d.serial_number;
            _partition_style = d.partition_style; //need
            _mbr_signature = d.MBRSignature;//need
            _gpt_guid = d.str_guid;// d.Disk_UUID;//need    //TETSETSETS
            _cluster_id = "";//need
            _friendly_name = d.device_filename;//need
            _address = d.scsi_address;
            _unique_id = "";// find_value_string(value.get_obj(), "unique_id");
            _unique_id_format = 0;// (unique_id_format_type)find_value_int32(value.get_obj(), "unique_id_format");
        }
        const disk_universal_identify &operator =(const disk_universal_identify& disk_uri)
        {
            if (this != &disk_uri)
                copy(disk_uri);
            return *this;
        }
        bool operator ==(const disk_universal_identify& disk_uri)
        {
            if (_cluster_id.length() && disk_uri._cluster_id.length())
                return  (_cluster_id == disk_uri._cluster_id);
            if (_unique_id_format == unique_id_format_type::FCPH_Name && _unique_id_format == disk_uri._unique_id_format && !_unique_id.empty() && !disk_uri._unique_id.empty())
                return _unique_id == disk_uri._unique_id;
            else if (_serial_number.length() > 4 &&
            disk_uri._serial_number.length() > 4 &&
            (_serial_number != "2020202020202020202020202020202020202020") &&
            (disk_uri._serial_number != "2020202020202020202020202020202020202020"))
                return (_serial_number == disk_uri._serial_number);
            else if (_partition_style == disk_uri._partition_style) {
                if (_partition_style == partition_style::PARTITION_MBR) //
                    return _mbr_signature == disk_uri._mbr_signature;
                else if (_partition_style == partition_style::PARTITION_GPT)//
                    return _gpt_guid == disk_uri._gpt_guid;
                else if (!_address.empty() && !disk_uri._address.empty()) {
                    return _address == disk_uri._address;
                }
            }
            return false;
        }
        bool operator !=(const disk_universal_identify& disk_uri)
        {
            return !(operator ==(disk_uri));
        }
        operator std::string()
        {
            mObject uri;
            uri["serial_number"] = _serial_number;
            uri["partition_style"] = (int)_partition_style;
            uri["mbr_signature"] = (int)_mbr_signature;
            uri["gpt_guid"] = (std::string)_gpt_guid;
            uri["cluster_id"] = (std::string)_cluster_id;
            uri["friendly_name"] = (std::string)_friendly_name;
            uri["address"] = (std::string)_address;
            uri["unique_id"] = (std::string)_unique_id;
            uri["unique_id_format"] = (int)_unique_id_format;
            return write(uri, json_spirit::raw_utf8);
        }
        std::string    string() { return operator std::string(); }

    private:
        void copy(const disk_universal_identify& disk_uri) {
            _serial_number = disk_uri._serial_number;
            _partition_style = disk_uri._partition_style;
            _mbr_signature = disk_uri._mbr_signature;
            _gpt_guid = disk_uri._gpt_guid;
            _cluster_id = disk_uri._cluster_id;
            _friendly_name = disk_uri._friendly_name;
            _address = disk_uri._address;
            _unique_id = disk_uri._unique_id;
            _unique_id_format = disk_uri._unique_id_format;
        }

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
    inline disk::vtr disk::get_disk_info()
    {
        disk::vtr output;
        char line[COMMON_BUFFER_SIZE], buf[COMMON_BUFFER_SIZE];
        int fd0, rd;
        fstream fs;
        boost::filesystem::path p(PROC_PARTITIONS);
        disk_baseA::vtr normal_disks, lvms;
        /*gen the uuid and save it to the specific file*/
        /*open the file first*/
        if (!boost::filesystem::exists(p))
            goto error;
        fs.open(p.string().c_str(), ios::in);
        if (!fs)
            goto error;
        fs.getline(line, sizeof(line), '\n');//get the first line;
        while (fs.getline(line, sizeof(line), '\n'))
        {
            disk_baseA::ptr disk_base = disk_baseA::ptr(new disk_baseA());

            /*to parse the partition file*/
            if (sscanf(line, "%d %d %llu %s", &disk_base->major, &disk_base->minor, &disk_base->blocks, buf) != 4)
                continue;
            disk_base->device_filename = string(buf);
            disk_base->blocks <<= 10;  // * 1024 
                                       /*to parse the partition file*/
                                       /*skip the three type device "fd" =>flappy disk "sr"=> cd rom "dm"=> dymantic mapping*/
            if (disk_base->device_filename.empty() ||
                disk_base->device_filename.find("fd") != string::npos ||
                disk_base->device_filename.find("sr") != string::npos ||
                disk_base->device_filename.find("datto") != string::npos)
                continue;
            disk_base->ab_path = "/dev/" + disk_base->device_filename;
            sprintf(buf, "%d:%d", disk_base->major, disk_base->minor);
            disk_base->sysfs_path = "/sys/dev/block/" + string(buf); //add sysfs_path
            if (disk_base->major != 252)
            {
                normal_disks.push_back(disk_base);
            }
            else
            {
                lvms.push_back(disk_base);
            }
            disk_base->disk_index = (disk_base->major << 10) + disk_base->minor;
        }
        //merge the two vector
        normal_disks.insert(normal_disks.end(), lvms.begin(), lvms.end());
        for (auto & disk_base : normal_disks)
        {
            struct stat st;
            fd0 = open(disk_base->sysfs_path.c_str(), O_RDONLY);
            if (!fd0)
            {
                LOG_ERROR("open %s error\r\n", disk_base->sysfs_path.c_str());
                goto error;
            }
            int rc = fstatat(fd0, "partition", &st, 0);
            int rc2 = fstatat(fd0, "device", &st, 0);
            close(fd0);
            LOG_TRACE("sysfs_path = %s ,  rc = %d\r\n", disk_base->sysfs_path.c_str(), rc);
            if (rc != 0 && !rc2) //partition is not exist --> disk
            {
                disk::ptr dc = disk::ptr(new disk());
                if (dc->parsing_disk(disk_base))
                {
                    LOG_ERROR("parsing_disk error\r\n");
                    goto error;
                }
                output.push_back(dc);
            }
            else if (!rc)//partition exist --> partition
            {
                partitionA::ptr pc = partitionA::ptr(new partitionA());
                if (pc->parsing_partition(disk_base))
                {
                    LOG_ERROR("parsing_partition error\r\n");
                    goto error;
                }
                if (output.empty())
                {
                    LOG_ERROR("there are no disk exist.\n");
                    goto error;
                }
                for (auto & d : output)
                {
                    if (pc->device_filename.find(d->device_filename, 0) != string::npos)
                    {
                        d->partitions.push_back(pc);
                        pc->parent_disk = boost::static_pointer_cast<disk_baseA>(d);
                        break;
                    }
                }
            }
            else //lvm
            {
                partitionA::ptr pc = partitionA::ptr(new partitionA());
                pc->parsing_partition_lvm(disk_base);
                disk::insert_lvm_to_disk(pc, output);
                for (auto &s : pc->slaves)
                    s->lvms.push_back(pc);
            }
        }
        fs.close();
        if (output.size() == 0)
            goto error;
        /*write json*/
        for (auto &dc : output)
        {
            disk_universal_identify disk_uri(*dc);
            /*dc->uri.put("serial_number", dc->serial_number);
            dc->uri.put("path", dc->ab_path);
            dc->uri.put("friendly_name", dc->device_filename);
            stringstream s;
            boost::property_tree::json_parser::write_json(s, dc->uri, false);*/
            dc->string_uri = (std::string)disk_uri;
        }

        return output;
    error:
        if (fs.is_open())
            fs.close();
        THROW_EXCEPTION("get_disk_info error");
        return output;
    }
}
#endif
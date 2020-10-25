#pragma once
#include "console.hpp"
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif

#define GPT_PMBR_LBA 0
#define GPT_PMBR_SECTORS 1
#define GPT_PRIMARY_HEADER_LBA 1
#define GPT_HEADER_SECTORS 1
#define GPT_PRIMARY_PART_TABLE_LBA 2
#define MS_PARTITION_LDM   0x42                   // PARTITION_LDM

/* Needs to be packed because the u16s force misalignment. */
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct _GUID {
    unsigned int   Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
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
    GUID     DiskGUID;
    uint64_t PartitionEntryLBA;
    uint32_t NumberOfPartitionEntries;
    uint32_t SizeOfPartitionEntry;
    uint32_t PartitionEntryArrayCRC32;

} GPT_PARTITIONTABLE_HEADER, *PGPT_PARTITIONTABLE_HEADER;

typedef struct _GPT_PARTITION_ENTRY
{
    GUID     PartitionTypeGuid;
    GUID     UniquePartitionGuid;
    uint64_t StartingLBA;
    uint64_t EndingLBA;
    uint64_t Attributes;
    wchar_t PartitionName[36];

} GPT_PARTITION_ENTRY, *PGPT_PARTITION_ENTRY;

typedef struct _CHS_ADDRESS
{
    uint8_t Head;
    uint8_t Sector;
    uint32_t Cylinder;

}CHS_ADDRESS, *PCHS_ADDRESS;

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

#pragma pack(pop)   /* restore original alignment from stack */

#ifdef __cplusplus
}
#endif

class storage {

private:
    static std::string erase_trailing_whitespaces(const std::string& str) 
    {
        std::string whitespaces(" \t\f\v\n\r");
        size_t found;
        std::string temp;
        temp = str;
        if (!temp.length()) {
            temp.clear();
        }
        else {
            found = temp.find_last_not_of(whitespaces);
            if (found != std::string::npos)
                temp.erase(found + 1);
            else
                temp.clear();
        }
        return temp;
    }

    static std::string remove_begining_whitespaces(const std::string& str) 
    {
        std::string whitespaces(" \t\f\v\n\r");
        size_t found;
        std::string temp;
        temp = str;
        if (!temp.length()) return temp;

        found = temp.find_first_not_of(whitespaces);

        if (found != 0) {
            if (found != std::string::npos) {
                temp = temp.substr(found, temp.length() - found);
            }
            else
                temp.clear();
        }
        return temp;
    }
    
    static std::string remove_quotation_mark(const std::string& str)
    {
        std::string temp = erase_trailing_whitespaces(str);
        temp = remove_begining_whitespaces(temp);
        if (temp.length() > 1 && temp[0] == '\"' && str[temp.length() - 1] == '\"')
            return temp.substr(1, temp.length() - 2);
        else if (temp.length() > 1 && temp[0] == '\'' && str[temp.length() - 1] == '\'')
            return temp.substr(1, temp.length() - 2);
        else
            return temp;
    }

    static std::string guid_to_string(const GUID &guid)
    {
        char guid_cstr[37];
        snprintf(guid_cstr, sizeof(guid_cstr),
            "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            guid.Data1, guid.Data2, guid.Data3,
            guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
            guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

        return std::string(guid_cstr);
    }

    bool initialize();

public:
    typedef boost::shared_ptr<storage> ptr;

    typedef enum {
        ST_PST_UNKNOWN = 0,
        ST_PST_MBR = 1,
        ST_PST_GPT = 2
    }ST_PARTITION_STYLE;

    struct volume {
        typedef boost::shared_ptr<volume> ptr;
        typedef std::vector<ptr> vtr;
        volume(std::string _name, std::string _major, std::string _minor) :
            size(0),
            major_(_major),
            minor_(_minor),
            name(_name),
            is_current_system_volume(false) {
            boost::filesystem::path block_path(boost::str(boost::format("/sys/dev/block/%s:%s") % major_ %minor_));
            size = std::atoll(tokenize(read_from_file(block_path / "size"), "\n")[0].c_str());
            name = tokenize(read_from_file(block_path / "dm/name"), "\n")[0];
            uuid = tokenize(read_from_file(block_path / "dm/uuid"), "\n")[0];
            path = boost::str(boost::format("/dev/mapper/%s") % name);
            boost::filesystem::directory_iterator end_itr;
            for (boost::filesystem::directory_iterator itr(block_path / "slaves"); itr != end_itr; ++itr) {
                slaves.push_back(itr->path().filename().string());
            }
            for (std::string line : tokenize(console::execute(boost::str(boost::format("lsblk -P -o NAME,FSTYPE,MOUNTPOINT %s") % path)), "\n", 0, false))
            {
                std::string dev_name;
                std::vector<std::string> elements = tokenize(line, " ", 0, false);
                for (std::string element : elements) {
                    std::vector<std::string> values = tokenize(element, "=", 2, false);
                    if (2 == values.size()) {
                        if ("NAME" == values[0]) {
                            dev_name = remove_quotation_mark(values[1]);
                        }
                        if (dev_name == _name)
                        {
                            if ("FSTYPE" == values[0])
                            {
                                fs = remove_quotation_mark(values[1]);
                            }
                            else if ("MOUNTPOINT" == values[0])
                            {
                                mount_point = remove_quotation_mark(values[1]);
                                if (values[1] == "\"/boot\"" || values[1] == "\"/boot/efi\"" || values[1] == "\"/\"") {
                                    is_current_system_volume = true;
                                }
                            }
                        }
                    }
                }
            }

            for (std::string swap : tokenize(read_from_file("/proc/swaps"), "\n", 0, false)) {
                if (0 == swap.find(path)) {
                    is_current_system_volume = true;
                    break;
                }
            }
        }
        std::string         major_;
        std::string         minor_;
        std::string         name;
        std::string         path;
        uint64_t            size;
        std::vector<std::string> slaves;
        std::string fs;
        std::string mount_point;
        bool is_current_system_volume;
        std::string uuid;
    };

    struct partition {
        typedef boost::shared_ptr<partition> ptr;
        typedef std::vector<ptr> vtr;
        partition(std::string _name, uint64_t _start, uint64_t _size, std::string _major, std::string _minor, int _number) :
            name(_name),
            start(_start),
            size(_size),
            major_(_major),
            minor_(_minor),
            number(_number),
            mbr_bootable_flag(0),
            mbr_partition_type(0),
            gpt_attribute_flags(0),
            is_current_system_volume(false),
            path(boost::str(boost::format("/dev/%s") % _name))
        {
            for (std::string line : tokenize(console::execute(boost::str(boost::format("lsblk -P -o NAME,FSTYPE,MOUNTPOINT %s") % path)), "\n", 0, false))
            {
                std::string dev_name;
                std::vector<std::string> elements = tokenize(line, " ", 0, false);
                for (std::string element : elements) {
                    std::vector<std::string> values = tokenize(element, "=", 2, false);
                    if (2 == values.size()) {  
                         if ("NAME" == values[0]) {
                             dev_name = remove_quotation_mark(values[1]);
                         }
                         if (dev_name == _name)
                         {
                            if ("FSTYPE" == values[0]) 
                            {
                                fs = remove_quotation_mark(values[1]);
                            }
                            else if ("MOUNTPOINT" == values[0]) 
                            {
                                mount_point = remove_quotation_mark(values[1]);
                                if (values[1] == "\"/boot\"" || values[1] == "\"/boot/efi\"" || values[1] == "\"/\"") {
                                    is_current_system_volume = true;
                                }
                            }
                         }
                    }
                }
            }

            for (std::string swap : tokenize(read_from_file("/proc/swaps"), "\n", 0, false)) {
                if (0 == swap.find(path)) {
                    is_current_system_volume = true;
                    break;
                }
            }
        }
        std::string name;
        uint64_t start;
        uint64_t size;
        std::string major_;
        std::string minor_;
        std::string path;
        int number;
        uint8_t mbr_bootable_flag;
        uint8_t mbr_partition_type;
        std::wstring gpt_partition_name; 
        std::string gpt_partition_type; 
        uint64_t gpt_attribute_flags;
        std::string gpt_partition_guid; 
        std::string fs;
        std::string mount_point;
        bool is_current_system_volume;
        volume::vtr         volumes;

        bool is_bootable() {
            return mbr_bootable_flag == 0x80 || gpt_partition_type == "21686148-6449-6E6F-744E-656564454649";
        }

        bool is_efi_system_partition() {
            return gpt_partition_type == "C12A7328-F81F-11D2-BA4B-00A0C93EC93B";
        }
        
        bool is_swap_partition() {
            return mbr_partition_type == 0x82 || gpt_partition_type == "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F";
        }
        
        bool is_lvm_partition() {
            return mbr_partition_type == 0x8e || gpt_partition_type == "E6D6D379-F507-44C2-A23C-238F2A3DF928";
        }
        
        bool is_extended_partition() {
            return mbr_partition_type == 0x05 || mbr_partition_type == 0x0f;
        }
    };

    struct disk {
        typedef boost::shared_ptr<disk> ptr;
        typedef std::vector<ptr> vtr;
        disk(std::string _name, std::string _major, std::string _minor)
            : partition_style(ST_PST_UNKNOWN),
            signature(0),
            size(0),
            scsi_bus(-1),
            scsi_lun(-1),
            scsi_port(-1),
            scsi_target_id(-1),
            name(_name),
            major_(_major),
            minor_(_minor),
            path(boost::str(boost::format("/dev/%s") % _name)) {
            boost::filesystem::path block_path(boost::str(boost::format("/sys/dev/block/%s:%s") % major_ %minor_));
            std::vector<std::string> arr = tokenize(read_from_file(block_path / "size"), "\n");
            if (arr.size())
                size = std::atoll(arr[0].c_str());
            if (boost::filesystem::is_symlink(block_path / "device")) {
                arr = tokenize(boost::filesystem::canonical(block_path / "device").string(), "/");
                arr = tokenize(arr[arr.size() - 1], ":");
                if (4 == arr.size()) {
                    scsi_port = (uint16_t)std::atoi(arr[0].c_str());
                    scsi_bus = (uint16_t)std::atoi(arr[1].c_str());
                    scsi_target_id = (uint16_t)std::atoi(arr[2].c_str());
                    scsi_lun = (uint16_t)std::atoi(arr[3].c_str());
                }
            }
            if (boost::filesystem::exists(block_path / "serial")) {
                serial = tokenize(read_from_file(boost::filesystem::path(block_path / "serial").string()), "\n")[0];
            }
            if (boost::filesystem::exists(block_path / "device/vendor")) {
                vendor = tokenize(read_from_file(boost::filesystem::path(block_path / "device/vendor").string()), "\n")[0];
            }
            if (boost::filesystem::exists(block_path / "device/rev")) {
                rev = tokenize(read_from_file(boost::filesystem::path(block_path / "device/rev").string()), "\n")[0];
            }
            if (boost::filesystem::exists(block_path / "device/model")) {
                model = tokenize(read_from_file(boost::filesystem::path(block_path / "device/model").string()), "\n")[0];
            }
            boost::filesystem::directory_iterator end_itr;
            for (boost::filesystem::directory_iterator itr(block_path); itr != end_itr; ++itr) {
                if (0 == itr->path().filename().string().find(name)) {
                    uint64_t start = std::atoll(tokenize(read_from_file(itr->path() / "start"), "\n")[0].c_str());
                    uint64_t size = std::atoll(tokenize(read_from_file(itr->path() / "size"), "\n")[0].c_str());
                    std::vector<std::string> devs = tokenize(tokenize(read_from_file(itr->path() / "dev"), "\n")[0], ":");
                    int number = std::atoi(itr->path().filename().string().substr(name.length()).c_str());
                    partitions.push_back(partition::ptr(new partition(itr->path().filename().string(), start, size, devs[0], devs[1], number)));
                }
            }
            std::string data = read_from_file(path, 0, 34 * 512);
            if (!data.empty()) {
                PLEGACY_MBR pMBR = (PLEGACY_MBR)data.c_str();
                if (pMBR->Signature == 0xAA55) {
                    if (0xEE == pMBR->PartitionRecord[0].PartitionType) {
                        partition_style = ST_PST_GPT;
                        PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data[512];
                        PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data[512 * 2];
                        guid = guid_to_string(gpt_hdr->DiskGUID);
                        for (uint32_t i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++) {
                            for (partition::ptr p : partitions) {
                                if(p->start == gpt_part[i].StartingLBA) {
                                    p->gpt_partition_guid = guid_to_string(gpt_part[i].UniquePartitionGuid);
                                    p->gpt_partition_type = guid_to_string(gpt_part[i].PartitionTypeGuid);
                                    p->gpt_attribute_flags = gpt_part[i].Attributes;
                                    p->gpt_partition_name = gpt_part[i].PartitionName;
                                    break;
                                }
                            }
                        }
                    }
                    else {
                        signature = pMBR->UniqueMBRSignature;
                        partition_style = ST_PST_MBR; 
                        for (int i = 0; i < 4; i++) {
                            for (partition::ptr p : partitions) {
                                if (p->start == pMBR->PartitionRecord[i].StartingLBA) {
                                    p->mbr_bootable_flag = pMBR->PartitionRecord[i].BootIndicator;
                                    p->mbr_partition_type = pMBR->PartitionRecord[i].PartitionType;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        
        ST_PARTITION_STYLE  partition_style;
        uint32_t            signature;
        std::string         guid;
        uint64_t            size;
        uint32_t            scsi_bus;
        uint16_t            scsi_lun;
        uint16_t            scsi_port;
        uint16_t            scsi_target_id;
        std::string         serial;
        std::string         model;
        std::string         vendor;
        std::string         rev;
        partition::vtr      partitions;
        volume::vtr         volumes;
        std::string         major_;
        std::string         minor_;
        std::string         name;
        std::string         path;
        bool is_system() {
            for (partition::ptr p : partitions) {
                if (p->is_current_system_volume)
                    return true;
            }
            return false;
        }
    };

    storage() {
    }

    static void rescan() {
        boost::filesystem::directory_iterator end_itr;
        // cycle through the directory
        for (boost::filesystem::directory_iterator itr("/sys/class/scsi_host/"); itr != end_itr; ++itr) {
            // If it's not a directory, list it. If you want to list directories too, just remove this check.
            system(boost::str(boost::format("echo \"- - -\" > %s") % (itr->path() / "scan")).c_str());
        }
    }

    static storage::ptr get() {
        storage::ptr stg(new storage());
        if (stg && stg->initialize())
            return stg;
        else
            return storage::ptr();
    }
    disk::ptr      find_current_boot_disk() {
        for (disk::ptr d : disks) {
            for (partition::ptr p : d->partitions) {
                if (p->is_current_system_volume)
                    return d;
            }
            for (volume::ptr v : d->volumes) {
                if (v->is_current_system_volume)
                    return d;
            }
        }
        return disk::ptr();
    }

    disk::vtr      disks;
    partition::vtr partitions;
    volume::vtr    volumes;
    static std::vector<std::string> find_all_inavtive_vgvols();
    static void online_vgvols(const std::vector<std::string>& vgvols);
    static void clean_all_lvm_devices();
    static void clean_lvm_device(std::string device);
    static void update_lvm_device(std::string device);
    static std::map<std::string, std::string> find_lvm_pv_vg_map();
};

bool storage::initialize() {
    rescan();
    std::string ret = read_from_file(boost::filesystem::path("/proc/partitions"));
    if (!ret.empty()) {
        std::vector<std::string> lines = tokenize(ret, "\n");
        for (std::string line : lines) {
            std::vector<std::string> elements = tokenize(line, " ", 0, false);
            if (elements.size() == 4 && elements[0] != "major") {
                boost::filesystem::path block_path = boost::str(boost::format("/sys/dev/block/%s:%s") % elements[0] % elements[1]);
                if (boost::filesystem::exists(block_path / "device")) {
                    if (0 == elements[3].find("fd") || 0 == elements[3].find("sr"))
                        continue;
                    disks.push_back(disk::ptr(new disk(elements[3], elements[0], elements[1])));
                }
                else if (boost::filesystem::exists(block_path / "start"))
                    continue;
                else if (boost::filesystem::exists(block_path / "dm")) {
                    bool available = true;
                    boost::filesystem::directory_iterator end_itr;
                    // cycle through the directory
                    for (boost::filesystem::directory_iterator itr(block_path / "slaves"); itr != end_itr; ++itr) {
                        if (!boost::filesystem::exists(boost::filesystem::path("/dev") / itr->path().filename())) {
                            available = false;
                            break;
                        }
                    }
                    if (available) {
                        volumes.push_back(volume::ptr(new volume(elements[3], elements[0], elements[1])));
                    }
                }
            }
        }
        for (disk::ptr d : disks) {
            for (partition::ptr p : d->partitions) {
                partitions.push_back(p);
                for (volume::ptr v : volumes) {
                    for (std::string s : v->slaves) {
                        if (s == p->name) {
                            bool found = false;
                            for (volume::ptr _v : p->volumes) {
                                if (_v->name == v->name) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                                p->volumes.push_back(v);
                            found = false;
                            for (volume::ptr _v : d->volumes) {
                                if (_v->name == v->name) {
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                                d->volumes.push_back(v);
                            break;
                        }
                    }
                }
            }
        }
        return true;
    }
    return false;
}

std::vector<std::string> storage::find_all_inavtive_vgvols() {
    std::vector<std::string> vgvols;
    for (std::string line : tokenize(console::execute("lvscan -v"), "\n")) {
        bool inactive = false;
        for (std::string var : tokenize(line, " ")) {
            if (var == " " || var.empty())
                continue;
            else if (0 == strcasecmp(var.c_str(), "inactive"))
                inactive = true;
            else if (inactive) {
                vgvols.push_back(remove_quotation_mark(var));
                break;
            }
            else {
                break;
            }
        }
    }
    return vgvols;
}

void storage::online_vgvols(const std::vector<std::string>& vgvols) {
    for (std::string vol : vgvols) {
        console::execute(boost::str(boost::format("dmsetup remove -f %s") % vol));
        console::execute(boost::str(boost::format("lvchange -ay %s") % vol));
    }
}

void storage::clean_all_lvm_devices() {
    storage::ptr stg = get();
    if (stg) {
        disk::ptr current_boot_disk = stg->find_current_boot_disk();
        disk::vtr disks;
        for (disk::ptr d : stg->disks) {
            if (current_boot_disk && d->path == current_boot_disk->path)
                continue;
            disks.push_back(d);
        }
        for (volume::ptr v : stg->volumes) {
            bool skip = false;
            if (current_boot_disk) {
                for (partition::ptr p : current_boot_disk->partitions) {
                    for (volume::ptr _v : p->volumes) {
                        if (_v->name == v->name) {
                            skip = true;
                            break;
                        }
                    }
                }
            }
            if (skip)
                continue;
            console::execute(boost::str(boost::format("dmsetup remove %s --force")% v->name));
        }
        for (disk::ptr d : disks) {
            clean_lvm_device(d->path);
        }
    }
}

void storage::clean_lvm_device(std::string device) {
    console::execute(boost::str(boost::format("kpartx -d %s") % device));
    boost::this_thread::sleep(boost::posix_time::seconds(1));
}

void storage::update_lvm_device(std::string device) {
    console::execute(boost::str(boost::format("kpartx -a %s") % device));
}

std::map<std::string, std::string> storage::find_lvm_pv_vg_map() {
    std::map<std::string, std::string> mapping;
    for (std::string line : tokenize(console::execute("lvm pvscan"), "\n")) {
        std::vector<std::string> terms = tokenize(line, " ", 0 , false);
        if (terms.size() > 3 && terms[0] == "PV" && terms[2] == "VG"){
            mapping[terms[1]] = terms[3];
        }
    }
    return mapping;
}
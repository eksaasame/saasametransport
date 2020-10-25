#include "clone_disk.h"
#include "..\irm_converter\irm_disk.h"
#include "..\linuxfs_parser\linuxfs_parser.h"
#include "..\linuxfs_parser\lvm.h"
#include "ntfs_parser.h"

using namespace macho::windows;
using namespace macho;

//GUID type:		{caddebf1-4400-4de8-b103-12117dcf3ccf}
//Partition name:   Microsoft shadow copy partition
DEFINE_GUID(PARTITION_SHADOW_COPY_GUID, 0xcaddebf1L, 0x4400, 0x4de8, 0xb1, 0x03, 0x12, 0x11, 0x7d, 0xcf, 0x3c, 0xcf);

#define MBR_PART_SIZE_SECTORS   1
#define GPT_PART_SIZE_SECTORS   34
#define MINI_BLOCK_SIZE         65536UL
#define MAX_BLOCK_SIZE          8388608UL
#define WAIT_INTERVAL_SECONDS   2
#define SIGNAL_THREAD_READ      1
#define SECTOR_SIZE             512

changed_area::vtr get_windows_excluded_range(ntfs::volume::ptr vol){
    changed_area::vtr excluded;
    ntfs::file_record::ptr root = vol->root();
    if (root){
        /*ntfs::file_record::ptr system_volume_information = vol->find_sub_entry(L"System Volume Information");
        if (system_volume_information){
            ntfs::file_record::vtr snapshots = vol->find_sub_entry(std::wregex(L"^.*\\{3808876b-c176-4e48-b7ae-04046e6cc752\\}$",std::regex_constants::icase), system_volume_information);
            foreach(ntfs::file_record::ptr s, snapshots){
                foreach(const ntfs::file_extent& e, s->extents())
                    excluded.push_back(changed_area(e.physical, e.length));
            }
        }*/
        ntfs::file_record::ptr pagefile = vol->find_sub_entry(L"pagefile.sys");
        if (pagefile){
            foreach(const ntfs::file_extent& e, pagefile->extents())
                excluded.push_back(changed_area(e.physical, e.length));
        }
        ntfs::file_record::ptr hiberfil = vol->find_sub_entry(L"hiberfil.sys");
        if (hiberfil){
            foreach(const ntfs::file_extent& e, hiberfil->extents())
                excluded.push_back(changed_area(e.physical, e.length));
        }
        ntfs::file_record::ptr swapfile = vol->find_sub_entry(L"swapfile.sys");
        if (swapfile){
            foreach(const ntfs::file_extent& e, swapfile->extents())
                excluded.push_back(changed_area(e.physical, e.length));
        }
        ntfs::file_record::ptr windows = vol->find_sub_entry(L"windows");
        if (windows){
            ntfs::file_record::ptr memorydump = vol->find_sub_entry(L"memory.dmp", windows);
            if (memorydump){
                foreach(const ntfs::file_extent& e, memorydump->extents())
                    excluded.push_back(changed_area(e.physical, e.length));
            }
        }
    }
    return excluded;
}

changed_area::vtr clone_disk::get_clone_data_range(int disk_number, bool file_system_filter_enable){
    linuxfs::volume::vtr results;
    logical_volume_manager lvm;
    uint64_t max_size = 1024 * 1024 * 8;
    macho::windows::storage::ptr stg = macho::windows::storage::get();
    changed_area::vtr changeds;
    changed_area::vtr excluded;
    std::wstring path = boost::str(boost::wformat(L"\\\\.\\PhysicalDrive%d") % disk_number);
    universal_disk_rw::ptr disk_rw = general_io_rw::open(path);
    if (stg && disk_rw){
        macho::windows::storage::disk::ptr d = stg->get_disk(disk_number);
        macho::windows::storage::partition::vtr parts = stg->get_partitions(disk_number);
        if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_GPT){
            foreach(macho::windows::storage::partition::ptr p, parts){
                macho::guid_ partition_type = macho::guid_(p->gpt_type());
                if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F") || // Linux SWAP partition
                    partition_type == macho::guid_("de94bba4-06d1-4d40-a16a-bfd50179d6ac") || // Microsoft Recovery partition
                    partition_type == macho::guid_("e3c9e316-0b5c-4db8-817d-f92df00215ae"))  // Microsoft reserved partition
                {
                }
                else if (partition_type == macho::guid_("af9b60a0-1431-4f62-bc68-3311714a69ad") || // LDM data partition
                    partition_type == macho::guid_("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3"))  // LDM metadata partition
                {
                    changeds.push_back(changed_area(p->offset(), p->size()));
                }
                else if (partition_type == macho::guid_("E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
                {
                    if (file_system_filter_enable)
                        lvm.scan_pv(disk_rw, p->offset());
                    else
                        changeds.push_back(changed_area(p->offset(), p->size()));
                }
                else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                    partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                    partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                    partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                    partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                    partition_type == macho::guid_("c12a7328-f81f-11d2-ba4b-00a0c93ec93b") || //EFI system partition
                    partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                    )
                {
                    if (file_system_filter_enable){
                        ntfs::volume::ptr vol = ntfs::volume::get(disk_rw, p->offset());
                        if (vol){
                            ntfs::io_range::vtr ranges = vol->file_system_ranges();
                            foreach(ntfs::io_range& r, ranges){
                                changeds.push_back(changed_area(r.start, r.length));
                            }
                            changed_area::vtr _excluded = get_windows_excluded_range(vol);
                            excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
                        }
                        else{
                            linuxfs::volume::ptr v = linuxfs::volume::get(disk_rw, p->offset(), p->size());
                            if (v){
                                linuxfs::io_range::vtr ranges = v->file_system_ranges();
                                foreach(linuxfs::io_range& r, ranges){
                                    changeds.push_back(changed_area(r.start, r.length));
                                }
                            }
                            else{
                                changeds.push_back(changed_area(p->offset(), p->size()));
                            }
                        }
                    }
                    else{
                        changeds.push_back(changed_area(p->offset(), p->size()));
                    }
                }

            }
        }
        else if (d->partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR){
            changeds = get_epbrs(disk_rw, *d.get(), parts, 0);
            foreach(macho::windows::storage::partition::ptr p, parts){
                if (p->mbr_type() == PARTITION_IFS){
                    ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(disk_rw, p->offset()) : NULL;
                    if (v) {
                        ntfs::io_range::vtr ranges = v->file_system_ranges();
                        foreach(ntfs::io_range& r, ranges){
                            changeds.push_back(changed_area(r.start, r.length));
                        }
                        changed_area::vtr _excluded = get_windows_excluded_range(v);
                        excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
                    }
                    else{
                        changeds.push_back(changed_area(p->offset(), p->size()));
                    }
                }
                else if (p->mbr_type() == 82 && p->size() != 0) // Linux SWAP partition
                {
                }
                else if (p->mbr_type() == PARTITION_EXT2){
                    linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(disk_rw, p->offset(),p->size()) : NULL;
                    if (v){
                        linuxfs::io_range::vtr ranges = v->file_system_ranges();
                        foreach(linuxfs::io_range& r, ranges){
                            changeds.push_back(changed_area(r.start, r.length));
                        }
                    }
                    else{
                        changeds.push_back(changed_area(p->offset(),p->size()));
                    }
                }
                else if (p->mbr_type() == PARTITION_LINUX_LVM){
                    lvm.scan_pv(disk_rw, p->offset());
                }
                else if ( p->size() && (!(p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED)) ){
                    changeds.push_back(changed_area(p->offset(), p->size()));
                }
            }
        }
        else{
            changeds.push_back(changed_area(0, d->size()));
        }
    }

    linuxfs::io_range::map lvm_system_ranges = lvm.get_system_ranges();
    foreach(linuxfs::io_range& r, lvm_system_ranges[disk_rw->path()]){
        changeds.push_back(changed_area(r.start, r.length));
    }
    std::sort(changeds.begin(), changeds.end(), changed_area::compare());
    std::sort(excluded.begin(), excluded.end(), changed_area::compare());
    size_t length = 1024 * 1024;
    if (changeds[0].start < length)
        length = changeds[0].start;
    changeds.insert(changeds.begin(), changed_area(0, length));
    return final_changed_area(changeds, excluded, max_size);
}

changed_area::vtr  clone_disk::get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions, const uint64_t& start_offset){
    changed_area::vtr changeds;
    int length = 4096;
    if (d.partition_style() == macho::windows::storage::ST_PARTITION_STYLE::ST_PST_MBR){
        std::auto_ptr<BYTE> pBuf = std::auto_ptr<BYTE>(new BYTE[length]);
        if (NULL != pBuf.get()){
            foreach(macho::windows::storage::partition::ptr p, partitions){
                if (!(p->mbr_type() == PARTITION_XINT13_EXTENDED || p->mbr_type() == PARTITION_EXTENDED))
                    continue;
                uint64_t start = p->offset();
                bool loop = true;
                while (loop){
                    loop = false;
                    memset(pBuf.get(), 0, length);
                    uint32_t nRead = 0;
                    BOOL result = FALSE;
                    if (result = disk_rw->read(start, length, pBuf.get(), nRead)){
                        PLEGACY_MBR pMBR = (PLEGACY_MBR)pBuf.get();
                        if (pMBR->Signature != 0xAA55)
                            break;
                        if (start > start_offset)
                            changeds.push_back(changed_area(start, d.logical_sector_size()));
                        for (int i = 0; i < 2; i++){
                            if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                start = p->offset() + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * d.logical_sector_size());
                                loop = true;
                                break;
                            }
                            else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                            }
                            else{
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    std::sort(changeds.begin(), changeds.end(), changed_area::compare());
    return changeds;
}

changed_area::vtr clone_disk::get_clone_data_range(universal_disk_rw::ptr rw, bool file_system_filter_enable){
    changed_area::vtr changed;
    changed_area::vtr excluded;
    uint64_t max_size = 1024 * 1024 * 8;
    linuxfs::volume::vtr results;
    logical_volume_manager lvm;
    bool result = false;
    std::auto_ptr<BYTE> data_buf(new BYTE[GPT_PART_SIZE_SECTORS * SECTOR_SIZE]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)){
        part_table = (PLEGACY_MBR)data_buf.get();

        if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
        {
            PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[SECTOR_SIZE];
            PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[SECTOR_SIZE * 2];

            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
            {
                macho::guid_ partition_type = macho::guid_(gpt_part[i].PartitionTypeGuid);
                if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F") || // Linux SWAP partition
                    partition_type == macho::guid_("de94bba4-06d1-4d40-a16a-bfd50179d6ac") || // Microsoft Recovery partition
                    partition_type == macho::guid_("e3c9e316-0b5c-4db8-817d-f92df00215ae"))  // Microsoft reserved partition
                {
                }
                else if (partition_type == macho::guid_("af9b60a0-1431-4f62-bc68-3311714a69ad") || // LDM data partition
                    partition_type == macho::guid_("5808c8aa-7e8f-42e0-85d2-e1e90434cfb3"))  // LDM metadata partition
                {
                    changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
                }
                else if (partition_type == macho::guid_("E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
                {
                    if (file_system_filter_enable)
                        lvm.scan_pv(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size());
                    else
                        changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
                }
                else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                    partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                    partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                    partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                    partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                    partition_type == macho::guid_("c12a7328-f81f-11d2-ba4b-00a0c93ec93b") || //EFI system partition
                    partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                    )
                {
                    if (file_system_filter_enable){
                        ntfs::volume::ptr vol = ntfs::volume::get(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size());
                        if (vol){
                            ntfs::io_range::vtr ranges = vol->file_system_ranges();
                            foreach(ntfs::io_range& r, ranges){
                                changed.push_back(changed_area(r.start, r.length));
                            }
                            changed_area::vtr _excluded = get_windows_excluded_range(vol);
                            excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
                        }
                        else{
                            linuxfs::volume::ptr v = linuxfs::volume::get(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size());
                            if (v){
                                linuxfs::io_range::vtr ranges = v->file_system_ranges();
                                foreach(linuxfs::io_range& r, ranges){
                                    changed.push_back(changed_area(r.start, r.length));
                                }
                            }
                            else{
                                changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
                            }
                        }
                    }
                    else{
                        changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
                    }
                }
            }
        }
        else
        {
            for (int index = 0; index < 4; index++)
            {
                uint64_t start = ((uint64_t)part_table->PartitionRecord[index].StartingLBA) * rw->sector_size();
                if (0 == part_table->PartitionRecord[index].SizeInLBA){
                }
                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_IFS){              
                    ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(rw, start) : NULL;
                    if (v) {
                        ntfs::io_range::vtr ranges = v->file_system_ranges();
                        foreach(ntfs::io_range& r, ranges){
                            changed.push_back(changed_area(r.start, r.length));
                        }
                        changed_area::vtr _excluded = get_windows_excluded_range(v);
                        excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
                    }
                    else{
                        changed.push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())));
                    }
                }
                else if (part_table->PartitionRecord[index].PartitionType == 82 && part_table->PartitionRecord[index].SizeInLBA != 0) // Linux SWAP partition
                {
                }
                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_EXT2){
                    linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(rw, start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())) : NULL;
                    if (v){
                        linuxfs::io_range::vtr ranges = v->file_system_ranges();
                        foreach(linuxfs::io_range& r, ranges){
                            changed.push_back(changed_area(r.start, r.length));
                        }
                    }
                    else{
                        changed.push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())));
                    }
                }
                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM){
                    lvm.scan_pv(rw, start);
                }
                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                    part_table->PartitionRecord[index].PartitionType == PARTITION_EXTENDED)
                {
                    std::string buf2;
                    bool loop = true;
                    while (loop)
                    {
                        loop = false;
                        if (rw->read(start, rw->sector_size(), buf2))
                        {
                            PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                            if (pMBR->Signature != 0xAA55)
                                break;
                            changed.push_back(changed_area(start, rw->sector_size()));
                            for (int i = 0; i < 2; i++)
                            {
                                if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                    pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED)
                                {
                                    start = ((uint64_t)((uint64_t)part_table->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * rw->sector_size());
                                    loop = true;
                                    break;
                                }
                                else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0)
                                {
                                    if (pMBR->PartitionRecord[i].PartitionType == 82)// Linux SWAP partition
                                    {
                                    }
                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_IFS){
                                        ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size())) : NULL;
                                        if (v) {
                                            ntfs::io_range::vtr ranges = v->file_system_ranges();
                                            foreach(ntfs::io_range& r, ranges){
                                                changed.push_back(changed_area(r.start, r.length));
                                            }
                                            changed_area::vtr _excluded = get_windows_excluded_range(v);
                                            excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
                                        }
                                        else{
                                            changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
                                        }
                                    }
                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2){
                                        linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())) : NULL;
                                        if (v){
                                            linuxfs::io_range::vtr ranges = v->file_system_ranges();
                                            foreach(linuxfs::io_range& r, ranges){
                                                changed.push_back(changed_area(r.start, r.length));
                                            }
                                        }
                                        else{
                                            changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
                                        }
                                    }
                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM){
                                        if (file_system_filter_enable)
                                            lvm.scan_pv(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()));
                                        else
                                            changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
                                    }
                                    else {
                                        changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }
                        }
                    }
                }
                else{
                    changed.push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())));
                }
            }
        }
    }

    linuxfs::io_range::map lvm_system_ranges = lvm.get_system_ranges();
    foreach(linuxfs::io_range& r, lvm_system_ranges[rw->path()]){
        changed.push_back(changed_area(r.start, r.length));
    }
    std::sort(changed.begin(), changed.end(), changed_area::compare());
    std::sort(excluded.begin(), excluded.end(), changed_area::compare());
    size_t length = 1024 * 1024;
    if (changed[0].start < length)
        length = changed[0].start;
    changed.insert(changed.begin(), changed_area(0, length));
    return final_changed_area(changed, excluded, max_size);
}

changed_area::vtr clone_disk::final_changed_area(changed_area::vtr& src, changed_area::vtr& excludes, uint64_t max_size){
    changed_area::vtr chunks;
    if (src.empty())
        return src;
    else if (excludes.empty())
        chunks = src;
    else
    {
        std::sort(src.begin(), src.end(), [](changed_area const& lhs, changed_area const& rhs){ return lhs.start < rhs.start; });
        std::sort(excludes.begin(), excludes.end(), [](changed_area const& lhs, changed_area const& rhs){ return lhs.start < rhs.start; });

        changed_area::vtr::iterator it = excludes.begin();
        int64_t start = 0;

        foreach(auto e, src)
        {
            LOG(LOG_LEVEL_DEBUG, "s/%I64u:%I64u", e.start, e.length);
            start = e.start;

            if (it != excludes.end())
            {
                while (it->start + it->length < start)
                {
                    it = excludes.erase(it);
                    if (it == excludes.end())
                        break;
                }

                if (it != excludes.end())
                {
                    if ((e.start + e.length) < it->start)
                        chunks.push_back(e);
                    else
                    {
                        while (start < e.start + e.length)
                        {
                            changed_area chunk;

                            if (it->start - start > 0)
                            {
                                chunk.start = start;
                                chunk.length = it->start - start;
                                chunks.push_back(chunk);
                                LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", chunk.start, chunk.length);
                            }

                            if (it->start + it->length < e.start + e.length)
                            {
                                start = it->start + it->length;
                                it++;
                                if (it == excludes.end() || it->start >(e.start + e.length))
                                {
                                    chunk.start = start;
                                    chunk.length = e.start + e.length - start;
                                    chunks.push_back(chunk);
                                    LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", chunk.start, chunk.length);
                                    break;
                                }
                            }
                            else
                                break;
                        }
                    }
                }
                else
                {
                    chunks.push_back(e);
                    LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", e.start, e.length);
                }
            }
            else
            {
                chunks.push_back(e);
                LOG(LOG_LEVEL_DEBUG, "t/%I64u:%I64u", e.start, e.length);
            }
        }
    }
    changed_area::vtr _chunks;
#if 0
    foreach(auto s, chunks){
        if (s.length <= max_size)
            _chunks.push_back(s);
        else{
            UINT64 length = s.length;
            UINT64 next_start = s.start;
            while (length > max_size){
                _chunks.push_back(changed_area(next_start, max_size));
                length -= max_size;
                next_start += max_size;
            }
            _chunks.push_back(changed_area(next_start, length));
        }
    }
    std::sort(_chunks.begin(), _chunks.end(), [](changed_area const& lhs, changed_area const& rhs){ return lhs.start < rhs.start; });
#else
    std::sort(chunks.begin(), chunks.end(), [](changed_area const& lhs, changed_area const& rhs){ return lhs.start < rhs.start ; });
    uint32_t byte_per_bit = 4096;
    uint32_t mini_number_of_bits = 0x100000 / byte_per_bit;
    uint32_t number_of_bits = max_size / byte_per_bit;
    uint64_t size = chunks[chunks.size() - 1].start + chunks[chunks.size() - 1].length;
    uint64_t total_number_of_bits = ((size - 1) / byte_per_bit) + 1;
    uint64_t buff_size = ((total_number_of_bits - 1) / 8) + 1;
    std::auto_ptr<BYTE> buf(new BYTE[buff_size]);
    memset(buf.get(), 0, buff_size);
    foreach(auto s, chunks){
        set_umap(buf.get(), s.start, s.length, byte_per_bit);
    }
    uint64_t newBit, oldBit = 0, next_dirty_bit = 0;
    bool     dirty_range = false;
    for (newBit = 0; newBit < (uint64_t)total_number_of_bits; newBit++){
        if (buf.get()[newBit >> 3] & (1 << (newBit & 7))){
            uint64_t num = (newBit - oldBit);
            if (!dirty_range){
                dirty_range = true;
                oldBit = next_dirty_bit = newBit;
            }
            else if (num == number_of_bits){
                _chunks.push_back(changed_area(oldBit * byte_per_bit, num * byte_per_bit));
                oldBit = next_dirty_bit = newBit;
            }
            else{
                next_dirty_bit = newBit;
            }
        }
        else {
            if (dirty_range && ((newBit - oldBit) >= mini_number_of_bits)){
                uint64_t lenght = (next_dirty_bit - oldBit + 1) * byte_per_bit;
                _chunks.push_back(changed_area(oldBit * byte_per_bit, lenght));
                dirty_range = false;
            }
        }
    }
    if (dirty_range){
        _chunks.push_back(changed_area( oldBit * byte_per_bit, (next_dirty_bit - oldBit + 1) * byte_per_bit));
    }
#endif
    return _chunks;
}

uint64_t clone_disk::get_boundary_of_partitions(universal_disk_rw::ptr rw){
    uint64_t boundary = 0;
    bool result = false;
    std::auto_ptr<BYTE> data_buf(new BYTE[GPT_PART_SIZE_SECTORS * SECTOR_SIZE]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)){
        part_table = (PLEGACY_MBR)data_buf.get();

        if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
        {
            PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[SECTOR_SIZE];
            PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[SECTOR_SIZE * 2];

            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
            {
                uint64_t b = (((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size()) + ((gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size());
                if (boundary < b)
                    boundary = b;
            }
        }
        else
        {
            for (int index = 0; index < 4; index++)
            {
                uint64_t start = ((uint64_t)part_table->PartitionRecord[index].StartingLBA) * rw->sector_size();
                uint64_t length = ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size());
                uint64_t b = start + length;
                if (boundary < b)
                    boundary = b;
            }
        }
    }
    return boundary;
}

bool clone_disk::is_bootable_disk(universal_disk_rw::ptr rw){
    bool bootable_disk = false;
    std::auto_ptr<BYTE> data_buf(new BYTE[GPT_PART_SIZE_SECTORS * rw->sector_size()]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    if (rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)){
        part_table = (PLEGACY_MBR)data_buf.get();
        unsigned __int64 start_lba = ULLONG_MAX;
        if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
        {
            PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[rw->sector_size()];
            PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[rw->sector_size() * 2];

            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
            {
                if (macho::guid_(gpt_part[i].PartitionTypeGuid) == macho::guid_("c12a7328-f81f-11d2-ba4b-00a0c93ec93b")) // EFI System Partition
                {
                    bootable_disk = true;
                    break;
                }
            }
        }
        else
        {
            for (int index = 0; index < 4; index++)
            {
                if (part_table->PartitionRecord[index].BootIndicator)
                {
                    bootable_disk = true;
                    break;
                }
            }
        }
    }
    return bootable_disk;
}
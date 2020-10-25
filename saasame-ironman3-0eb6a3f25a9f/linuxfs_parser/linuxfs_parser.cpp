#include "macho.h"
#include <Windows.h>
#include "..\irm_converter\irm_disk.h"
#include "linuxfs_parser.h"
#include "ext2fs.h"
#include "lvm.h"
#include "xfs.h"

using namespace linuxfs;
using namespace macho;

volume::vtr volume::get(universal_disk_rw::ptr rw){
    std::vector<universal_disk_rw::ptr> rws;
    rws.push_back(rw);
    return get(rws);
}

volume::vtr   volume::get(std::vector<universal_disk_rw::ptr> rws){
    std::string buf;
    volume::vtr results;
    foreach(universal_disk_rw::ptr rw, rws){
        if (rw->read(0, rw->sector_size() * 34, buf)){
            PLEGACY_MBR                pLegacyMBR = (PLEGACY_MBR)&buf[0];
            PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
            if ((pLegacyMBR->Signature == 0xAA55) && (pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
                PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&buf[rw->sector_size() * pGptPartitonHeader->PartitionEntryLBA];
                for (int index = 0; index < pGptPartitonHeader->NumberOfPartitionEntries; index++){
                    if (pGptPartitionEntries[index].StartingLBA && pGptPartitionEntries[index].EndingLBA){
                        macho::guid_ partition_type = macho::guid_(pGptPartitionEntries[index].PartitionTypeGuid);
                        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){}// Linux SWAP partition     
                        else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                            partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                            partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                            partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                            partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                            partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                            )
                        {
                            volume::ptr v = get(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size(), (pGptPartitionEntries[index].EndingLBA - pGptPartitionEntries[index].StartingLBA + 1) * rw->sector_size());
                            if (v) results.push_back(v);
                        }
                    }
                }
            }
            else{
                for (int index = 0; index < 4; index++){
                    uint64_t start = ((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA) * rw->sector_size();
                    if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXT2){
                        volume::ptr v = get(rw, start, ((uint64_t)pLegacyMBR->PartitionRecord[index].SizeInLBA * rw->sector_size()));
                        if (v) results.push_back(v);
                    }
                    else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                        pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXTENDED){
                        std::string buf2;
                        bool loop = true;
                        while (loop){
                            loop = false;
                            if (rw->read(start, rw->sector_size(), buf2)){
                                PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                                if (pMBR->Signature != 0xAA55)
                                    break;
                                for (int i = 0; i < 2; i++){
                                    if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                        pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                        start = ((uint64_t)((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * rw->sector_size());
                                        loop = true;
                                        break;
                                    }
                                    else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                                        if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2){
                                            volume::ptr v = get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size()));
                                            if (v) results.push_back(v);
                                        }
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
        }
    }
    return results;
}

volume::ptr volume::get(universal_disk_rw::ptr& rw, ULONGLONG _offset, ULONGLONG _size){
    volume::ptr v = ext2_volume::get(rw, _offset, _size);
    if (!v)
        v = xfs_volume::get(rw, _offset, _size);
    return v;
}

io_range::map volume::get_file_system_ranges(std::map<std::string, universal_disk_rw::ptr> rws){
    typedef std::map<std::string, universal_disk_rw::ptr> universal_disk_rw_map;
    std::vector< universal_disk_rw::ptr> _rws;
    foreach(universal_disk_rw_map::value_type& rw, rws)
        _rws.push_back(rw.second);
    return get_file_system_ranges(_rws);
}

io_range::map volume::get_file_system_ranges(std::vector<universal_disk_rw::ptr> rws){
    io_range::map file_system_ranges;
    std::string buf;
    volume::vtr results;
    logical_volume_manager lvm;
    foreach(universal_disk_rw::ptr rw, rws){
        if (rw->read(0, rw->sector_size() * 34, buf)){
            PLEGACY_MBR                pLegacyMBR = (PLEGACY_MBR)&buf[0];
            PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
            if ((pLegacyMBR->Signature == 0xAA55) && (pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
                PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&buf[rw->sector_size() * pGptPartitonHeader->PartitionEntryLBA];
                for (int index = 0; index < pGptPartitonHeader->NumberOfPartitionEntries; index++){
                    if (pGptPartitionEntries[index].StartingLBA && pGptPartitionEntries[index].EndingLBA){
                        macho::guid_ partition_type = macho::guid_(pGptPartitionEntries[index].PartitionTypeGuid);
                        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){}// Linux SWAP partition     
                        else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                            partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                            partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                            partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                            partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                            partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                            )
                        {
                            volume::ptr v = get(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size(), (pGptPartitionEntries[index].EndingLBA - pGptPartitionEntries[index].StartingLBA + 1) * rw->sector_size());
                            if (v){
                                io_range::vtr ranges = v->file_system_ranges();
                                file_system_ranges[rw->path()].insert(file_system_ranges[rw->path()].end(), ranges.begin(), ranges.end());
                            }
                        }
                        else if (partition_type == macho::guid_("E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
                        {
                            lvm.scan_pv(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size());
                        }
                    }
                }
            }
            else{
                for (int index = 0; index < 4; index++){
                    uint64_t start = ((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA) * rw->sector_size();
                    if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXT2){
                        volume::ptr v = get(rw, start, ((uint64_t)pLegacyMBR->PartitionRecord[index].SizeInLBA * rw->sector_size()));
                        if (v){
                            io_range::vtr ranges = v->file_system_ranges();
                            file_system_ranges[rw->path()].insert(file_system_ranges[rw->path()].end(), ranges.begin(), ranges.end());
                        }
                    }
                    else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM){
                        lvm.scan_pv(rw, start);
                    }
                    else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                        pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXTENDED){
                        std::string buf2;
                        bool loop = true;
                        while (loop){
                            loop = false;
                            if (rw->read(start, rw->sector_size(), buf2)){
                                PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                                if (pMBR->Signature != 0xAA55)
                                    break;
                                for (int i = 0; i < 2; i++){
                                    if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                        pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                        start = ((uint64_t)((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * rw->sector_size());
                                        loop = true;
                                        break;
                                    }
                                    else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                                        if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2){
                                            volume::ptr v = get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size()));
                                            if (v){
                                                io_range::vtr ranges = v->file_system_ranges();
                                                file_system_ranges[rw->path()].insert(file_system_ranges[rw->path()].end(), ranges.begin(), ranges.end());
                                            }
                                        }
                                        else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM){
                                            lvm.scan_pv(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()));
                                        }
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
        }
    }
    io_range::map lvm_system_ranges = lvm.get_system_ranges();
    foreach(io_range::map::value_type &m, lvm_system_ranges){
        file_system_ranges[m.first].insert(file_system_ranges[m.first].end(), m.second.begin(), m.second.end());
    }
    foreach(io_range::map::value_type& m, file_system_ranges){
        std::sort(m.second.begin(), m.second.end(), io_range::compare());
    }
    return file_system_ranges;
}

lvm_mgmt::groups_map lvm_mgmt::get_groups(std::vector<universal_disk_rw::ptr> rws){
    lvm_mgmt::groups_map groups;
    std::string buf;
    logical_volume_manager lvm;
    foreach(universal_disk_rw::ptr rw, rws){
        if (rw->read(0, rw->sector_size() * 34, buf)){
            PLEGACY_MBR                pLegacyMBR = (PLEGACY_MBR)&buf[0];
            PGPT_PARTITIONTABLE_HEADER pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
            if ((pLegacyMBR->Signature == 0xAA55) && (pLegacyMBR->PartitionRecord[0].PartitionType == 0xEE) && (pGptPartitonHeader->Signature == 0x5452415020494645)){
                PGPT_PARTITIONTABLE_HEADER                     pGptPartitonHeader = (PGPT_PARTITIONTABLE_HEADER)&buf[rw->sector_size()];
                PGPT_PARTITION_ENTRY                           pGptPartitionEntries = (PGPT_PARTITION_ENTRY)&buf[rw->sector_size() * pGptPartitonHeader->PartitionEntryLBA];
                for (int index = 0; index < pGptPartitonHeader->NumberOfPartitionEntries; index++){
                    if (pGptPartitionEntries[index].StartingLBA && pGptPartitionEntries[index].EndingLBA){
                        macho::guid_ partition_type = macho::guid_(pGptPartitionEntries[index].PartitionTypeGuid);
                        if (partition_type == macho::guid_("0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")){}// Linux SWAP partition     
                        else if (partition_type == macho::guid_("0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                            partition_type == macho::guid_("44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                            partition_type == macho::guid_("4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                            partition_type == macho::guid_("933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                            partition_type == macho::guid_("3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                            partition_type == macho::guid_("EBD0A0A2-B9E5-4433-87C0-68B6B72699C7") // MS basic data partition 
                            )
                        {
                        }
                        else if (partition_type == macho::guid_("E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
                        {
                            lvm.scan_pv(rw, ((uint64_t)pGptPartitionEntries[index].StartingLBA) * rw->sector_size());
                        }
                    }
                }
            }
            else{
                for (int index = 0; index < 4; index++){
                    uint64_t start = ((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA) * rw->sector_size();
                    if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXT2){
                    }
                    else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM){
                        lvm.scan_pv(rw, start);
                    }
                    else if (pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
                        pLegacyMBR->PartitionRecord[index].PartitionType == PARTITION_EXTENDED){
                        std::string buf2;
                        bool loop = true;
                        while (loop){
                            loop = false;
                            if (rw->read(start, rw->sector_size(), buf2)){
                                PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
                                if (pMBR->Signature != 0xAA55)
                                    break;
                                for (int i = 0; i < 2; i++){
                                    if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
                                        pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED){
                                        start = ((uint64_t)((uint64_t)pLegacyMBR->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * rw->sector_size());
                                        loop = true;
                                        break;
                                    }
                                    else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0){
                                        if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2){
                                        }
                                        else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM){
                                            lvm.scan_pv(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()));
                                        }
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
        }
    }
    return lvm.get_groups_luns_mapping();
}

lvm_mgmt::groups_map lvm_mgmt::get_groups(std::map<std::string, universal_disk_rw::ptr> rws){
    typedef std::map<std::string, universal_disk_rw::ptr> universal_disk_rw_map;
    std::vector< universal_disk_rw::ptr> _rws;
    foreach(universal_disk_rw_map::value_type& rw, rws)
        _rws.push_back(rw.second);
    return get_groups(_rws);
}
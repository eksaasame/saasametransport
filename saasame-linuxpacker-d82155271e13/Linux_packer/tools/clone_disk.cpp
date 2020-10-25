#include "clone_disk.h"
#include "string_operation.h"
#include "data_type.h"
#include "snapshot.h"
#include "ntfs_parser.h"
#include "ext2fs.h"
#include <string.h>
//#include "sys/swap.h"
#include <unistd.h>
//#define SWAP_HEADER_SIZE (sizeof(struct swap_header_v1_2))
#define EMPTY_CLONE_SIZE 512
#define SHOW_CLONE_DISK_MSG 1
uint64_t clone_disk::get_boundary_of_partitions(universal_disk_rw::ptr rw) {
    uint64_t boundary = 0;
    bool result = false;
    std::unique_ptr<BYTE> data_buf(new BYTE[GPT_PART_SIZE_SECTORS * SECTOR_SIZE]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)) {
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


//changed_area::vtr clone_disk::get_clone_data_range(universal_disk_rw::ptr rw, bool file_system_filter_enable) {
//    changed_area::vtr changed;
//    changed_area::vtr excluded;
//    uint64_t max_size = 1024 * 1024 * 8;
//    //linuxfs::volume::vtr results;
//    logical_volume_manager lvm;
//    bool result = false;
//    std::unique_ptr<BYTE> data_buf(new BYTE[GPT_PART_SIZE_SECTORS * SECTOR_SIZE]);
//    uint32_t number_of_sectors_read = 0;
//    PLEGACY_MBR part_table;
//    boost::uuids::string_generator gen;
//    if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)) {
//        part_table = (PLEGACY_MBR)data_buf.get();
//
//        if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
//        {
//            PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[SECTOR_SIZE];
//            PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[SECTOR_SIZE * 2];
//
//            for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
//            {
//                GUID * partition_type = (GUID *)gpt_part[i].PartitionTypeGuid;
//                //printf("partition_type = %s\r\n", partition_type->to_string().c_str());
//                if (!strcasecmp(partition_type->to_string().c_str(),"0657FD6D-A4AB-43C4-84E5-0933C84B4F4F") || // Linux SWAP partition
//                    !strcasecmp(partition_type->to_string().c_str(),"de94bba4-06d1-4d40-a16a-bfd50179d6ac") || // Microsoft Recovery partition
//                    !strcasecmp(partition_type->to_string().c_str(),"e3c9e316-0b5c-4db8-817d-f92df00215ae"))  // Microsoft reserved partition
//                {
//                }
//                else if (!strcasecmp(partition_type->to_string().c_str(), "af9b60a0-1431-4f62-bc68-3311714a69ad") || // LDM data partition
//                    !strcasecmp(partition_type->to_string().c_str(), "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3"))  // LDM metadata partition
//                {
//                    changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
//                }
//                else if (!strcasecmp(partition_type->to_string().c_str(), "E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
//                {
//                    if (file_system_filter_enable)
//                        lvm.scan_pv(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size());
//                    else
//                    {
//                        changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
//                    }
//                }
//                else if (!strcasecmp(partition_type->to_string().c_str(), "0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
//                    !strcasecmp(partition_type->to_string().c_str(), "44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
//                    !strcasecmp(partition_type->to_string().c_str(), "4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
//                    !strcasecmp(partition_type->to_string().c_str(), "933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
//                    !strcasecmp(partition_type->to_string().c_str(), "3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
//                    !strcasecmp(partition_type->to_string().c_str(), "c12a7328-f81f-11d2-ba4b-00a0c93ec93b") || //EFI system partition
//                    !strcasecmp(partition_type->to_string().c_str(), "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7")) // MS basic data partition 
//                {
//                    /*remove first*/
//                    /*if (file_system_filter_enable) {
//                    ntfs::volume::ptr vol = ntfs::volume::get(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size());
//                    if (vol) {
//                    ntfs::io_range::vtr ranges = vol->file_system_ranges();
//                    for(ntfs::io_range& r: ranges) {
//                    changed.push_back(changed_area(r.start, r.length));
//                    }
//                    changed_area::vtr _excluded = get_windows_excluded_range(vol);
//                    excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
//                    }
//                    else {
//                    linuxfs::volume::ptr v = linuxfs::volume::get(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size());
//                    if (v) {
//                    linuxfs::io_range::vtr ranges = v->file_system_ranges();
//                    for(linuxfs::io_range& r: ranges) {
//                    changed.push_back(changed_area(r.start, r.length));
//                    }
//                    }
//                    else {
//                    changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
//                    }
//                    }
//                    }
//                    else {*/
//                    changed.push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size()));
//                    //}
//                }
//            }
//        }
//        else
//        {
//            for (int index = 0; index < 4; index++)
//            {
//                uint64_t start = ((uint64_t)part_table->PartitionRecord[index].StartingLBA) * rw->sector_size();
//                if (0 == part_table->PartitionRecord[index].SizeInLBA) {
//                }
//                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_IFS) {
//                    /*ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(rw, start) : NULL;
//                    if (v) {
//                    ntfs::io_range::vtr ranges = v->file_system_ranges();
//                    for(ntfs::io_range& r: ranges){
//                    changed.push_back(changed_area(r.start, r.length));
//                    }
//                    changed_area::vtr _excluded = get_windows_excluded_range(v);
//                    excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
//                    }
//                    else{*/
//                    changed.push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())));
//                    //}
//                }
//                else if (part_table->PartitionRecord[index].PartitionType == 82 && part_table->PartitionRecord[index].SizeInLBA != 0) // Linux SWAP partition
//                {
//                }
//                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_EXT2) {
//                    /*linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(rw, start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())) : NULL;
//                    if (v){
//                    linuxfs::io_range::vtr ranges = v->file_system_ranges();
//                    for(linuxfs::io_range& r: ranges){
//                    changed.push_back(changed_area(r.start, r.length));
//                    }
//                    }
//                    else{*/
//                    changed.push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())));
//                    //}
//                }
//                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM) {
//                    lvm.scan_pv(rw, start);
//                }
//                else if (part_table->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
//                    part_table->PartitionRecord[index].PartitionType == PARTITION_EXTENDED)
//                {
//                    std::string buf2;
//                    bool loop = true;
//                    while (loop)
//                    {
//                        loop = false;
//                        if (rw->read(start, rw->sector_size(), buf2))
//                        {
//                            PLEGACY_MBR pMBR = (PLEGACY_MBR)&buf2[0];
//                            if (pMBR->Signature != 0xAA55)
//                                break;
//                            changed.push_back(changed_area(start, rw->sector_size()));
//                            for (int i = 0; i < 2; i++)
//                            {
//                                if (pMBR->PartitionRecord[i].PartitionType == PARTITION_XINT13_EXTENDED ||
//                                    pMBR->PartitionRecord[i].PartitionType == PARTITION_EXTENDED)
//                                {
//                                    start = ((uint64_t)((uint64_t)part_table->PartitionRecord[index].StartingLBA + (uint64_t)pMBR->PartitionRecord[i].StartingLBA) * rw->sector_size());
//                                    loop = true;
//                                    break;
//                                }
//                                else if (pMBR->PartitionRecord[i].StartingLBA != 0 && pMBR->PartitionRecord[i].SizeInLBA != 0)
//                                {
//                                    if (pMBR->PartitionRecord[i].PartitionType == 82)// Linux SWAP partition
//                                    {
//                                    }
//                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_IFS) {
//                                        /*ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size())) : NULL;
//                                        if (v) {
//                                        ntfs::io_range::vtr ranges = v->file_system_ranges();
//                                        for(ntfs::io_range& r: ranges){
//                                        changed.push_back(changed_area(r.start, r.length));
//                                        }
//                                        changed_area::vtr _excluded = get_windows_excluded_range(v);
//                                        excluded.insert(excluded.end(), _excluded.begin(), _excluded.end());
//                                        }
//                                        else{*/
//                                        changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
//                                        //}
//                                    }
//                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2) {
//                                        /*linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())) : NULL;
//                                        if (v){
//                                        linuxfs::io_range::vtr ranges = v->file_system_ranges();
//                                        for(linuxfs::io_range& r: ranges){
//                                        changed.push_back(changed_area(r.start, r.length));
//                                        }
//                                        }
//                                        else{*/
//                                        changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
//                                        //}
//                                    }
//                                    else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM) { //only lvm remain
//                                        if (file_system_filter_enable)
//                                            lvm.scan_pv(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()));
//                                        else
//                                            changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
//                                    }
//                                    else {
//                                        changed.push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())));
//                                    }
//                                }
//                                else
//                                {
//                                    break;
//                                }
//                            }
//                        }
//                    }
//                }
//                else {
//                    changed.push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())));
//                }
//            }
//        }
//    }
//
//    linuxfs::io_range::map lvm_system_ranges = lvm.get_system_ranges();
//    printf("**************************************rw->path() = %s\r\n", rw->path().c_str());
//    for (linuxfs::io_range& r : lvm_system_ranges[rw->path()]) {
//        printf("%llu, %llu\r\n", r.start, r.length);
//        changed.push_back(changed_area(r.start, r.length));
//    }
//    std::sort(changed.begin(), changed.end(), changed_area::compare());
//    std::sort(excluded.begin(), excluded.end(), changed_area::compare());
//    size_t length = 1024 * 1024;
//    if (changed[0].start < length)
//        length = changed[0].start;
//    changed.insert(changed.begin(), changed_area(0, length));
//    return final_changed_area(changed, excluded, max_size);
//}

bool get_physicak_location_of_specify_file_by_command(std::vector<std::pair<uint64_t, uint64_t>> & inextents, const set<string> & filenames, const string & block_device,uint32_t block_size, bool is_xfs, linuxfs::volume::ptr v)
{
    FUNC_TRACER;
    if (!is_xfs) //ext2
    {
        string O_DEBUGFS_PAGER = system_tools::execute_command("echo \"$DEBUGFS_PAGER\"");
        if (O_DEBUGFS_PAGER.size() != 0)
            system_tools::execute_command("export DEBUGFS_PAGER=");

        string O_PAGER = system_tools::execute_command("echo \"$PAGER\"");
        if (O_PAGER.size() != 0)
            system_tools::execute_command("export PAGER=");
        //remove the debugfs_pager and pager environment variable to NULL
        for (auto & filename_ori : filenames)
        {
            string mp = system_tools::get_files_mount_point_by_command(filename_ori);
            int mpindex = filename_ori.find(mp);
            string filename = "/" + filename_ori.substr(mpindex + mp.size(), filename_ori.size() - mp.size());

            string command = "debugfs -R \"stat \\\"" + filename + "\\\"\" " + block_device;
            string result = system_tools::execute_command(command.c_str());
            string target[] = { "EXTENTS:","BLOCKS:" };
            int target_location = std::string::npos;
            int i;
            for (i = 0; i < 2; i++)
            {
                target_location = result.find(target[i]);
                if (target_location != std::string::npos)
                    break;
            }
            if (target_location == std::string::npos)
            {
                LOG_TRACE("%s is not a regular file, skip it.", filename_ori.c_str());
                continue;
            }
            string extents = result.substr(target_location + target[i].size(), result.size());
            LOG_TRACE("extents = %s", extents.c_str());
            if (extents.size())
            {
                vector<string> strVec;
                boost::split(strVec, extents, boost::is_any_of(",:"));
                bool is_index = false;
                bool is_total = false;
                bool is_ext0 = false;
                if (strVec.size() >= 2)
                {
                    for (string & s : strVec)
                    {
                        //LOG_TRACE("s = %s\r\n", s.c_str());
                        if (s.find("IND") != std::string::npos)
                        {
                            is_index = true;
                            continue;
                        }
                        if (s.find("TOTAL") != std::string::npos)
                        {
                            is_total = true;
                            continue;
                        }
                        if (s.find("ETB") != std::string::npos)
                        {
                            is_ext0 = true;
                            continue;
                        }
                        if (s.find("(") == std::string::npos)
                        {
                            if (is_index)
                            {
                                is_index = false;
                                continue;
                            }
                            if (is_total)
                            {
                                is_total = false;
                                continue;
                            }
                            if (is_ext0)
                            {
                                is_ext0 = false;
                                continue;
                            }
                            vector<string> strVec2;
                            boost::split(strVec2, s, boost::is_any_of("-"));
                            if (strVec2.size() == 1)
                            {
                                uint64_t start = std::stoll(strVec2[0], 0, 10) * block_size;
                                inextents.push_back(std::make_pair(start, block_size));
                            }
                            else
                            {
                                uint64_t start = std::stoll(strVec2[0], 0, 10);
                                uint64_t end = std::stoll(strVec2[1], 0, 10);
                                inextents.push_back(std::make_pair(start*block_size, (end - start + 1)*block_size));
                            }
                        }
                        //else //if there are the index of  the  blocks
                        //{
                        //    vector<string> strVec2;
                        //    boost::split(strVec2, s, boost::is_any_of("([]-)"));
                        //    if (strVec2.size() == 4)
                        //    {
                        //        uint64_t start = std::stoll(strVec2[1], 0, 10);
                        //        uint64_t end = std::stoll(strVec2[2], 0, 10);
                        //    }
                        //    else
                        //        uint64_t end = std::stoll(strVec2[1], 0, 10);
                        //}
                    }
                }
            }
            else
            {
                LOG_TRACE("%s's file size is zero.", filename_ori.c_str());
            }
        }
        if (O_DEBUGFS_PAGER.size() != 0)
        {
            string cmd = "export DEBUGFS_PAGER=" + O_DEBUGFS_PAGER;
            system_tools::execute_command(cmd.c_str());
        }
        if (O_PAGER.size() != 0)
        {
            string cmd = "export PAGER=" + O_PAGER;
            system_tools::execute_command(cmd.c_str());
        }
    }
    else
    {
        std::vector<std::pair<uint64_t, uint64_t>> inextents2;        
        string command = "xfs_metadump " + block_device + " /.dattodump";
        string result = system_tools::execute_command(command.c_str());
        if (result.size() != 0)
        {
            LOG_ERROR("%s", result.c_str());
            return false;
        }
        command = "xfs_mdrestore /.dattodump /.dattofs";
        result = system_tools::execute_command(command.c_str());
        if (result.size() != 0)
        {
            LOG_ERROR("%s", result.c_str());
            return false;
        }
        remove("/.dattodump");
        command = "xfs_repair -L /.dattofs";
        result = system_tools::execute_command(command.c_str());
        if (result.size() != 0)
        {
            LOG_ERROR("%s", result.c_str());
            return false;
        }
        for (auto & filename : filenames)
        {
            struct stat var;
            if (-1 == stat(filename.c_str(), &var))
            {
                LOG_ERROR("get %s status error.", filename.c_str());
                return false;
            }
            command = "xfs_db -c \"inode " + std::to_string(var.st_ino) + "\" -c \"bmap\" /.dattofs";
            result = system_tools::execute_command(command.c_str());
            if (result.size() != 0)
            {
                //LOG_TRACE("result = %s", result.c_str());
                uint64_t data_offset = 0;
                uint64_t startblock = 0;
                uint64_t before = 0;
                uint64_t after = 0;
                uint64_t count = 0;
                uint64_t offset = 0;
                const char * result_start = result.c_str();
                int flag = 0;
                std::string line;
                bool got_datta = false;
                while (sscanf(result_start, "data offset %llu startblock %llu (%llu/%llu) count %llu flag %d\n%n", &data_offset, &startblock, &before, &after, &count, &flag, &offset) == 6)
                {
                    got_datta = true;
                    uint64_t blocks = before * v->get_sb_agblocks() + after;
                    result_start += offset;
                    inextents.push_back(std::make_pair(blocks * block_size, count *block_size));
                }
                if (!got_datta)
                {
                    LOG_TRACE("%s's file size is zero.", filename.c_str());
                }
            } //this is the second way to get the block index, in the normal situation should not use this
            else
            {
                command = "xfs_db -c \"blockget -i " + std::to_string(var.st_ino) + "\" /.dattofs | grep / | sed 's/.*block //'";
                result = system_tools::execute_command(command.c_str());
                LOG_TRACE("result = %s", result.c_str());
                vector<string> strVec;
                boost::split(strVec, result, boost::is_any_of("\n"));
                if (v == NULL)
                {
                    remove("/.dattofs");
                    return false;
                }
                uint64_t first_start = 0;
                uint64_t previous_start = 0;
                uint64_t lenght = 0;
                for (string & s : strVec)
                {
                    vector<string> strVec2;
                    boost::split(strVec2, s, boost::is_any_of("/"));
                    /**/

                    if (strVec2.size() == 2)
                    {
                        uint64_t offset = (std::stoll(strVec2[0], 0, 10) * v->get_sb_agblocks() + std::stoll(strVec2[1], 0, 10))* block_size;
                        if (first_start == 0)
                        {
                            first_start = offset;
                            previous_start = offset;
                            lenght = 1;
                        }
                        else
                        {
                            if ((previous_start + block_size) != offset)
                            {
                                inextents.push_back(std::make_pair(first_start, lenght *block_size));
                                LOG_TRACE("2push_back, %llu , %llu", first_start, lenght *block_size);
                                previous_start = first_start = offset;
                                lenght = 1;
                            }
                            else
                            {
                                previous_start = offset;
                                lenght++;
                            }
                        }
                    }
                }
                if (lenght)
                {
                    inextents.push_back(std::make_pair(first_start, lenght *block_size));
                    LOG_TRACE("2push_back, %llu , %llu", first_start, lenght *block_size);
                }
            }
        }
        remove("/.dattofs");
    }
    return true;
}

void directory_traversal(string directory_path, set<string> & files)
{
    boost::filesystem::path ori_path(directory_path);
    if (!boost::filesystem::exists(ori_path))
    {
        return;
    }
    if (boost::filesystem::is_regular_file(ori_path))
    {
        files.insert(ori_path.string().c_str());
        return;
    }
    boost::filesystem::directory_iterator end;
    for (boost::filesystem::directory_iterator pos(directory_path); pos != end; ++pos)
    {
        boost::filesystem::path path(*pos);
        if (boost::filesystem::is_regular_file(path))
        {
            files.insert(path.string().c_str());
        }
        else if (boost::filesystem::is_directory(path))
        {
            directory_traversal(path.string().c_str(), files);
        }
    }
}


changed_area::map clone_disk::get_clone_data_range(universal_disk_rw::vtr rws, storage::ptr str,snapshot_manager::ptr sh, disk::map modify_disks,set<string> & excluded_paths, set<string> & resync_paths,bool file_system_filter_enable, bool b_full) {
    changed_area::map changed;
    changed_area::vtr just_compare;
    changed_area::vtr just_compare_result;
    changed_area::map datto_changed;
    changed_area::map excluded , resync;
    changed_area::map intersection;
    changed_area::map area_union;
    changed_area::map final_result;
    boost::shared_ptr<changed_area::map> result_changed;
    uint64_t max_size = 1024 * 1024 * 8;
    uint64_t between_size = sh->get_merge_size();
    LOG_TRACE("between_size = %llu", between_size);
    //linuxfs::volume::vtr results;
    logical_volume_manager lvm;
    bool result = false;
    std::unique_ptr<BYTE> data_buf(new BYTE[GPT_PART_SIZE_SECTORS * SECTOR_SIZE]);
    uint32_t number_of_sectors_read = 0;
    PLEGACY_MBR part_table;
    uint64_t start = 0, length = 0, src_start = 0;
    set<string> excluded_paths_in_snapshots , resync_paths_in_snapshots, all_mounted_point;
    std::map<string, string> disk_snapshot_mount_point_mapping, disk_snapshot_mount_point_reverse_mapping;
    /*parsing the directory path to file*/
    all_mounted_point = sh->get_all_mount_point();
    for (auto & si : sh->snapshot_map)
    {
        si.second->mount_snapshot();
        disk_snapshot_mount_point_mapping[si.second->get_mounted_point()->get_mounted_on()] = si.second->get_snapshot_mount_point();
        disk_snapshot_mount_point_reverse_mapping[si.second->get_snapshot_mount_point()] = si.second->get_mounted_point()->get_mounted_on();
    }

    for (auto & a : excluded_paths)
    {
        string mp = system_tools::get_files_mount_point(a, all_mounted_point); //change to use the mount point structure to finish this
        int mpindex = a.find(mp);
        string raw_file_name = a.substr(mpindex + mp.size(), a.size() - mp.size());
        excluded_paths_in_snapshots.insert(disk_snapshot_mount_point_mapping[mp] + raw_file_name);
    }

    for (auto & a : resync_paths)
    {
        string mp = system_tools::get_files_mount_point(a, all_mounted_point);
        int mpindex = a.find(mp);
        string raw_file_name = a.substr(mpindex + mp.size(), a.size() - mp.size());
        resync_paths_in_snapshots.insert(disk_snapshot_mount_point_mapping[mp] + raw_file_name);
    }

    set<string> excluded_files,resync_files, excluded_files_in_snapshots, resync_files_in_snapshots;
    for (auto & e : excluded_paths_in_snapshots)
    {
        directory_traversal(e, excluded_files_in_snapshots);
    }
    for (auto & e : resync_paths_in_snapshots)
    {
        directory_traversal(e, resync_files_in_snapshots);
    }

    for (auto & a : excluded_files_in_snapshots)
    {
        string mp = system_tools::get_files_mount_point_by_command(a);
        int mpindex = a.find(mp);
        string raw_file_name = a.substr(mpindex + mp.size(), a.size() - mp.size());
        excluded_files.insert(disk_snapshot_mount_point_reverse_mapping[mp] + raw_file_name);
    }

    for (auto & a : resync_files_in_snapshots)
    {
        string mp = system_tools::get_files_mount_point_by_command(a);
        int mpindex = a.find(mp);
        string raw_file_name = a.substr(mpindex + mp.size(), a.size() - mp.size());
        resync_files.insert(disk_snapshot_mount_point_reverse_mapping[mp] + raw_file_name);
    }

    for (auto & si : sh->snapshot_map)
    {
        si.second->unmount_snapshot();
    }
    map<string, set<string>> excluded_paths_map,resync_paths_map;
    /*before everything, split the excluded paths to block device base's map*/
    for (auto &a : excluded_files)
    {
        string mp = system_tools::get_files_mount_point(a, all_mounted_point);
        int mpindex = a.find(mp);
        string raw_file_name = a.substr(mpindex + mp.size(), a.size() - mp.size());
        if (raw_file_name != ".excluded")
        {
            excluded_paths_map[mp].insert(a);
            LOG_TRACE("excluded_paths_map[%s] = %s", mp.c_str(), a.c_str());
        }
    }
    for (auto &a : resync_files)
    {
        string mp = system_tools::get_files_mount_point(a, all_mounted_point);
        //int mpindex = a.find(mp);
        //string final = "/" + a.substr(mpindex + mp.size(), a.size() - mp.size());
        resync_paths_map[mp].insert(a);
        LOG_TRACE("resync_paths_map[%s] = %s", mp.c_str(), a.c_str());
    }


    for (auto & rw : rws)
    {
        btrfs_snapshot_instance::ptr dbsn = sh->get_btrfs_snapshot_by_offset(rw->path(), 0);
        bool b_full_rw = b_full || (modify_disks.count(rw->path()) > 0) || (dbsn != NULL);
        LOG_TRACE("rw->path() = %s,b_full_rw = %d", rw->path().c_str(), b_full_rw);
        snapshot_instance::ptr dsn = sh->get_snapshot_by_offset(rw->path(), 0);
        universal_disk_rw::ptr disk_rw;
        uint64_t disk_rw_offset = 0;
        uint64_t disk_offset = 0;
        bool b_fs_on_disk = false;
        if (dsn != NULL && dsn->get_block_device_path() != rw->path())
            dsn = NULL;
        if (dsn != NULL)
        {
            disk_rw = dsn->get_src_rw();
        }
        if (disk_rw == NULL)
            disk_rw = rw;
        LOG_TRACE("disk_rw->path() = %s", disk_rw->path().c_str());
        disk_rw_offset = 0;
        if (file_system_filter_enable) {
            ntfs::volume::ptr vol = ntfs::volume::get(disk_rw, disk_rw_offset/*rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size()*/);
            if (vol) {
                b_fs_on_disk = true;
                ntfs::io_range::vtr ranges = vol->file_system_ranges();
                for (ntfs::io_range& r : ranges) {
                    LOG_TRACE("KAHAHA start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", r.start + disk_offset,
                        r.length, r.start, disk_rw->path().c_str());
                    changed[rw->path()].push_back(changed_area(r.start + disk_offset, r.length, r.start, disk_rw/*r.start, r.length,r.start, rw*/));
                }
                //
                /*no count exclude file first*/
                //changed_area::vtr _excluded = get_windows_excluded_range(vol);
                //excluded[rw->path()].insert(excluded[rw->path()].end(), _excluded.begin(), _excluded.end());
            }
            else {
                uint64_t block_size = str->get_instance_from_ab_path(rw->path())->blocks;
                LOG_TRACE("disk_rw->path = %s, disk_rw_offset = % llu, block_size = %llu", disk_rw->path().c_str(), disk_rw_offset, block_size);
                linuxfs::volume::ptr v = linuxfs::volume::get(disk_rw, disk_rw_offset/*rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size()*/, block_size);
                if (v) {
                    b_fs_on_disk = true;
                    linuxfs::io_range::vtr ranges = v->file_system_ranges();
                    for (linuxfs::io_range& r : ranges) {
                        LOG_TRACE("KAHAHA start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", r.start + disk_offset,
                            r.length, r.start, disk_rw->path().c_str());
                        changed[rw->path()].push_back(changed_area(r.start + disk_offset, r.length, r.start, disk_rw/*r.start, r.length,r.start,rw*/));
                    }
                    std::vector<std::pair<uint64_t, uint64_t>> extents;
                    bool is_xfs = (v->get_type() == VOLUME_TYPE_XFS) ? true : false;
                    if (dsn != NULL)
                    {
                        if (b_full_rw)
                        {
                            if (boost::filesystem::exists(boost::filesystem::path(dsn->get_abs_cow_path())))
                            {
                                excluded_paths_map[dsn->get_mounted_point()->get_mounted_on()].insert(dsn->get_abs_cow_path());
                            }
                            if (boost::filesystem::exists(boost::filesystem::path(dsn->get_previous_cow_path())))
                            {
                                excluded_paths_map[dsn->get_mounted_point()->get_mounted_on()].insert(dsn->get_previous_cow_path());
                            }
                            if (get_physicak_location_of_specify_file_by_command(extents, excluded_paths_map[dsn->get_mounted_point()->get_mounted_on()], dsn->get_datto_device(), v->get_block_size(), is_xfs, v))
                            {
                                for (auto & ex : extents)
                                {
                                    LOG_TRACE("excluded start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + disk_offset, ex.second, ex.first, disk_rw->path().c_str());
                                    excluded[rw->path()].push_back(changed_area(ex.first + disk_offset, ex.second, ex.first, disk_rw/*r.start, r.length,r.start,rw*/));
                                }
                            }
                            extents.clear();
                        }
                        if (dsn->get_mounted_point() != NULL && resync_paths_map.count(dsn->get_mounted_point()->get_mounted_on()) != 0)
                        {
                            if (get_physicak_location_of_specify_file_by_command(extents, resync_paths_map[dsn->get_mounted_point()->get_mounted_on()], dsn->get_datto_device(), v->get_block_size(), is_xfs, v))
                            {
                                for (auto & ex : extents)
                                {
                                    LOG_TRACE("resync start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + disk_offset, ex.second, ex.first, disk_rw->path().c_str());
                                    resync[rw->path()].push_back(changed_area(ex.first + disk_offset, ex.second, ex.first, disk_rw/*r.start, r.length,r.start,rw*/));
                                }
                            }
                        }
                        extents.clear();
                    }
                }
                else
                {
                    uint64_t length;
                    disk_baseA::ptr dsk = str->get_instance_from_ab_path(rw->path());
                    if (dsk != NULL)
                        length = dsk->blocks;
                    /*LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path() = %s", start,
                        length, src_start, rw->path().c_str());*/
                    btrfs_snapshot_instance::ptr bsn = sh->get_btrfs_snapshot_by_offset(rw->path(), disk_offset);
                    if (bsn != NULL)
                        resync[rw->path()].push_back(changed_area(0, length, 0, rw));
                }
            }          
        }
        if (b_fs_on_disk)
            continue;
        boost::uuids::string_generator gen;

        if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf.get(), number_of_sectors_read)) {
            part_table = (PLEGACY_MBR)data_buf.get();
            if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
            {
                PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf.get()[SECTOR_SIZE];
                PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf.get()[SECTOR_SIZE * 2];
                for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
                {
                    uint64_t partition_offset = gpt_part[i].StartingLBA * rw->sector_size();
                    uint64_t rw_offset;
                    GUID * partition_type = (GUID *)gpt_part[i].PartitionTypeGuid;
                    snapshot_instance::ptr part_sn_GPT = sh->get_snapshot_by_offset(rw->path(), partition_offset);
                    universal_disk_rw::ptr part_rw;
                    if (part_sn_GPT != NULL)
                        part_rw = part_sn_GPT->get_src_rw();
                    if (part_rw == NULL)
                    {
                        part_rw = rw;
                        rw_offset = partition_offset;
                        partition_offset = 0;
                    }
                    else
                        rw_offset = 0;
                    LOG_TRACE("rw_offset = %llu", rw_offset);
                    LOG_TRACE("partition_offset = %llu", partition_offset);
                    //printf("partition_type = %s\r\n", partition_type->to_string().c_str());
                    if (!strcasecmp(partition_type->to_string().c_str(), "0657FD6D-A4AB-43C4-84E5-0933C84B4F4F")) // Linux SWAP partition
                    {
                        start = ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size();
                        length = getpagesize();//rw->sector_size(); /*just one sector*/
                        src_start = start;
                        LOG_TRACE("GPT start = %llu, lenght = %llu, src_start = %llu, rw->path() = %s", start,
                            length, src_start, rw->path().c_str());
                        changed[rw->path()].push_back(changed_area(start, length, src_start, rw));
                    }
                    else if(!strcasecmp(partition_type->to_string().c_str(), "de94bba4-06d1-4d40-a16a-bfd50179d6ac") || // Microsoft Recovery partition
                        !strcasecmp(partition_type->to_string().c_str(), "e3c9e316-0b5c-4db8-817d-f92df00215ae"))  // Microsoft reserved partition
                    {
                    }
                    else if (!strcasecmp(partition_type->to_string().c_str(), "af9b60a0-1431-4f62-bc68-3311714a69ad") || // LDM data partition
                        !strcasecmp(partition_type->to_string().c_str(), "5808c8aa-7e8f-42e0-85d2-e1e90434cfb3"))  // LDM metadata partition
                    {
                        start = ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size();
                        length = (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size();
                        src_start = start;
                        LOG_TRACE("GPT data start = %llu, lenght = %llu, src_start = %llu, rw->path() = %s", start,
                            length, src_start, rw->path().c_str());
                        changed[rw->path()].push_back(changed_area(start, length, src_start, rw));
                    }
                    else if (!strcasecmp(partition_type->to_string().c_str(), "E6D6D379-F507-44C2-A23C-238F2A3DF928"))// Logical Volume Manager (LVM) partition
                    {
                        /*LOG_TRACE("lvm_pv_scan,start = %llu, lenght = %llu", ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), rw->sector_size());
                        changed[rw->path()].push_back(changed_area(((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), rw->sector_size(), ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), rw));*/
                        LOG_TRACE("GPT data MUMI");
                        lvm.scan_pv(rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size(), b_full_rw);
                    }
                    else if (!strcasecmp(partition_type->to_string().c_str(), "0FC63DAF-8483-4772-8E79-3D69D8477DE4") || // "GNU/Linux filesystem data"
                        !strcasecmp(partition_type->to_string().c_str(), "44479540-F297-41B2-9AF7-D131D5F0458A") || //"Root partition (x86)"
                        !strcasecmp(partition_type->to_string().c_str(), "4F68BCE3-E8CD-4DB1-96E7-FBCAF984B709") || //"Root partition (x86-64)"
                        !strcasecmp(partition_type->to_string().c_str(), "933AC7E1-2EB4-4F13-B844-0E14E2AEF915") || //"GNU/Linux /home"
                        !strcasecmp(partition_type->to_string().c_str(), "3B8F8425-20E0-4F3B-907F-1A25A76F98E8") || //"/srv (server data) partition"
                        !strcasecmp(partition_type->to_string().c_str(), "c12a7328-f81f-11d2-ba4b-00a0c93ec93b") || //EFI system partition
                        !strcasecmp(partition_type->to_string().c_str(), "EBD0A0A2-B9E5-4433-87C0-68B6B72699C7")) // MS basic data partition 
                    {
                        /*remove first*/
                        if (file_system_filter_enable) {
                            ntfs::volume::ptr vol = ntfs::volume::get(part_rw, rw_offset/*rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size()*/);
                            if (vol) {
                                ntfs::io_range::vtr ranges = vol->file_system_ranges();
                                for(ntfs::io_range& r: ranges) {
                                    LOG_TRACE("GPT data start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", r.start + partition_offset,
                                        r.length, r.start, part_rw->path().c_str());
                                    changed[rw->path()].push_back(changed_area(r.start + partition_offset, r.length, r.start, part_rw/*r.start, r.length,r.start, rw*/));
                                }
                                //
                            /*no count exclude file first*/
                            //changed_area::vtr _excluded = get_windows_excluded_range(vol);
                            //excluded[rw->path()].insert(excluded[rw->path()].end(), _excluded.begin(), _excluded.end());
                            }
                            else {
                                LOG_TRACE("GPT part_rw->path = %s, rw_offset = % llu, block_size = %llu", part_rw->path().c_str(), rw_offset, (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size());
                                linuxfs::volume::ptr v = linuxfs::volume::get(part_rw, rw_offset/*rw, ((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size()*/, (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size());
                                if (v) {
                                    linuxfs::io_range::vtr ranges = v->file_system_ranges();
                                    for(linuxfs::io_range& r: ranges) {
                                        LOG_TRACE("GPT start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", r.start + partition_offset,
                                            r.length, r.start, part_rw->path().c_str());
                                        changed[rw->path()].push_back(changed_area(r.start + partition_offset, r.length, r.start, part_rw/*r.start, r.length,r.start,rw*/));
                                    }
                                    std::vector<std::pair<uint64_t, uint64_t>> extents;
                                    LOG_TRACE("(v->get_type() = %d\r\n", v->get_type());
                                    bool is_xfs = (v->get_type() == VOLUME_TYPE_XFS) ? true : false;
                                    if (part_sn_GPT != NULL)
                                    {
                                        if (b_full_rw)
                                        {
                                            if (boost::filesystem::exists(boost::filesystem::path(part_sn_GPT->get_abs_cow_path())))
                                            {
                                                excluded_paths_map[part_sn_GPT->get_mounted_point()->get_mounted_on()].insert(part_sn_GPT->get_abs_cow_path());
                                            }
                                            if (boost::filesystem::exists(boost::filesystem::path(part_sn_GPT->get_previous_cow_path())))
                                            {
                                                excluded_paths_map[part_sn_GPT->get_mounted_point()->get_mounted_on()].insert(part_sn_GPT->get_previous_cow_path());
                                            }
                                            if (get_physicak_location_of_specify_file_by_command(extents, excluded_paths_map[part_sn_GPT->get_mounted_point()->get_mounted_on()], part_sn_GPT->get_datto_device(), v->get_block_size(), is_xfs, v))
                                            {
                                                for (auto & ex : extents)
                                                {
                                                    LOG_TRACE("excluded start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + partition_offset, ex.second, ex.first, part_rw->path().c_str());
                                                    excluded[rw->path()].push_back(changed_area(ex.first + partition_offset, ex.second, ex.first, part_rw/*r.start, r.length,r.start,rw*/));
                                                }
                                            }
                                            extents.clear();
                                        }
                                        if (part_sn_GPT->get_mounted_point() != NULL && resync_paths_map.count(part_sn_GPT->get_mounted_point()->get_mounted_on()) != 0)
                                        {
                                            if (get_physicak_location_of_specify_file_by_command(extents, resync_paths_map[part_sn_GPT->get_mounted_point()->get_mounted_on()], part_sn_GPT->get_datto_device(), v->get_block_size(), is_xfs, v))
                                            {
                                                for (auto & ex : extents)
                                                {
                                                    LOG_TRACE("resync start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + partition_offset, ex.second, ex.first, part_rw->path().c_str());
                                                    resync[rw->path()].push_back(changed_area(ex.first + partition_offset, ex.second, ex.first, part_rw/*r.start, r.length,r.start,rw*/));
                                                }
                                            }
                                        }
                                        extents.clear();
                                    }
                                }
                                else {
                                    start = rw_offset + partition_offset;
                                    length = (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size();
                                    src_start = rw_offset;
                                    LOG_TRACE("GPT start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", start,
                                        length, src_start, part_rw->path().c_str());
                                    changed[rw->path()].push_back(changed_area(start, length, src_start, part_rw));
                                }
                            }
                        }
                        else {
                                start = rw_offset + partition_offset;//((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size();
                                length = (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size();
                                src_start = rw_offset;
                                LOG_TRACE("GPT start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", start,
                                    length, src_start, part_rw->path().c_str());
                                changed[rw->path()].push_back(changed_area(start, length, src_start, part_rw));
                        }
                    }
                    else if (gpt_part[i].StartingLBA && (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1)) {
                        // BIOS boot partition  21686148-6449-6E6F-744E-656564454649                     
                            start = rw_offset + partition_offset;//((uint64_t)gpt_part[i].StartingLBA) * rw->sector_size();
                            length = (gpt_part[i].EndingLBA - gpt_part[i].StartingLBA + 1) * rw->sector_size();
                            src_start = rw_offset + partition_offset;//start;
                            LOG_TRACE("GPT start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", start,
                                length, src_start, part_rw->path().c_str());
                            changed[rw->path()].push_back(changed_area(start, length, src_start, part_rw));
                    }
                }
            }
            else
            {
                for (int index = 0; index < 4; index++)
                {
                    uint64_t partition_offset = (uint64_t)part_table->PartitionRecord[index].StartingLBA * rw->sector_size();
                    uint64_t rw_offset;
                    snapshot_instance::ptr part_sn = sh->get_snapshot_by_offset(rw->path(), partition_offset);
                    universal_disk_rw::ptr part_rw;
                    if (part_sn != NULL)
                        part_rw = part_sn->get_src_rw();
                    if (part_rw == NULL)
                    {
                        part_rw = rw;
                        rw_offset = partition_offset;
                        partition_offset = 0;
                    }
                    else
                        rw_offset = 0;
                    LOG_TRACE("part_table->PartitionRecord[%d].PartitionType = %x", index, part_table->PartitionRecord[index].PartitionType);
                    LOG_TRACE("part_table->PartitionRecord[%d].StartCHS[0] = %x ,StartCHS[1] = %x StartCHS[0] = %x", index, part_table->PartitionRecord[index].StartCHS[0],part_table->PartitionRecord[index].StartCHS[1], part_table->PartitionRecord[index].StartCHS[2]);
                    LOG_TRACE("part_table->PartitionRecord[%d].EndCHS[0] = %x ,EndCHS[1] = %x EndCHS[0] = %x", index, part_table->PartitionRecord[index].EndCHS[0], part_table->PartitionRecord[index].EndCHS[1], part_table->PartitionRecord[index].EndCHS[2]);
                    start = ((uint64_t)part_table->PartitionRecord[index].StartingLBA) * rw->sector_size();
                    if (lvm.scan_pv(rw, start, b_full_rw)) {}
                    else if (0 == part_table->PartitionRecord[index].SizeInLBA) {
                        LOG_TRACE("0 == part_table->PartitionRecord[index].SizeInLBA");
                    }
                    else if (part_table->PartitionRecord[index].PartitionType == PARTITION_IFS) 
                    {
                        LOG_TRACE("PARTITION_IFS start=%llu" , start);
                        ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(part_rw, rw_offset) : NULL;
                        if (v) 
                        {
                            ntfs::io_range::vtr ranges = v->file_system_ranges();
                            for(ntfs::io_range& r: ranges){
                                LOG_TRACE("start = %llu, length = %llu,src_start = %llu,rw->path = %s", r.start + partition_offset
                                    , r.length, r.start, part_rw->path().c_str());
                                changed[rw->path()].push_back(changed_area(r.start + partition_offset, r.length, r.start, part_rw/*r.start, r.length,r.start,rw*/));
                            }
                            /*changed_area::vtr _excluded = get_windows_excluded_range(v);
                            excluded[rw->path()].insert(excluded[rw->path()].end(), _excluded.begin(), _excluded.end());*/
                        }
                        else
                        {
                            length = ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size());
                            src_start = start;
                            LOG_TRACE("start = %llu, length = %llu,src_start = %llu,rw->path = %s", start, length, src_start, rw->path().c_str());
                            changed[rw->path()].push_back(changed_area(start, length, src_start, rw));
                        }
                    }
                    else if (part_table->PartitionRecord[index].PartitionType == 82 && part_table->PartitionRecord[index].SizeInLBA != 0) // Linux SWAP partition
                    {
                        length = getpagesize();//rw->sector_size();/*just one sector*/
                        src_start = start;
                        LOG_TRACE("start = %llu, length = %llu,src_start = %llu,rw->path = %s", start, length, src_start, rw->path().c_str());
                        changed[rw->path()].push_back(changed_area(start, length, src_start, rw));
                    }
                    else if (part_table->PartitionRecord[index].PartitionType == PARTITION_EXT2) {
                        //LOG_TRACE("PARTITION_EXT2 start=%llu", start);
                        //LOG_TRACE("part_rw->sector_size = %llu\r\n", part_rw->sector_size());
                        LOG_TRACE("part_rw->path = %s, rw_offset = % llu, block_size = %llu", part_rw->path().c_str(), rw_offset, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size()));
                        linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(part_rw, rw_offset/*rw, start*/, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size())) : NULL;
                        if (v){
                            linuxfs::io_range::vtr ranges = v->file_system_ranges();
                            for(linuxfs::io_range& r: ranges){
                                LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, part_rw->path() = %s", r.start + partition_offset, r.length, r.start, part_rw->path().c_str());
                                changed[rw->path()].push_back(changed_area(r.start + partition_offset, r.length, r.start, part_rw/*r.start, r.length, r.start,rw*/));
                            }
#if REMOVE_DATTO_COW
                            std::vector<std::pair<uint64_t, uint64_t>> extents;
                            LOG_TRACE("(v->get_type() = %d\r\n", v->get_type());
                            bool is_xfs = (v->get_type() == VOLUME_TYPE_XFS) ? true : false;
                            if (part_sn != NULL)
                            {
                                if (b_full_rw)
                                {
                                    if (boost::filesystem::exists(boost::filesystem::path(part_sn->get_abs_cow_path())))
                                    {
                                        excluded_paths_map[part_sn->get_mounted_point()->get_mounted_on()].insert(part_sn->get_abs_cow_path());
                                    }
                                    if (boost::filesystem::exists(boost::filesystem::path(part_sn->get_previous_cow_path())))
                                    {
                                        excluded_paths_map[part_sn->get_mounted_point()->get_mounted_on()].insert(part_sn->get_previous_cow_path());
                                    }
                                    if (get_physicak_location_of_specify_file_by_command(extents, excluded_paths_map[part_sn->get_mounted_point()->get_mounted_on()], part_sn->get_datto_device(), v->get_block_size(), is_xfs, v))
                                    {
                                        for (auto & ex : extents)
                                        {
                                            LOG_TRACE("excluded start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + partition_offset, ex.second, ex.first, part_rw->path().c_str());
                                            excluded[rw->path()].push_back(changed_area(ex.first + partition_offset, ex.second, ex.first, part_rw/*r.start, r.length,r.start,rw*/));
                                        }
                                    }
                                    extents.clear();
                                }
                                if (part_sn->get_mounted_point() != NULL && resync_paths_map.count(part_sn->get_mounted_point()->get_mounted_on()) != 0)
                                {
                                    if (get_physicak_location_of_specify_file_by_command(extents, resync_paths_map[part_sn->get_mounted_point()->get_mounted_on()], part_sn->get_datto_device(), v->get_block_size(), is_xfs, v))
                                    {
                                        for (auto & ex : extents)
                                        {
                                            LOG_TRACE("resync start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + partition_offset, ex.second, ex.first, part_rw->path().c_str());
                                            resync[rw->path()].push_back(changed_area(ex.first + partition_offset, ex.second, ex.first, part_rw/*r.start, r.length,r.start,rw*/));
                                        }
                                    }
                                }
                                extents.clear();
                            }
#endif
                        }
                        else{
                            //btrfs would enter here
                            length = ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size());
                            src_start = start;
                            LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path() = %s", start,
                                length, src_start, rw->path().c_str());
                            btrfs_snapshot_instance::ptr bsn = sh->get_btrfs_snapshot_by_offset(rw->path(), (partition_offset == 0)?rw_offset: partition_offset);
                            if (bsn != NULL)
                            {
                                LOG_TRACE("GO resync");
                                resync[rw->path()].push_back(changed_area(start, length, src_start, rw));
                            }
                            else
                            {
                                LOG_TRACE("GO changed");
                                changed[rw->path()].push_back(changed_area(start, length, src_start, rw));
                            }
                        }
                    }
                    //else if (part_table->PartitionRecord[index].PartitionType == PARTITION_LINUX_LVM) {
                        /*LOG_TRACE("lvm_pv_scan");
                        LOG_TRACE("lvm_pv_scan,start = %llu, lenght = %llu", start, rw->sector_size());
                        changed[rw->path()].push_back(changed_area(start, rw->sector_size(), start, rw));*/
                        //LOG_TRACE("MUMI");
                        //lvm.scan_pv(rw, start);
                    //}
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
                                if (b_full_rw)
                                {
                                    LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path() = %s", start,
                                        rw->sector_size(), start, rw->path().c_str());
                                    changed[rw->path()].push_back(changed_area(start, rw->sector_size(), start, rw));
                                }
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
                                        uint64_t MBR_partition_offset = start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size());
                                        uint64_t MBR_rw_offset;
                                        snapshot_instance::ptr MBR_sn = sh->get_snapshot_by_offset(rw->path(), MBR_partition_offset);
                                        universal_disk_rw::ptr MBR_part_rw = NULL;
                                        if (MBR_sn != NULL)
                                            MBR_part_rw = MBR_sn->get_src_rw();
                                        if (MBR_part_rw == NULL)
                                        {
                                            MBR_part_rw = rw;
                                            MBR_rw_offset = MBR_partition_offset;
                                            MBR_partition_offset = 0;
                                        }
                                        else
                                            MBR_rw_offset = 0;
                                        LOG_TRACE("MUMI");
                                        if (lvm.scan_pv(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), b_full_rw)) {}
                                        else if (pMBR->PartitionRecord[i].PartitionType == 82)// Linux SWAP partition
                                        {
                                            LOG_TRACE("SWAP: start = %llu, length = %llu", start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), rw->sector_size());
                                            changed[rw->path()].push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), /*rw->sector_size()*/getpagesize(), 0, rw));
                                        }
                                        else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_IFS) {

                                           
                                            ntfs::volume::ptr v = file_system_filter_enable ? ntfs::volume::get(MBR_part_rw, MBR_rw_offset/*rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size())*/) : NULL;
                                            if (v) 
                                            {
                                                ntfs::io_range::vtr ranges = v->file_system_ranges();
                                                for(ntfs::io_range& r: ranges)
                                                {
                                                    LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, MBR_part_rw->path() = %s", r.start + MBR_partition_offset,
                                                        r.length, r.start, MBR_part_rw->path().c_str());
                                                    changed[rw->path()].push_back(changed_area(r.start + MBR_partition_offset, r.length, r.start, MBR_part_rw/*r.start, r.length,r.start,rw*/));
                                                }
                                                /*changed_area::vtr _excluded = get_windows_excluded_range(v);
                                                excluded[rw->path()].insert(excluded[rw->path()].end(), _excluded.begin(), _excluded.end());*/
                                            }
                                            else{
                                                LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path() = %s", start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()),
                                                    ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size()), 0, rw->path().c_str());
                                                changed[rw->path()].push_back(changed_area(start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()), ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size()), 0, rw));
                                            }
                                        }
                                        else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_EXT2) {
                                            LOG_TRACE("MBR_part_rw->path = %s, MBR_rw_offset = % llu, block_size = %llu", MBR_part_rw->path().c_str(), MBR_rw_offset, ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size()));
                                            linuxfs::volume::ptr v = file_system_filter_enable ? linuxfs::volume::get(MBR_part_rw, MBR_rw_offset/*rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size())*/, ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size())) : NULL;
                                            if (v){
                                                linuxfs::io_range::vtr ranges = v->file_system_ranges();
                                                for(linuxfs::io_range& r: ranges)
                                                {
                                                    LOG_TRACE(" start = %llu , length = %llu , src_start = %llu, part_rw = %s", r.start + MBR_partition_offset, r.length, r.start, MBR_part_rw->path().c_str());
                                                    changed[rw->path()].push_back(changed_area(r.start + MBR_partition_offset, r.length, r.start, MBR_part_rw/*r.start, r.length, r.start,rw*/));
                                                }
#if REMOVE_DATTO_COW
                                                std::vector<std::pair<uint64_t, uint64_t>> extents;
                                                bool is_xfs = (v->get_type() == VOLUME_TYPE_XFS) ? true : false;
                                                if (part_sn != NULL)
                                                {
                                                    if (b_full_rw)
                                                    {
                                                        if (boost::filesystem::exists(boost::filesystem::path(part_sn->get_abs_cow_path())))
                                                        {
                                                            excluded_paths_map[part_sn->get_mounted_point()->get_mounted_on()].insert(part_sn->get_abs_cow_path());
                                                        }
                                                        if (boost::filesystem::exists(boost::filesystem::path(part_sn->get_previous_cow_path())))
                                                        {
                                                            excluded_paths_map[part_sn->get_mounted_point()->get_mounted_on()].insert(part_sn->get_previous_cow_path());
                                                        }
                                                        if (get_physicak_location_of_specify_file_by_command(extents, excluded_paths_map[part_sn->get_mounted_point()->get_mounted_on()], part_sn->get_datto_device(), v->get_block_size(), is_xfs, v))
                                                        {
                                                            for (auto & ex : extents)
                                                            {
                                                                //LOG_TRACE("start = %llu , length = %llu\r\n", ex.first, ex.second);
                                                                LOG_TRACE("excluded start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + partition_offset, ex.second, ex.first, part_rw->path().c_str());
                                                                excluded[rw->path()].push_back(changed_area(ex.first + partition_offset, ex.second, ex.first, part_rw/*r.start, r.length,r.start,rw*/));
                                                            }
                                                        }
                                                        extents.clear();
                                                    }
                                                    if (part_sn->get_mounted_point()!=NULL && resync_paths_map.count(part_sn->get_mounted_point()->get_mounted_on()) != 0)
                                                    {
                                                        if (get_physicak_location_of_specify_file_by_command(extents, resync_paths_map[part_sn->get_mounted_point()->get_mounted_on()], part_sn->get_datto_device(), v->get_block_size(), is_xfs, v))
                                                        {
                                                            for (auto & ex : extents)
                                                            {
                                                                LOG_TRACE("resync start = %llu , length = %llu , src_start = %llu, part_rw = %s", ex.first + partition_offset, ex.second, ex.first, part_rw->path().c_str());
                                                                resync[rw->path()].push_back(changed_area(ex.first + partition_offset, ex.second, ex.first, part_rw/*r.start, r.length,r.start,rw*/));
                                                            }
                                                        }
                                                    }
                                                    extents.clear();
                                                }
#endif
                                            }
                                            else{
                                                uint64_t tempstart = start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size());
                                                length = ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size());
                                                src_start = tempstart;
                                                LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path().c_str()", tempstart, length, src_start, rw->path().c_str());
                                                btrfs_snapshot_instance::ptr bsn = sh->get_btrfs_snapshot_by_offset(rw->path(), (MBR_partition_offset==0)? MBR_rw_offset : MBR_partition_offset);
                                                if (bsn != NULL)
                                                {
                                                    LOG_TRACE("GO resync");
                                                    resync[rw->path()].push_back(changed_area(tempstart, length, src_start, rw));
                                                }
                                                else
                                                {
                                                    LOG_TRACE("GO changed");
                                                    changed[rw->path()].push_back(changed_area(tempstart, length, src_start, rw));
                                                }
                                            }
                                        }
                                        //else if (pMBR->PartitionRecord[i].PartitionType == PARTITION_LINUX_LVM) { //only lvm remain
                                            /*LOG_TRACE("start = %llu, lvm_pv_scan: StartingLBA = %llu,rw->path = %s" ,start, ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()) ,rw->path().c_str());
                                            uint64_t tempstart = start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size());
                                            length = rw->sector_size();
                                            src_start = tempstart;
                                            changed[rw->path()].push_back(changed_area(tempstart, length, src_start, rw));*/
                                            //LOG_TRACE("MUMI");
                                            //lvm.scan_pv(rw, start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size()));
                                        //}
                                        else 
                                        {
                                            uint64_t tempstart = start + ((uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size());
                                            length = ((uint64_t)pMBR->PartitionRecord[i].SizeInLBA * rw->sector_size());
                                            src_start = tempstart;
                                            LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path().c_str()", tempstart, length, src_start, rw->path().c_str());
                                            changed[rw->path()].push_back(changed_area(tempstart, length, src_start, rw));
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
                    else {
                        length = ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size());
                        src_start = start;
                        LOG_TRACE("start = %llu, lenght = %llu, src_start = %llu, rw->path().c_str() = %s", start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size()), src_start, rw->path().c_str());
                        changed[rw->path()].push_back(changed_area(start, ((uint64_t)part_table->PartitionRecord[index].SizeInLBA * rw->sector_size()), src_start, rw));
                    }
                }
            }
        }
    }

    LOG_TRACE("finished!");
    for (auto & rw : rws)
    {
        bool b_full_rw = b_full || (modify_disks.count(rw->path()) > 0);
        /*first do datto*/
        if (!b_full_rw)
        {
            snapshot_instance::vtr sns = sh->enumerate_snapshot_by_disk_ab_path(rw->path());
            for (auto & sn : sns)
            {
                if (sn->get_block_device_offset() != 0)
                {
                    changed_area::vtr temp = sn->get_changed_area(b_full);
                    datto_changed[rw->path()].insert(datto_changed[rw->path()].end(), temp.begin(), temp.end());
                }
            }
            std::sort(changed[rw->path()].begin(), changed[rw->path()].end(), changed_area::compare());
            system_tools::regions_vectors_intersection<changed_area, changed_area::vtr, changed_area::vtr::iterator>(datto_changed[rw->path()], changed[rw->path()], intersection[rw->path()]);
            datto_changed[rw->path()].clear();
        }
        else
            intersection[rw->path()] = changed[rw->path()];
        changed[rw->path()].clear();



        if (resync.count(rw->path()))
        {
            std::sort(resync[rw->path()].begin(), resync[rw->path()].end(), changed_area::compare());
            system_tools::regions_vectors_union<changed_area, changed_area::vtr, changed_area::vtr::iterator>(intersection[rw->path()], resync[rw->path()], area_union[rw->path()]);
            resync[rw->path()].clear();
        }
        else
        {
            area_union[rw->path()] = intersection[rw->path()];
        }
        intersection[rw->path()].clear();
        std::sort(excluded[rw->path()].begin(), excluded[rw->path()].end(), changed_area::compare());
        system_tools::regions_vectors_complement<changed_area, changed_area::vtr, changed_area::vtr::iterator>(area_union[rw->path()], excluded[rw->path()], final_result[rw->path()]);
        excluded[rw->path()].clear();
    }
    LOG_TRACE("change area merge finished");
    excluded.clear();
    changed.clear();
    resync.clear();
    area_union.clear();
    intersection.clear();
    result_changed = boost::make_shared<changed_area::map>(final_result);
//    }
//    else
//    {
//#if REMOVE_DATTO_COW
//        for (auto & rw : rws)
//        {
//            std::sort(excluded[rw->path()].begin(), excluded[rw->path()].end(), changed_area::compare());
//        }
//        system_tools::regions_vectors_maps_complement<changed_area, changed_area::vtr, changed_area::vtr::iterator, changed_area::map>(changed, excluded, intersection);
//
//        excluded.clear();
//        result_changed = boost::make_shared<changed_area::map>(intersection);
//#else
//        /*don't do "and" just use the original */
//        result_changed = boost::make_shared<changed_area::map>(changed);
//#endif
//    }
    /*after this the change is here*/
#if SHOW_CLONE_DISK_MSG
    for (auto & ca : (*result_changed.get()))
    {
        LOG_TRACE("!!ca.first = %s\r\n", ca.first.c_str());
        uint64_t size = 0;
        for (auto & c : ca.second)
        {
            size += c.length;
            LOG_TRACE("!!c.start = %llu , c.src_start = %llu, c.length = %llu, c._rw->path = %s\r\n", c.start, c.src_start, c.length,c._rw->path().c_str());
            //printf("ca.second.start = %llu\r\n, ca.second.lenght = %llu\r\n", c.start, c.length);
        }
        LOG_TRACE("size = %llu\r\n", size);
    }
#endif
    linuxfs::io_range::map lvm_system_ranges = lvm.get_system_ranges(sh , excluded_paths_map, resync_paths_map,b_full, file_system_filter_enable);
    for (auto & rw : rws)
    {
        LOG_TRACE("**************************************rw->path() = %s\r\n", rw->path().c_str());
        for (linuxfs::io_range& r : lvm_system_ranges[rw->path()]) {
            LOG_TRACE("%llu, %llu, %llu, %s\r\n", r.start, r.length, r.src_start, r._rw->path().c_str());
            (*result_changed.get())[rw->path()].push_back(changed_area(r.start, r.length, r.src_start, r._rw));
        }
        std::sort((*result_changed.get())[rw->path()].begin(), (*result_changed.get())[rw->path()].end(), changed_area::compare());
        std::sort(excluded[rw->path()].begin(), excluded[rw->path()].end(), changed_area::compare());
        
        size_t length = 1024 * 1024;
        if (!(*result_changed.get())[rw->path()].empty() && (*result_changed.get())[rw->path()][0].start < length)
            length = (*result_changed.get())[rw->path()][0].start;
        if(length>0)
            (*result_changed.get())[rw->path()].insert((*result_changed.get())[rw->path()].begin(), changed_area(0, length, 0, rw));
    }
#if SHOW_CLONE_DISK_MSG
    for (auto & ca : (*result_changed.get()))
    {
        LOG_TRACE("5566ca.first = %s\r\n", ca.first.c_str());
        uint64_t size = 0;
        for (auto & c : ca.second)
        {
            size += c.length;
            LOG_TRACE("5566c.start = %llu , c.src_start = %llu, c.length = %llu, c._rw->path = %s\r\n", c.start, c.src_start, c.length, c._rw->path().c_str());
            //printf("ca.second.start = %llu\r\n, ca.second.lenght = %llu\r\n", c.start, c.length);
        }
        LOG_TRACE("5566size = %llu\r\n", size);
    }
#endif
    for (auto & rw : rws)
    {
        (*result_changed.get())[rw->path()] = final_changed_area((*result_changed.get())[rw->path()], excluded[rw->path()], max_size, between_size);
    }
    /*after final modify to the dato and file system*/
#if SHOW_CLONE_DISK_MSG

    for (auto & ca : (*result_changed.get()))
    {
        LOG_TRACE("after final change ca.first = %s\r\n", ca.first.c_str());
        uint64_t size = 0;
        for (auto & c : ca.second)
        {
            size += c.length;
            LOG_TRACE("after final change c.start = %llu , c.src_start = %llu, c.length = %llu, c._rw->path = %s\r\n", c.start, c.src_start, c.length, c._rw->path().c_str());
        }
        LOG_TRACE("after final change size = %llu\r\n", size);
    }
#endif

    if(b_full)
        modify_change_area_with_datto_snapshot((*result_changed.get()), sh, modify_disks,b_full);
#if SHOW_CLONE_DISK_MSG
    for (auto & ca : (*result_changed.get()))
    {
        LOG_TRACE("after add datto partition ca.first = %s\r\n", ca.first.c_str());
        uint64_t size = 0;
        for (auto & c : ca.second)
        {
            size += c.length;
            LOG_TRACE("after add datto partition c.start = %llu , c.src_start = %llu, c.length = %llu, c._rw->path = %s\r\n", c.start, c.src_start, c.length, c._rw->path().c_str());
        }
        LOG_TRACE("after add datto partitionsize = %llu\r\n", size);
    }
#endif
    return (*result_changed.get());
}

struct rw_and_offset_memory
{
    rw_and_offset_memory(uint64_t h, uint64_t l, int64_t o, universal_disk_rw::ptr _rw, std::string _dst = "") :local_h(h), local_l(l), offset(o), rw(_rw), dst_device(_dst){}
    uint64_t local_h;
    uint64_t local_l;
    int64_t offset;
    universal_disk_rw::ptr rw;
    std::string dst_device;
};


changed_area::vtr clone_disk::final_changed_area(changed_area::vtr& src, changed_area::vtr& excludes, uint64_t max_size, uint64_t between_size) {
    changed_area::vtr chunks;
    if (src.empty())
        return src;
    else if (excludes.empty())
    {
        chunks = src;
    }
    else
    {
        std::sort(src.begin(), src.end(), [](changed_area const& lhs, changed_area const& rhs) { return lhs.start < rhs.start; });
        std::sort(excludes.begin(), excludes.end(), [](changed_area const& lhs, changed_area const& rhs) { return lhs.start < rhs.start; });

        changed_area::vtr::iterator it = excludes.begin();
        uint64_t start = 0;
        uint64_t src_start = 0;
        uint64_t offset = 0;

        for (auto e : src)
        {
            LOG_DEBUG("s/%I64u:%I64u", e.start, e.length);
            start = e.start;
            src_start = e.src_start;
            offset = start - src_start;

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
                                chunk.src_start = src_start;
                                chunk.length = it->start - start;
                                chunk._rw = e._rw;
                                chunks.push_back(chunk);
                                LOG_DEBUG("t/%I64u:%I64u", chunk.start, chunk.length);
                            }

                            if (it->start + it->length < e.start + e.length)
                            {
                                start = it->start + it->length;
                                src_start = offset + start;
                                it++;
                                if (it == excludes.end() || it->start >(e.start + e.length))
                                {
                                    chunk.start = start;
                                    chunk.src_start = src_start;
                                    chunk.length = e.start + e.length - start;
                                    chunk._rw = e._rw;
                                    chunks.push_back(chunk);
                                    LOG_DEBUG("t/%I64u:%I64u", chunk.start, chunk.length);
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
                    LOG_DEBUG("t/%I64u:%I64u", e.start, e.length);
                }
            }
            else
            {
                chunks.push_back(e);
                LOG_DEBUG("t/%I64u:%I64u", e.start, e.length);
            }
        }
    }
    std::sort(chunks.begin(), chunks.end(), [](changed_area const& lhs, changed_area const& rhs) { return lhs.start < rhs.start; });
    changed_area::vtr _chunks;
    if (between_size != 0)
    {
        changed_area pre_ca;
        for (auto s : chunks) {
            //LOG_TRACE("s.length = %llu, s.start = %llu, s.src_start = %llu", s.length, s.start, s.src_start);
            if (pre_ca._rw!=NULL && pre_ca._rw->path() == s._rw->path())
            {
                if ( (pre_ca.start + pre_ca.length + between_size > s.start) && (s.start + s.length - pre_ca.start <= max_size) )
                {
                    s.length = s.start + s.length - pre_ca.start;
                    s.start = pre_ca.start;
                    s.src_start = pre_ca.src_start;
                }
                else
                {
                    _chunks.push_back(pre_ca);
                }
            }
            else
            {
                if (!pre_ca.isEmpty())
                    _chunks.push_back(pre_ca);
            }
            pre_ca = s;
        }
        _chunks.push_back(pre_ca);
    }
    else {
        _chunks = chunks;
    }
    chunks.clear();
    std::sort(_chunks.begin(), _chunks.end(), [](changed_area const& lhs, changed_area const& rhs) { return lhs.start < rhs.start; });
#if 1
    for (auto s : _chunks) {
        if (s.length <= max_size)
            chunks.push_back(s);
        else {
            uint64_t length = s.length;
            uint64_t next_start = s.start;
            uint64_t next_src_start = s.src_start;
            universal_disk_rw::ptr temp_rw = s._rw;

            while (length > max_size) {
                chunks.push_back(changed_area(next_start, max_size, next_src_start, temp_rw));
                length -= max_size;
                next_start += max_size;
                next_src_start += max_size;
            }
            chunks.push_back(changed_area(next_start, length, next_src_start, temp_rw));
        }
    }
    _chunks.clear();
    std::sort(chunks.begin(), chunks.end(), [](changed_area const& lhs, changed_area const& rhs) { return lhs.start < rhs.start; });
#else
    std::vector<rw_and_offset_memory> memories;
    std::vector<rw_and_offset_memory>::iterator i_memories, i_memoriesp1;
    universal_disk_rw::ptr temp_rw = NULL;
    std::sort(chunks.begin(), chunks.end(), [](changed_area const& lhs, changed_area const& rhs) { return lhs.start < rhs.start; });
    uint32_t byte_per_bit = 4096;
    uint32_t mini_number_of_bits = 0x100000 / byte_per_bit;
    uint32_t number_of_bits = max_size / byte_per_bit;
    uint64_t size = chunks[chunks.size() - 1].start + chunks[chunks.size() - 1].length;
    uint64_t total_number_of_bits = ((size - 1) / byte_per_bit) + 1;
    uint64_t buff_size = ((total_number_of_bits - 1) / 8) + 1;
    std::unique_ptr<BYTE> buf(new BYTE[buff_size]);
    memset(buf.get(), 0, buff_size);
    for (auto s : chunks) {
        if (temp_rw != s._rw)
        {
            temp_rw = s._rw;
            int64_t offset = s.start - s.src_start;
            memories.push_back(rw_and_offset_memory((s.start/ byte_per_bit)>>3,(1<< ((s.start/ byte_per_bit) & 7)), offset, temp_rw));
            printf("local_h = %llu , local_l = %llu , offset = %lld, temp_rw->path().c_str()", (s.start / byte_per_bit) >> 3, (1 << ((s.start / byte_per_bit) & 7)), offset, temp_rw->path().c_str());
        }
        set_umap(buf.get(), s.start, s.length, byte_per_bit);
    }
    if (!memories.empty())
    {
        i_memories = memories.begin();
        i_memoriesp1 = i_memories + 1;
    }
    uint64_t newBit, oldBit = 0, next_dirty_bit = 0;
    bool     dirty_range = false;
    universal_disk_rw::ptr t_rw = chunks[0]._rw;
    for (newBit = 0; newBit < (uint64_t)total_number_of_bits; newBit++) {
        uint64_t h = newBit >> 3, l = 1 << (newBit & 7);
        if (buf.get()[h] & (l)) {
            uint64_t num = (newBit - oldBit);
            if (!dirty_range) {
                dirty_range = true;
                oldBit = next_dirty_bit = newBit;
            }
            else if (num == number_of_bits) {
                uint64_t start = oldBit * byte_per_bit;
                uint64_t length = num * byte_per_bit;
                uint64_t src_start = start - i_memories->offset;
                _chunks.push_back(changed_area(start, length, src_start, i_memories->rw));
                printf("_chunks start = %llu,length = %llu,src_start = %llu,i_memories->rw->path().c_str() = %s\r\n", start, length, src_start, i_memories->rw->path().c_str());
                oldBit = next_dirty_bit = newBit;
            }
            else {
                next_dirty_bit = newBit;
            }
        }
        else {
            if (dirty_range && ((newBit - oldBit) >= mini_number_of_bits)) {
                uint64_t start = oldBit * byte_per_bit;
                uint64_t length = (next_dirty_bit - oldBit + 1) * byte_per_bit;
                uint64_t src_start = start - i_memories->offset;
                _chunks.push_back(changed_area(start, length, src_start, i_memories->rw));
                printf("_chunks start = %llu,length = %llu,src_start = %llu,i_memories->rw->path().c_str() = %s\r\n", start, length, src_start, i_memories->rw->path().c_str());
                dirty_range = false;
            }
        }
        if (i_memoriesp1 != memories.end())
        {
            if (l >= i_memoriesp1->local_l && h >= i_memoriesp1->local_h)
            {
                printf("change!!\r\n");
                if (dirty_range && oldBit!= newBit)
                {
                    uint64_t start = oldBit * byte_per_bit;
                    uint64_t length = (next_dirty_bit - oldBit + 1) * byte_per_bit;
                    uint64_t src_start = start - i_memories->offset;
                    _chunks.push_back(changed_area(start, length, src_start, i_memories->rw));
                    printf("_chunks start = %llu,length = %llu,src_start = %llu,i_memories->rw->path().c_str() = %s\r\n", start, length, src_start, i_memories->rw->path().c_str());
                    dirty_range = false;
                }
                ++i_memories;
                ++i_memoriesp1;
            }
        }
    }
    if (dirty_range) {
        uint64_t start = oldBit * byte_per_bit;
        uint64_t lenght = (next_dirty_bit - oldBit + 1) * byte_per_bit;
        _chunks.push_back(changed_area(start, lenght, start, t_rw));
    }
#endif
    return chunks;
}

//bool clone_disk::modify_change_area_with_datto_snapshot(changed_area::map & cam, logical_volume_manager & lvm)

bool clone_disk::modify_change_area_with_datto_snapshot(changed_area::map & cam, snapshot_manager::ptr sh, disk::map modify_disks, bool b_full)
{
    FUNC_TRACER;
    linux_storage::storage::ptr str = linux_storage::storage::get_storage();
    snapshot_instance::vtr sns;
    for (auto & cas : cam)
    {
        bool b_full_rw = b_full || (modify_disks.count(cas.first) > 0);
        if (b_full_rw)
        {
            LOG_TRACE("cas.first = %s\r\n", cas.first.c_str());
            sns = sh->enumerate_snapshot_by_disk_ab_path(cas.first);
            for (auto & sn : sns)
            {
                LOG_TRACE("sn->path = %s", sn->get_block_device_path().c_str());
                LOG_TRACE("sn->get_block_device_offset() = %llu", sn->get_block_device_offset());
                if (!sn->get_lvm())
                {
                    uint64_t offset;
                    uint64_t end;
                    linux_storage::disk_baseA::ptr db = str->get_instance_from_ab_path(sn->get_block_device_path());
                    if (db->block_type == linux_storage::disk_baseA::block_dev_type::disk)
                    {
                        linux_storage::disk::ptr d = boost::static_pointer_cast<linux_storage::disk>(db);
                        offset = 0;
                        end = offset + d->blocks;
                    }
                    else
                    {
                        linux_storage::partitionA::ptr p = boost::static_pointer_cast<linux_storage::partitionA>(db);
                        offset = p->partition_start_offset;
                        end = offset + p->size();
                    }
                    changed_area::vtr::iterator ica;
                    for (ica = cas.second.begin(); ica != cas.second.end() /*&& ica->start < end*/;)
                    {

                        uint64_t iend = ica->start + ica->length;
                        uint64_t temp_length;
                        if (ica->start == ica->src_start)
                        {
                            if (ica->start<offset && iend>offset && iend <= end)
                            {
                                /*update ica's data first*/
                                temp_length = ica->length;
                                ica->length = offset - ica->start;

                                ica = cas.second.insert(ica + 1, changed_area(offset, temp_length - ica->length, 0, sn->get_src_rw()));
                            }
                            else if (ica->start >= offset && iend <= end)
                            {
                                ica->src_start = ica->start - offset;
                                ica->_rw = sn->get_src_rw();
                            }
                            else if (ica->start < end && iend >end)
                            {
                                ica->src_start = ica->start - offset;
                                temp_length = ica->length;
                                ica->length = end - ica->start;
                                universal_disk_rw::ptr tmp_rw = ica->_rw;
                                ica->_rw = sn->get_src_rw();
                                ica = cas.second.insert(ica + 1, changed_area(end, iend - end, end, tmp_rw));
                            }
                            else if (ica->start >= end)
                            {
                                break;
                            }
                        }
                        ++ica;
                    }
                }
            }
        }
    }
    return true;
}


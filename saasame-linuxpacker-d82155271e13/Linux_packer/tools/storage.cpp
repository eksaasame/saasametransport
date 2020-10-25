#include "storage.h"
#include "data_type.h"
#include "linuxfs_parser.h"
namespace linux_storage {
    class disk;
    void disk_baseA::set_disk_base(disk_baseA::ptr input)
    {
        major = input->major;
        minor = input->minor;
        disk_index = input->disk_index;
        blocks = input->blocks;
        device_filename = input->device_filename;
        ab_path = input->ab_path;
        sysfs_path = input->sysfs_path;
        block_type = input->block_type;
        mp = input->mp;
        filesystem_type = input->filesystem_type;
        metadata = input->metadata;
    }
    void disk_baseA::print_data()
    {
        LOG_TRACE("major = %d\r\n", major);
        LOG_TRACE("minor = %d\r\n", minor);
        LOG_TRACE("disk_index = %d\r\n", disk_index);
        LOG_TRACE("blocks = %llu\r\n", blocks);
        LOG_TRACE("device_filename = %s\r\n", device_filename.c_str());
        LOG_TRACE("ab_path = %s\r\n", ab_path.c_str());
        LOG_TRACE("sysfs_path = %s\r\n", sysfs_path.c_str());
    }

    bool disk_baseA::auto_setting_block_device_type()
    {
        struct stat st;
        int fd0 = open(sysfs_path.c_str(), O_RDONLY);
        if (!fd0)
        {
            LOG_ERROR("open %s error\r\n", sysfs_path.c_str());
            return false;
        }
        int rc = fstatat(fd0, "partition", &st, 0);
        int rc2 = fstatat(fd0, "device", &st, 0);
        int rc3 = fstatat(fd0, "dm", &st, 0);
        close(fd0);
        if (metadata.is_mpath_partition)
        {
            block_type = disk_baseA::partition;
        }
        else
        {
            if (rc == 0)
            {
                block_type = disk_baseA::partition;
            }
            else if (rc2 == 0)
            {
                block_type = disk_baseA::disk;
            }
            else if (rc3 == 0)
            {
                block_type = disk_baseA::lvm;
            }
        }
        return true;
    }

    bool partitionA::auto_setting_partition_offset_by_sys_path()
    {
        uint64_t offset = 0;
        /*get the start*/
        std::string start_path = sysfs_path + "/start";
        FILE * fp = fopen(start_path.c_str(), "r");
        //mount_point::ptr mp2;
        if (!fp)
        {
            LOG_ERROR("open %s failed.\n", start_path.c_str());
            return false;
        }
        if (fscanf(fp, "%llu", &offset) != 1)
        {
            LOG_ERROR("read offset failed.\n");
            fclose(fp);
            return false;
        }
        fclose(fp);
        fp = NULL;
        partition_start_offset = (offset << 9);
        /*get the mounted_path*/
        return true;
    }

    bool partitionA::auto_setting_partition_offset_by_fdisk()
    {
        string cmdstring = "fdisk -l " + metadata.mpath_partition_parent + " | grep /dev | sed -e \'/Disk/ d\'";
        LOG_TRACE("cmdstring = %s", cmdstring.c_str());
        string result = system_tools::execute_command(cmdstring.c_str());
        LOG_TRACE("result = %s", result.c_str());
        std::istringstream iss(result);
        std::vector<std::string> results((std::istream_iterator<std::string>(iss)),std::istream_iterator<std::string>());
        for (auto & r : results)
        {
            LOG_TRACE("r = %s", r.c_str());
        }
        partition_start_offset = stoi(results[1])<<9;
        return true;
    }

    bool partitionA::auto_setting_partition_offset()
    {
        if (sysfs_path.empty())
        {
            return auto_setting_partition_offset_by_fdisk();
        }
        else
        {
            return auto_setting_partition_offset_by_sys_path();
        }
    }

    bool partitionA::parsing_partition_header()
    {
        FUNC_TRACER;
        bool result = false;
        /*first get the rw from it parents*/
        if (parent_disk == NULL)
        {
            LOG_TRACE("parent_disk == NULL")
            return false;
        }
        universal_disk_rw::ptr rw = parent_disk->get_rw();
        if (!rw)
        {
            LOG_TRACE("rw == NULL")
            return false;
        }
        std::shared_ptr<unsigned char> buf(new unsigned char[(GPT_PART_SIZE_SECTORS + 1) * SECTOR_SIZE]);
        unsigned char * data_buf = buf.get();
        if (!data_buf)
        {
            LOG_ERROR("data_buf create error\r\n");
        }
        uint32_t number_of_sectors_read = 0;
        PLEGACY_MBR part_table;
        if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf, number_of_sectors_read)) {
            //return 0;
            part_table = (PLEGACY_MBR)data_buf;
            if (part_table->PartitionRecord[0].PartitionType == 0xEE)
            {
                PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf[SECTOR_SIZE];
                PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf[SECTOR_SIZE * 2];
                for (int i = 0; i < gpt_hdr->NumberOfPartitionEntries; i++)
                {
                    uint64_t partition_offset = gpt_part[i].StartingLBA * rw->sector_size();
                    if (partition_offset == partition_start_offset)
                    {
                        GUID * partition_type = (GUID *)gpt_part[i].PartitionTypeGuid;
                        if (!strcasecmp(partition_type->to_string().c_str(), "c12a7328-f81f-11d2-ba4b-00a0c93ec93b"))
                        {
                            BootIndicator = true;
                        }
                        return true;
                    }
                }
            }
            else
            {
                for (int index = 0; index < 4; index++)
                {
                    uint64_t start = ((uint64_t)part_table->PartitionRecord[index].StartingLBA) * rw->sector_size();
                    if (start == partition_start_offset)
                    {
                        if (part_table->PartitionRecord[index].BootIndicator == 0x80)
                        {
                            BootIndicator = true;
                        }
                        return true;
                    }
                    if (part_table->PartitionRecord[index].PartitionType == PARTITION_XINT13_EXTENDED ||
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
                                        uint64_t partition_offset = (uint64_t)pMBR->PartitionRecord[i].StartingLBA * rw->sector_size() + start;
                                        if (partition_offset == partition_start_offset)
                                        {
                                            if (pMBR->PartitionRecord[i].BootIndicator == 0x80)
                                            {
                                                BootIndicator = true;
                                            }
                                            return true;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }      
        return true;
        /*if the bootable is 0x80-> is system == true, else is false*/
    }
    int partitionA::parsing_partition(disk_baseA::ptr disk_base)
    {
        set_disk_base(disk_base);
        if (!auto_setting_partition_offset())
        {
            goto error;
        }
        mp = disk_baseA::get_mount_point(ab_path);
        /*mp2 = get_mount_point();
        if (mp2 != NULL)
        {
            LOG_TRACE("mp2->get_mounted_on() = %s", mp2->get_mounted_on().c_str());
            LOG_TRACE("mp2->get_device() = %s", mp2->get_device().c_str());
            LOG_TRACE("mp2->get_fstype() = %s", mp2->get_fstype().c_str());
        }*/
        if (mp != NULL)
        {
            filesystem_type = mp->get_fstype();
            if (filesystem_type == "btrfs")
            {
                mp = NULL;
                mps = disk_baseA::get_mount_points(ab_path);
            }
        }

        return 0;
    error:
        return -1;
    }
    int partitionA::parsing_partition_lvm(disk_baseA::ptr disk_base)
    {
        FUNC_TRACER;
        char buf[COMMON_BUFFER_SIZE];
        int fd0;
        set_disk_base(disk_base);
        std::string lvname_path = sysfs_path + "/dm/name";
        FILE * fp = fopen(lvname_path.c_str(), "r");
        std::string uuid_path = sysfs_path + "/dm/uuid";
        //mount_point::ptr mp2;
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
        /*get the id*/
        fp = fopen(uuid_path.c_str(), "r");
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
        uuid = string(buf + 4);

        /*get the mounted_path*/
        mp = disk_baseA::get_mount_point(ab_path);
        if (mp != NULL)
        {
            filesystem_type = mp->get_fstype();
            if (filesystem_type == "btrfs")
            {
                mp = NULL;
                mps = disk_baseA::get_mount_points(ab_path);
            }
        }
        /*mp2 = get_mount_point();
        if (mp2 != NULL)
        {
            LOG_TRACE("mp2->get_mounted_on() = %s", mp2->get_mounted_on().c_str());
            LOG_TRACE("mp2->get_device() = %s", mp2->get_device().c_str());
            LOG_TRACE("mp2->get_fstype() = %s", mp2->get_fstype().c_str());
        }*/

        return 0;
    error:
        if (fp) fclose(fp);
        return -1;
    }

    bool partitionA::compare(partitionA::ptr ptrA, partitionA::ptr ptrB)
    {
        if (ptrA->major != ptrB->major ||
            ptrA->minor != ptrB->minor ||
            ptrA->block_type != ptrB->block_type ||
            ptrA->blocks != ptrB->blocks ||
            ptrA->device_filename != ptrB->device_filename ||
            ptrA->ab_path != ptrB->ab_path ||
            ptrA->sysfs_path != ptrB->sysfs_path ||
            ptrA->filesystem_type != ptrB->filesystem_type ||
            !ptrA->mp && ptrB->mp ||
            ptrA->mp && !ptrB->mp ||
            (ptrA->mp && ptrB->mp && ptrA->mp->get_mounted_on() != ptrB->mp->get_mounted_on())||
            ptrA->lvname != ptrB->lvname ||
            ptrA->uuid != ptrB->uuid)
            return false;
        for (auto & lvmA : ptrA->lvms)
        {
            bool lvm_same = false;
            for (auto & lvmB : ptrB->lvms)
            {
                if (partitionA::compare(lvmA, lvmB))
                {
                    lvm_same = true;
                    break;
                }
            }
            if (!lvm_same)
                return false;
        }
        return true;
    }

    void partitionA::clear()
    {
        cleared = true;
        for (auto & l : lvms)
        {
            if(!l->cleared)
                l->clear();
        }
        lvms.clear();
        for (auto & s : slaves)
        {
            if(!s->cleared)
                s->clear();
        }
        slaves.clear();      
        linux_storage::disk::ptr dsk = boost::static_pointer_cast<linux_storage::disk>(parent_disk);
        if(!dsk->cleared)
            dsk->clear();
        dsk = NULL;
    }


    /*mount_point::ptr partitionA::get_mount_point()
    {
        char dev[COMMON_BUFFER_SIZE], mount[COMMON_BUFFER_SIZE], fsb[COMMON_BUFFER_SIZE];
        FILE * fp = fopen(PROC_MOUNTSTATS, "r");
        mount_point::ptr out;
        bool b_found = false;
        std::vector<std::string> mounted_device_names;
        if(!lvname.empty())
            mounted_device_names.push_back(lvname);
        mounted_device_names.push_back(device_filename);
        mounted_device_names.push_back(ab_path);
        if (!fp)
        {
            LOG_ERROR("open %s failed.\n", PROC_MOUNTSTATS);
            goto error;
        }
        //2. parsing the file to find the device name
        while (fscanf(fp, "device %s mounted on %s with fstype %s\n", dev, mount, fsb) == 3)
        {
            for (auto & mounted_device_name : mounted_device_names)
            {
                if (std::string(dev).find(mounted_device_name) != string::npos)
                {
                    out = mount_point::ptr(new mount_point());
                    out->set_device(std::string(dev));
                    out->set_mounted_on(std::string(mount));
                    out->set_fstype(std::string(fsb));
                    b_found = true;
                    break;
                }
            }
            if (b_found)
                break;
        }
        fclose(fp);
    error:
        return out;
    }*/
    mount_point::vtr disk_baseA::get_mount_points(string ab_path)
    {
        FUNC_TRACER;
        char dev[COMMON_BUFFER_SIZE], mount[COMMON_BUFFER_SIZE], fsb[COMMON_BUFFER_SIZE];
        FILE * fp = fopen(PROC_MOUNTSTATS, "r");
        mount_point::vtr out;
        mount_point::ptr mp;
        bool b_found = false;
        int fr = ab_path.rfind("/");
        /*std::vector<std::string> mounted_device_names;
        if (!lvname.empty())
        mounted_device_names.push_back(lvname);
        mounted_device_names.push_back(device_filename);
        mounted_device_names.push_back(ab_path);*/
        if (!fp)
        {
            LOG_ERROR("open %s failed.\n", PROC_MOUNTSTATS);
            goto error;
        }
        //2. parsing the file to find the device name
        while (fscanf(fp, "device %s mounted on %s with fstype %s\n", dev, mount, fsb) == 3)
        {
            char link_buf[256];
            char * real_path;
            if (realpath(dev, link_buf))
                real_path = link_buf;
            else
                real_path = dev;
            if (string(real_path) == ab_path)
            {
                mp = mount_point::ptr(new mount_point());
                mp->set_device(std::string(real_path));
                mp->set_mounted_on(std::string(mount));
                mp->set_fstype(std::string(fsb));
                out.push_back(mp);
            }
        }
        fclose(fp);
        for (auto & mp : out)
        {
            if (fr != string::npos && mp)
            {
                string device_filename = ab_path.substr(fr + 1);
                boost::filesystem::path DBU(DISK_BY_UUID);
                char link[256];
                std::map<string, string> uuids;
                for (boost::filesystem::directory_iterator it = boost::filesystem::directory_iterator(DBU);
                    it != boost::filesystem::directory_iterator(); ++it)
                {
                    memset(link, 0, sizeof(link));
                    string sling;
                    int rd = readlink(it->path().string().c_str(), link, sizeof(link));
                    if (rd > 0)
                    {
                        string slink = string(link);
                        string spath = it->path().string();
                        if (slink.empty())
                            continue;
                        string device_file_name = slink.substr(slink.rfind("/") + 1);
                        if (device_file_name.empty())
                            continue;
                        if (spath.empty())
                            continue;
                        string uuid = spath.substr(spath.rfind("/") + 1);
                        if (uuid.empty())
                            continue;
                        uuids[device_file_name] = uuid;
                    }
                }
                if (uuids.count(device_filename))
                {
                    mp->set_system_uuid(uuids[device_filename]);
                }
            }
        }
    error:
        return out;
    }
    mount_point::ptr disk_baseA::get_mount_point(string ab_path)
    {
        FUNC_TRACER;
        char dev[COMMON_BUFFER_SIZE], mount[COMMON_BUFFER_SIZE], fsb[COMMON_BUFFER_SIZE];
        FILE * fp = fopen(PROC_MOUNTSTATS, "r");
        mount_point::ptr out = NULL;
        bool b_found = false;
        int fr = ab_path.rfind("/");
        /*std::vector<std::string> mounted_device_names;
        if (!lvname.empty())
            mounted_device_names.push_back(lvname);
        mounted_device_names.push_back(device_filename);
        mounted_device_names.push_back(ab_path);*/
        if (!fp)
        {
            LOG_ERROR("open %s failed.\n", PROC_MOUNTSTATS);
            goto error;
        }
        //2. parsing the file to find the device name
        while (fscanf(fp, "device %s mounted on %s with fstype %s\n", dev, mount, fsb) == 3)
        {
            char link_buf[256]; 
            char * real_path;
            if (realpath(dev, link_buf))
                real_path = link_buf;
            else
                real_path = dev;
            if (string(real_path) == ab_path)
            {
                out = mount_point::ptr(new mount_point());
                out->set_device(std::string(real_path));
                out->set_mounted_on(std::string(mount));
                out->set_fstype(std::string(fsb));
                b_found = true;
                break;
            }

            if (b_found)
                break;
        }
        fclose(fp);
        if (fr != string::npos && out)
        {
            string device_filename = ab_path.substr(fr+1);
            boost::filesystem::path DBU(DISK_BY_UUID);
            char link[256];
            std::map<string, string> uuids;
            for (boost::filesystem::directory_iterator it = boost::filesystem::directory_iterator(DBU);
                it != boost::filesystem::directory_iterator(); ++it)
            {
                memset(link, 0, sizeof(link));
                string sling;
                int rd = readlink(it->path().string().c_str(), link, sizeof(link));
                if (rd > 0)
                {
                    string slink = string(link);
                    string spath = it->path().string();
                    if (slink.empty())
                        continue;
                    string device_file_name = slink.substr(slink.rfind("/") + 1);
                    if (device_file_name.empty())
                        continue;
                    if (spath.empty())
                        continue;
                    string uuid = spath.substr(spath.rfind("/") + 1);
                    if (uuid.empty())
                        continue;
                    uuids[device_file_name] = uuid;
                }
            }
            if (uuids.count(device_filename))
            {
                out->set_system_uuid(uuids[device_filename]);
            }
        }
    error:
        return out;
    }

    int disk::insert_lvm_to_disk(partitionA::ptr partition, disk::vtr disks)
    {
        std::string slaves_folder_path = partition->sysfs_path + "/slaves";
        boost::filesystem::path pslave = boost::filesystem::path(slaves_folder_path.c_str()); //slaves should be here

        for (auto it : boost::filesystem::directory_iterator(pslave))
        {
            boost::filesystem::path px = it.path();
            for (auto d : disks)
            {
                //LOG_TRACE("&d = %x, sizeof(d) = %u", &d, sizeof(d));
                if (d->partitions.size() > 0)
                {
                    for (auto p : d->partitions)
                    {
                        if (p->device_filename == px.filename())
                        {
                            partition->slaves.push_back(p);
                            if (partition->parent_disk == NULL)
                            {
                                partition->parent_disk = boost::static_pointer_cast<disk_baseA>(d);

                            }
                        }
                    }
                }
            }
        }
    }

    bool disk::compare(disk::ptr diskA, disk::ptr diskB)
    {
        if (!(disk_universal_identify(diskA->string_uri) == disk_universal_identify(diskB->string_uri)))
            return false;
        for (auto & ptrA : diskA->partitions)
        {
            bool partition_same = false;
            for (auto & ptrB : diskB->partitions)
            {
                if (partitionA::compare(ptrA,ptrB))
                {
                    partition_same = true;
                    break;
                }
            }
            if (!partition_same)
                return false;
        }
        return true;
    }

    disk::vtr disk::get_disk_info()
    {
        disk::vtr output;
        char line[COMMON_BUFFER_SIZE], buf[COMMON_BUFFER_SIZE];
        int fd0, rd;
        boost::filesystem::path DBU(DISK_BY_UUID);
        fstream fs;
        char link[256];
        struct stat st;
        std::map<string, string> uuids;
        std::map<string, std::vector<string>> mpath_mapping;
        boost::filesystem::path p(PROC_PARTITIONS);
        disk_baseA::vtr normal_disks, lvms, parsing_result;
        std::map<string, string> system_folder_map;
        std::vector<string> remove_device_name;
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
                disk_base->device_filename.find("ram") != string::npos ||
                disk_base->device_filename.find("datto") != string::npos ||
                disk_base->device_filename.find("loop") != string::npos)
                continue;
            disk_base->ab_path = "/dev/" + disk_base->device_filename;
            sprintf(buf, "%d:%d", disk_base->major, disk_base->minor);
            disk_base->sysfs_path = "/sys/dev/block/" + string(buf); //add sysfs_path
            system_folder_map[disk_base->device_filename] = disk_base->sysfs_path;
            mpath_mapping[disk_base->device_filename].push_back(disk_base->device_filename);
            parsing_result.push_back(disk_base);
        }

        //first check the mpath disk
        for (auto & db : parsing_result)
        {
            int fd1 = open(db->sysfs_path.c_str(), O_RDONLY);
            if (!fd1)
            {
                LOG_ERROR("open %s error\r\n", db->sysfs_path.c_str());
                goto error;
            }
            if (!fstatat(fd1, "dm", &st, 0))
            {
                string inputStr;
                string uuid = string(db->sysfs_path + "/dm/uuid");
                fstream fs_uuid(uuid.c_str(), ios::in);
                if (!fs_uuid)
                {
                    LOG_ERROR("open %s error\r\n", uuid.c_str());
                    close(fd1);
                    continue;
                }
                getline(fs_uuid, inputStr);
                if (inputStr.empty())
                {
                    LOG_ERROR("read uuid error\r\n");
                    fs_uuid.close();
                    close(fd1);
                    goto error;
                }
                //check multi path
                if (inputStr.find("mpath") != std::string::npos)
                {
                    //partition
                    int part_local = inputStr.find("part");
                    if (part_local == std::string::npos)
                    {
                        boost::filesystem::path slaves = boost::filesystem::path(db->sysfs_path) / "slaves";
                        bool not_change = true;
                        mpath_mapping[db->device_filename].clear(); // if this is a mpath device, should remove the original mapping value;
                        for (boost::filesystem::directory_iterator it = boost::filesystem::directory_iterator(slaves);
                            it != boost::filesystem::directory_iterator(); ++it)
                        {
                            if (not_change)
                            {
                                system_folder_map[db->device_filename] = system_folder_map[it->path().filename().string()];
                                db->sysfs_path = system_folder_map[db->device_filename];
                                not_change = false;
                            }
                            remove_device_name.push_back(it->path().filename().string());
                            mpath_mapping[db->device_filename].push_back(it->path().filename().string());
                        }
                    }
                }
            }
        }

        for (auto & str : remove_device_name)
        {
            for (disk_baseA::vtr::iterator idb = parsing_result.begin(); idb != parsing_result.end();)
            {
                if ((*idb)->device_filename == str)
                {
                    idb = parsing_result.erase(idb);
                    break;
                }
                else
                    ++idb;           
            }
        }
        remove_device_name.clear();

        //first check the mpath partition
        for (auto & db : parsing_result)
        {
            int fd1 = open(db->sysfs_path.c_str(), O_RDONLY);
            if (!fd1)
            {
                LOG_ERROR("open %s error\r\n", db->sysfs_path.c_str());
                goto error;
            }
            if (!fstatat(fd1, "dm", &st, 0))
            {
                string inputStr;
                string uuid = string(db->sysfs_path + "/dm/uuid");
                fstream fs_uuid(uuid.c_str(), ios::in);
                if (!fs_uuid)
                {
                    LOG_ERROR("open %s error\r\n", uuid.c_str());
                    close(fd1);
                    continue;
                }
                getline(fs_uuid, inputStr);
                if (inputStr.empty())
                {
                    LOG_ERROR("read uuid error\r\n");
                    fs_uuid.close();
                    close(fd1);
                    goto error;
                }
                //check multi path
                if (inputStr.find("mpath")!=std::string::npos)
                {
                    //partition
                    int part_local = inputStr.find("part");
                    if (part_local != std::string::npos)
                    {
                        //int partition_number;
                        if (sscanf(inputStr.c_str(), "part%d-%s", &db->metadata.mpath_partition_number, line) != 2)
                        {
                            LOG_ERROR("read mpath partition error\r\n");
                            fs_uuid.close();
                            close(fd1);
                            goto error;
                        }
                        db->metadata.is_mpath_partition = true;
                        mpath_mapping[db->device_filename].clear(); // if this is a mpath partition device, should remove the original mapping value;
                        boost::filesystem::path slaves = boost::filesystem::path(db->sysfs_path) / "slaves";
                        for (boost::filesystem::directory_iterator it = boost::filesystem::directory_iterator(slaves);
                            it != boost::filesystem::directory_iterator(); ++it)
                        {
                            if (mpath_mapping.count(it->path().filename().string()))
                            {
                                for (auto & mapping : mpath_mapping[it->path().filename().string()])
                                    mpath_mapping[db->device_filename].push_back(mapping + to_string(db->metadata.mpath_partition_number));
                                db->metadata.mpath_partition_parent = "/dev/" + mpath_mapping[it->path().filename().string()][0];
                            }
                            system_folder_map[db->device_filename] = system_folder_map[mpath_mapping[db->device_filename][0]] ;
                            db->sysfs_path = system_folder_map[db->device_filename];
                            remove_device_name.insert(remove_device_name.end(),mpath_mapping[db->device_filename].begin(), mpath_mapping[db->device_filename].end());
                        }
                    }
                }
            }
        }

        for (auto & str : remove_device_name)
        {
            for (disk_baseA::vtr::iterator idb = parsing_result.begin(); idb != parsing_result.end();)
            {
                if ((*idb)->device_filename == str)
                {
                    idb = parsing_result.erase(idb);
                    break;
                }
                else
                    ++idb;
            }
        }
        remove_device_name.clear();

        for (auto & disk_base : parsing_result)
        {
            /*go to system path to check the block type*/
            
            if (!disk_base->auto_setting_block_device_type())
            {
                goto error;
            }

            if (disk_base->block_type != disk_baseA::lvm)
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
            if (disk_base->block_type == disk_baseA::disk) //partition is not exist --> disk
            {
                disk::ptr dc = disk::ptr(new disk());
                if (dc->parsing_disk(disk_base))
                {
                    LOG_ERROR("parsing_disk error\r\n");
                    goto error;
                }
                output.push_back(dc);
            }
            else if (disk_base->block_type == disk_baseA::partition)//partition exist --> partition
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
                    if (mpath_mapping[pc->device_filename][0].find(mpath_mapping[d->device_filename][0], 0) != string::npos)
                    {
                        d->partitions.push_back(pc);
                        pc->parent_disk = boost::static_pointer_cast<disk_baseA>(d);
                        break;
                    }
                }
                LOG_TRACE("pc->ab_path = %s", pc->ab_path.c_str());
                pc->parsing_partition_header();
            }
            else if (disk_base->block_type == disk_baseA::lvm)//lvm
            {
                partitionA::ptr pc = partitionA::ptr(new partitionA());
                if (pc->parsing_partition_lvm(disk_base))
                {
                    LOG_ERROR("parsing_partition_lvm error\r\n");
                    /*not go to error*/
                }
                else
                {
                    disk::insert_lvm_to_disk(pc, output);
                    for (auto &s : pc->slaves)
                        s->lvms.push_back(pc);
                }
            }
        }
        fs.close();
        /*get the uuid mapping*/
        for (boost::filesystem::directory_iterator it = boost::filesystem::directory_iterator(DBU);
            it != boost::filesystem::directory_iterator(); ++it)
        {
            memset(link, 0, sizeof(link));
            string sling;
            int rd = readlink(it->path().string().c_str(), link, sizeof(link));
            if (rd > 0)
            {
                string slink = string(link);
                string spath = it->path().string();
                if (slink.empty())
                    continue;
                string device_file_name = slink.substr(slink.rfind("/") + 1);
                if (device_file_name.empty())
                    continue;
                if (spath.empty())
                    continue;
                string uuid = spath.substr(spath.rfind("/") + 1);
                if (uuid.empty())
                    continue;
                uuids[device_file_name] = uuid;
            }
        }
        
        for (auto &dc : output)
        {
            disk_universal_identify disk_uri(*dc);
            /*dc->uri.put("serial_number", dc->serial_number);
            dc->uri.put("path", dc->ab_path);
            dc->uri.put("friendly_name", dc->device_filename);
            stringstream s;
            boost::property_tree::json_parser::write_json(s, dc->uri, false);*/
            dc->string_uri = (std::string)disk_uri;
            if (dc->mp)
                dc->set_system_uuid(dc->mp->get_system_uuid());
            else
                if (uuids.count(dc->device_filename))
                    dc->set_system_uuid(uuids[dc->device_filename]);

            for (auto & pt : dc->partitions)
            {
                if (pt->mp)
                    pt->set_system_uuid(pt->mp->get_system_uuid());
                else
                    if (uuids.count(pt->device_filename))
                        pt->set_system_uuid(uuids[pt->device_filename]);
                for (auto & lvm : pt->lvms)
                {
                    if (lvm->mp)
                        lvm->set_system_uuid(lvm->mp->get_system_uuid());
                    else
                        if (uuids.count(lvm->device_filename))
                            lvm->set_system_uuid(uuids[lvm->device_filename]);
                }
            }
        }
        if (output.size() == 0)
            goto error;
        /*write json*/
        

        return output;
    error:
        if (fs.is_open())
            fs.close();
        THROW_EXCEPTION("get_disk_info error");
        return output;
    }

    int disk::parsing_disk(disk_baseA::ptr disk_base)
    {
        set_disk_base(disk_base);
        char bus_link[COMMON_BUFFER_SIZE], scsi_link[COMMON_BUFFER_SIZE],temp_buf[COMMON_BUFFER_SIZE];
        char * tempchr;
        std::vector<string> strVec;
        std::string s2;

        memset(bus_link, 0, sizeof(bus_link));
        std::string bus_path;
        std::string scsi_path = sysfs_path + "/device";
        std::vector<string>::iterator it;
        bool have_scsi_id = true;
        int rd = readlink(scsi_path.c_str(), scsi_link, sizeof(scsi_link));
        if (rd <= 0) //error handle, readlink failed.
        {
            LOG_ERROR("link %s status : %d\r\n", scsi_path.c_str(), rd);
            goto error;
        }
        s2 = std::string(scsi_link);
        boost::split(strVec, s2, boost::is_any_of("/"));
        it = strVec.begin();
        scsi_address = *it;
        while (4 != sscanf(scsi_address.c_str(), "%d:%d:%d:%d", &scsi_port, &scsi_bus, &scsi_target_id, &scsi_logical_unit))
        {
            it++;
            if (it == strVec.end())
            {
                LOG_WARNING("there are no scsi_address, fill it with -1");
                scsi_port = scsi_bus = scsi_target_id = scsi_logical_unit = -1;
                break;
            }
            scsi_address = *it;
            
        }
        /*if(it == strVec.end())
        {
            LOG_ERROR("scsi_address %s error\r\n", scsi_address.c_str());
            goto error;
        }*/
        sprintf(temp_buf, "%d:%d:%d:%d", scsi_port, scsi_bus, scsi_target_id, scsi_logical_unit);
        scsi_address = string(temp_buf);
        bus_path = sysfs_path + "/device/driver";
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
        get_sd_ioctl_info();
        if (serial_number.empty() || serial_number.size()!=GUID_size_with_dash)
        {
            get_hd_ioctl_info();
            if (serial_number.empty() || serial_number.size() != GUID_size_with_dash)
            {
                //serial_number = GUID().to_string();
                LOG_TRACE("serial_number.size() = %d\r\n", serial_number.size());
            }
        }
        mp = disk_baseA::get_mount_point(ab_path);
        if (mp != NULL)
        {
            filesystem_type = mp->get_fstype();
            if (filesystem_type == "btrfs")
            {
                mp = NULL;
                mps = disk_baseA::get_mount_points(ab_path);
            }

        }

        parsing_disk_header();
        return 0;
    error:
        return -1;
    }

    int disk::get_hd_ioctl_info()
    {
        int fd = open(ab_path.c_str(), O_RDONLY);
        int ret = -1;
        if (!fd)
        {
            LOG_ERROR("open %s failed\r\n", ab_path.c_str());
            goto error;
        }
        /*if the file name is "hdx"*/

        struct hd_driveid hd;
        if (!ioctl(fd, HDIO_GET_IDENTITY, &hd))
        {
            LOG_ERROR("HDIO_GET_IDENTITY %s\r\n", hd.serial_no);
            goto error;
        }
        else if (errno == -ENOMSG) {
            LOG_ERROR("No serial number available\n");
            goto error;
        }
        else {
            LOG_ERROR("ERROR: HDIO_GET_IDENTITY");
            goto error;
        }
        serial_number = string((char*)hd.serial_no);
        sector_size = 512;//hd.sector_bytes;
        ret = 0;
    error:
        if(fd)
            close(fd);
        return ret;
    }
    int disk::get_sd_ioctl_info()
    {
        int fd = open(ab_path.c_str(), O_RDONLY);
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
            if (io_hdr.masked_status || io_hdr.host_status || io_hdr.driver_status) 
            {
                LOG_ERROR("status=0x%x\n", io_hdr.status);
                LOG_ERROR("masked_status=0x%x\n", io_hdr.masked_status);
                LOG_ERROR("host_status=0x%x\n", io_hdr.host_status);
                LOG_ERROR("driver_status=0x%x\n", io_hdr.driver_status);
            }
            else
            {
                serial_number = string(&scsi_data[4]);
                LOG_TRACE("%s\r\n", scsi_data);
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

    int disk::parsing_disk_header()
    {
        boost::uuids::string_generator gen;
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
        std::shared_ptr<unsigned char> buf(new unsigned char[(GPT_PART_SIZE_SECTORS+1) * SECTOR_SIZE]);
        unsigned char * data_buf = buf.get();
        if (!data_buf)
        {
            LOG_ERROR("data_buf create error\r\n");
        }
        uint32_t number_of_sectors_read = 0;
        PLEGACY_MBR part_table;
        if (result = rw->sector_read(0, GPT_PART_SIZE_SECTORS, data_buf, number_of_sectors_read)) {
            //return 0;
            part_table = (PLEGACY_MBR)data_buf;
            MBRSignature = part_table->UniqueMBRSignature;

            int cnt = 0;
            boost::uuids::uuid u;
            if (part_table->PartitionRecord[0].PartitionType == 0xEE) //GPT
            {
                partition_style = partition_style::PARTITION_GPT;
                PGPT_PARTITIONTABLE_HEADER gpt_hdr = (PGPT_PARTITIONTABLE_HEADER)&data_buf[SECTOR_SIZE];
                PGPT_PARTITION_ENTRY gpt_part = (PGPT_PARTITION_ENTRY)&data_buf[SECTOR_SIZE * 2];
                LOG_TRACE("gpt_hdr->NumberOfPartitionEntries = %d\r\n", gpt_hdr->NumberOfPartitionEntries);
                memcpy(&u, gpt_hdr->DiskGUID, 16);
                str_guid = boost::uuids::to_string(u);
                /*parsing something*/

            }
            else
            {
                partition_style = partition_style::PARTITION_MBR;
                uint8_t NULL_GUID[16] = { 0 };
                memset(NULL_GUID, 0, 16);
                memcpy(&u, NULL_GUID, 16);
                str_guid = boost::uuids::to_string(u);
            }
            LOG_TRACE("GUID = %s\r\n", str_guid.c_str());
        }
        return 0;
    }
    int storage::scan_disk()
    {
        try {
            if (_disks.size())
                _disks.clear();
            _disks = disk::get_disk_info();
        }
        catch (boost::exception &e) {
            LOG_ERROR("scan_disk exception : %s\r\n", boost::diagnostic_information(e).c_str());
        }
    }

    storage::ptr storage::get_storage()
    {
        storage::ptr out = storage::ptr(new storage());
        out->scan_disk();
        return out;
    }

    mount_info::vtr storage::get_mount_info(mount_info::filter::ptr flt)
    {
        mount_info::vtr out;
        fstream fs;
        fs.open(PROC_MOUNTS, fstream::in);
        string mount_info_;
        while (getline(fs, mount_info_))
        {
            vector<string> strVec;
            boost::split(strVec, mount_info_, boost::is_any_of(" "));
            //add / to the last of string
            if (strVec[1][strVec[1].size() - 1] != '/')
                strVec[1] += '/';

            bool pass = true;
            if (flt)
            {
                if ((!flt->block_device.empty() && strVec[0] != flt->block_device) || 
                    (!flt->mount_path.empty() && strVec[1] != flt->mount_path) || 
                    (!flt->fs_type.empty() && strVec[2] != flt->fs_type))
                    pass = false;
            }
            if (pass)
            {
                mount_info::ptr mi = mount_info::ptr(new mount_info(strVec[0], strVec[1], strVec[2], strVec[3]));
                out.push_back(mi);
            }
        }
        return out;
    }

    disk::vtr storage::get_all_disk()
    {
        return _disks;
    }

    std::string storage::get_disk_ab_path_from_uri(std::string uri)
    {
        std::set<std::string> out;
        for (auto & d : _disks)
        {
            LOG_TRACE("d->ab_path = %s\r\n", d->ab_path.c_str());
            LOG_TRACE("uri = %s\r\n", uri.c_str());
            if (disk_universal_identify(*d) == disk_universal_identify(uri))
            {
                LOG_TRACE("OK d->ab_path = %s\r\n", d->ab_path.c_str());
                return d->ab_path;
            }
        }
        return std::string();
    }

    std::set<std::string> storage::enum_disks_ab_path_from_uri(std::set<std::string> & uris)
    {
        std::set<std::string> out;       
        for (auto u : uris)
        {
            std::string ad_path = get_disk_ab_path_from_uri(u);
            if (ad_path.size() > 0)
                out.insert(ad_path);
        }
        return out;
    }


    disk_baseA::vtr storage::get_instances_from_ab_paths(std::set<std::string> ab_paths)
    {
        disk_baseA::vtr out;
        for (auto & ab_path : ab_paths)
        {
            out.push_back(get_instance_from_ab_path(ab_path));
        }
        return out;
    }

    disk_baseA::ptr storage::get_instance_from_ab_path(std::string ab_path)
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
                    }
                    else
                    {
                        for (auto & l : p->lvms)
                        {
                            if (l->ab_path == ab_path)
                            {
                                found = true;
                                out = boost::static_pointer_cast<disk_baseA>(l);
                                break;
                            }
                        }
                    }
                    if (found)
                        break;
                }
            }
            if (found)
            {
                break;
            }
        }
        return out;
    }

    disk_baseA::vtr storage::enum_multi_instances_from_multi_ab_paths(std::set<std::string> ab_paths)
    {
        disk_baseA::vtr out;
        for (auto & a : ab_paths)
        {
            disk_baseA::ptr d = get_instance_from_ab_path(a);
            out.push_back(d);
        }
        return out;
    }

    std::set<std::string> storage::enum_partition_ab_path_from_uri(std::set<std::string> uris)
    {
        std::set<std::string> disks = enum_disks_ab_path_from_uri(uris);
        return enum_partition_ab_path_from_disk_ab_path(disks);
    }

    //disk_baseA::ptr gets

    partitionA::vtr storage::get_all_partition()
    {
        partitionA::vtr out;
        for (auto & d : _disks)
        {
            if (d->partitions.size())
                out.insert(out.end(), d->partitions.begin(), d->partitions.end());
        }
        return out;
    }
    mount_point::vtr storage::get_all_mount_point()
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

    partitionA::ptr storage::get_lvm_instance_from_volname(std::string volname)
    {
        partitionA::vtr all_parts = get_all_partition();
        for (auto & p : all_parts)
        {
            if (p->lvms.size())
            {
                for (auto & l : p->lvms)
                {
                    if (l->lvname == volname)
                    {
                        return l;
                    }
                }
            }
        }
        return NULL;
    }

    std::set<std::string> storage::enum_partition_ab_path_from_disk_ab_path(std::set<std::string> & disk_names)
    {
        std::set<std::string> out;
        for (auto & d : _disks)
        {
            for (auto & dn : disk_names)
            {
                LOG_TRACE("d->ab_path = %s, dn = %s\r\n", d->ab_path.c_str(), dn.c_str());
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

    disk_universal_identify::disk_universal_identify(std::string uri) //i think use ptree to parsing this....  is oK? but json is the same i think use boost and other api is the same , so ~
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
    disk_universal_identify::disk_universal_identify(disk & d)
    {
        _serial_number = d.serial_number;
        _partition_style = d.partition_style; //need
        _mbr_signature = d.MBRSignature;//need
        _gpt_guid = d.str_guid;// d.Disk_UUID;//need    //TETSETSETS
        _cluster_id = "";//need
        _friendly_name = d.ab_path;//need
        _address = d.scsi_address;
        _unique_id = "";// find_value_string(value.get_obj(), "unique_id");
        _unique_id_format = 0;// (unique_id_format_type)find_value_int32(value.get_obj(), "unique_id_format");
    }
    const disk_universal_identify& disk_universal_identify::operator =(const disk_universal_identify& disk_uri)
    {
        if (this != &disk_uri)
            copy(disk_uri);
        return *this;
    }
    bool disk_universal_identify::operator ==(const disk_universal_identify& disk_uri)
    {
        if (_cluster_id.length() && disk_uri._cluster_id.length())
            return  (_cluster_id == disk_uri._cluster_id);
        if (_unique_id_format == unique_id_format_type::FCPH_Name && _unique_id_format == disk_uri._unique_id_format && !_unique_id.empty() && !disk_uri._unique_id.empty())
            return _unique_id == disk_uri._unique_id;
        else if (_serial_number.length() > 4 &&
            disk_uri._serial_number.length() > 4 &&
            (_serial_number != "00000000-0000-0000-0000-000000000000" && _serial_number != "2020202020202020202020202020202020202020") &&
            (disk_uri._serial_number != "00000000-0000-0000-0000-000000000000" && disk_uri._serial_number != "2020202020202020202020202020202020202020"))
            return (_serial_number == disk_uri._serial_number);
        else if (!_friendly_name.empty() && !disk_uri._friendly_name.empty())
            return(_friendly_name == disk_uri._friendly_name);
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
    bool disk_universal_identify::operator !=(const disk_universal_identify& disk_uri)
    {
        return !(operator ==(disk_uri));
    }
    disk_universal_identify::operator std::string()
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

    void disk_universal_identify::copy(const disk_universal_identify& disk_uri) {
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

}
#include "snapshot.h"
#include "data_type.h"
#include <boost/pointer_cast.hpp>
#include <unistd.h>

using namespace linux_storage;
int dattobd::dattobd_setup_snapshot(unsigned int minor, const char *bdev, const char *cow, unsigned long fallocated_space, unsigned long cache_size)
{
    FUNC_TRACER;
    LOG_TRACE("minor = %d\r\n", minor);
    int fd, ret;
    setup_params sp;
    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }

    sp.minor = minor;
    sp.bdev = strdup(bdev);
    sp.cow = strdup(cow);
    sp.fallocated_space = fallocated_space;
    sp.cache_size = cache_size;
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_SETUP_SNAP, &sp);
        --count;
    } while (ret && count >0);
    if (ret)
        LOG_ERROR("datto%d(%s : fallocated_space:%llu: cach_size:%llu) with cow %s setup snapshot error %d", minor, bdev, fallocated_space, cache_size, cow, ret);

    close(fd);
    return ret;
}
int dattobd::dattobd_reload_snapshot(unsigned int minor, const char *bdev, const char *cow, unsigned long cache_size)
{
    FUNC_TRACER;
    int fd, ret;
    reload_params rp;

    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }
    LOG_TRACE("minor = %u, bdev=%s, cow = %s", minor, bdev, cow);
    rp.minor = minor;
    rp.bdev = strdup(bdev);
    rp.cow = strdup(cow);
    rp.cache_size = 0;
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_RELOAD_SNAP, &rp);
        if (ret)
            LOG_ERROR("datto%d(%s : cach_size:%llu) with cow %s reload snapshot error", minor, bdev, cache_size, cow);
        --count;
    } while (ret && count >0);

    close(fd);
    return ret;
}
int dattobd::dattobd_reload_incremental(unsigned int minor, const char *bdev, const char *cow, unsigned long cache_size)
{
    FUNC_TRACER;
    int fd, ret;
    reload_params rp;

    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }
    LOG_TRACE("minor = %u, bdev=%s, cow = %s, cache_size = %u", minor, bdev, cow, cache_size);
    rp.minor = minor;
    rp.bdev = strdup(bdev);
    rp.cow = strdup(cow);
    rp.cache_size = 0;
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_RELOAD_INC, &rp);
        if (ret)
            LOG_ERROR("datto%d(%s : cach_size:%llu) with cow %s reload snapshot error", minor, bdev, cache_size, cow);
        --count;
    } while (ret && count >0);
    close(fd);
    return ret;
}
int dattobd::dattobd_destroy(unsigned int minor)
{
    FUNC_TRACER;
    int fd, ret;

    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_DESTROY, &minor);
        if (ret)
            LOG_ERROR("datto%d destroy snapshot error", minor);
        --count;
    } while (ret && count >0);
    close(fd);
    return ret;
}
int dattobd::dattobd_transition_incremental(unsigned int minor)
{
    FUNC_TRACER;
    int fd, ret;

    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_TRANSITION_INC, &minor);
        if (ret) // need check
            LOG_ERROR("datto%d trans to inc error", minor);
        --count;
    } while (ret && count >0);
    close(fd);
    return ret;
}
int dattobd::dattobd_transition_snapshot(unsigned int minor, const char *cow, unsigned long fallocated_space)
{
    FUNC_TRACER;
    int fd, ret;
    transition_snap_params tp;

    tp.minor = minor;
    tp.cow = strdup(cow);

    tp.fallocated_space = fallocated_space;

    fd = open("/dev/datto-ctl", O_RDONLY);

    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_TRANSITION_SNAP, &tp);
        if (ret)
            LOG_ERROR("datto%d with cow %s trans to snapshot error", minor, cow);
        --count;
    } while (ret && count >0);
    close(fd);
    return ret;
}
int dattobd::dattobd_reconfigure(unsigned int minor, unsigned long cache_size)
{
    FUNC_TRACER;
    int fd, ret;
    reconfigure_params rp;

    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }

    rp.minor = minor;
    rp.cache_size = cache_size;
    int count = 3;
    do
    {
        ret = ioctl(fd, IOCTL_RECONFIGURE, &rp);
        if (ret)
            LOG_ERROR("datto%d(cache_size = %llu) reconfigure error", minor, cache_size);
        --count;
    } while (ret && count >0);
    close(fd);
    return ret;
}
int dattobd::dattobd_get_info(unsigned int minor, struct dattobd_info *info, string datto_version)
{
    FUNC_TRACER;
    int fd, ret;

    if (!info) {
        errno = EINVAL;
        return -1;
    }

    fd = open("/dev/datto-ctl", O_RDONLY);
    if (fd < 0)
    {
        LOG_ERROR("open datto-ctl error\r\n");
        return -1;
    }
    info->minor = minor;
    LOG_TRACE("errno = %d", errno);
    LOG_TRACE("datto_version = %s", datto_version.c_str());
    system_tools::version target(string("0.10.6"));
    system_tools::version dattos(datto_version);
    LOG_TRACE("YEAH~");
    if (dattos < target)
    {
        ret = ioctl(fd, IOCTL_DATTOBD_INFO, info);
        if (ret)
        {
            LOG_ERROR("datto%d get info error\r\n", minor);
        }
    }
    else
    {
        struct dattobd_info_10_6 info_10_6;
        info_10_6.minor = minor;
        ret = ioctl(fd, IOCTL_DATTOBD_INFO_10_6, &info_10_6);
        if (ret)
        {
            LOG_ERROR("datto%d get info error\r\n", minor);
        }
        else
        {
            info->minor = info_10_6.minor;
            info->state = info_10_6.state;
            info->error = info_10_6.error;
            info->cache_size = info_10_6.cache_size;
            info->falloc_size = info_10_6.falloc_size;
            info->seqid = info_10_6.seqid;
            char uuid[COW_UUID_SIZE];
            char cow[PATH_MAX];
            char bdev[PATH_MAX];
            memcpy(info->uuid, &info_10_6.uuid, sizeof(info_10_6.uuid));
            memcpy(info->cow, &info_10_6.cow, sizeof(info_10_6.cow));
            memcpy(info->bdev, &info_10_6.bdev, sizeof(info_10_6.bdev));
        }
    }
    close(fd);
    LOG_TRACE("info->minor = %u", info->minor);
    LOG_TRACE("info->state = %lu", info->state);
    LOG_TRACE("info->error = %d", info->error);
    LOG_TRACE("info->cache_size = %lu", info->cache_size);
    LOG_TRACE("info->falloc_size = %llu", info->falloc_size);
    LOG_TRACE("info->seqid = %llu", info->seqid);
    return ret;
}

int snapshot_instance::setup_snapshot() {
    FUNC_TRACER;
    int ret = dattobd::dattobd_setup_snapshot(_minor, block_device_path.c_str(), abs_cow_path.c_str(), _fallocated_space, _cache_size);
    if (ret == 0)
    {
        ret = get_info_and_set_uuid();
        if (ret == 0)
        {
            total_blocks = got_block_size();
            if (!total_blocks)
            {
                LOG_ERROR("error opening snapshot\n");
                return -1;
            }
            total_chunks = (total_blocks + INDEX_BUFFER_SIZE - 1) / INDEX_BUFFER_SIZE;
            datto_device = "/dev/datto" + to_string(di.minor);
            LOG_TRACE("datto_device = %s\r\n", datto_device.c_str());
            /*src = general_io_rw::open_rw(datto_device);
            if (!src) {
            LOG_ERROR("error opening snapshot_rw\n");
            return -1;
            }*/
            set_creation_time(boost::posix_time::microsec_clock::universal_time()); //this info is not stable
        }
    }
    return ret;
}

int snapshot_instance::reload() {
    FUNC_TRACER;
    int ret = -1;
    string p = boost::filesystem::path(abs_cow_path).filename().string();
    if(state==3)
        ret = dattobd::dattobd_reload_snapshot(_minor, block_device_path.c_str(), p.c_str(), _cache_size);
    else if(state == 2)
        ret = dattobd::dattobd_reload_incremental(_minor, block_device_path.c_str(), p.c_str(), _cache_size);
    if(!ret)
        ret = get_info_and_set_uuid();
    return ret;
}
int snapshot_instance::transition_snapshot(unsigned long cache_size) {
    FUNC_TRACER;
    _cache_size = cache_size;
    std::string ori_previous_cow_path = previous_cow_path;
    previous_cow_path = abs_cow_path;
    index = (index + 1) % MAX_COW_INDEX;
    abs_cow_path = mp->get_mounted_on() + SNAPSHOT_NAME + to_string(index);
    LOG_TRACE("abs_cow_path = %s, previous_cow_path = %s, ori_previous_cow_path = %s\r\n", abs_cow_path.c_str(), previous_cow_path.c_str(), ori_previous_cow_path.c_str());
    linux_storage::storage::ptr str = linux_storage::storage::get_storage();
    partitionA::vtr parts = str->get_all_partition();
    for (auto & p : parts)
    {
        if (p->ab_path == block_device_path)
        {
            if (p->mp && p->mp->get_mounted_on().empty()) //the device being unounted by others, mount it back!
            {
                LOG_TRACE("the device %s is unmount from %s, try to mount it back!\r\n", block_device_path.c_str(), mp->get_mounted_on().c_str());
                string cmd_result;
                string cmd = "mount " + block_device_path + " " + mp->get_mounted_on();
                cmd_result = system_tools::execute_command(cmd.c_str());
                if (!cmd_result.empty()) //mount fail
                {
                    LOG_TRACE("mount fail, destroy snapshot.");
                    destroy();
                    return -1;
                }
                LOG_TRACE("mount back success,GOOD.");
            }
        }
    }
    /**/
    if (!ori_previous_cow_path.empty())
    {
        LOG_TRACE("ori_previous_cow_path = %s\r\n remove in", ori_previous_cow_path.c_str());
        boost::filesystem::path p = boost::filesystem::path(ori_previous_cow_path);
        if (boost::filesystem::exists(p))
        {
            LOG_TRACE("ori_previous_cow_path = %s\r\n remove start", ori_previous_cow_path.c_str());
            if (remove(ori_previous_cow_path.c_str()))
                LOG_ERROR("ori_previous_cow_path = %s is not exist\r\n", ori_previous_cow_path.c_str());
        }
    }
    /*boost::filesystem::path pp = boost::filesystem::path(ori_previous_cow_path);
    if (boost::filesystem::exists(pp))
    {
        LOG_TRACE("ori_previous_cow_path = %s\r\n still exist!", ori_previous_cow_path.c_str());
    }
    boost::filesystem::path ppp = boost::filesystem::path(previous_cow_path);
    if (boost::filesystem::exists(ppp))
    {
        LOG_TRACE("previous_cow_path = %s\r\n still exist!", previous_cow_path.c_str());
    }
    boost::filesystem::path pppp = boost::filesystem::path(abs_cow_path);
    if (boost::filesystem::exists(pppp))
    {
        LOG_TRACE("abs_cow_path = %s\r\n still exist!", abs_cow_path.c_str());
    }*/
    int ret = dattobd::dattobd_transition_snapshot(_minor, abs_cow_path.c_str(), _cache_size);
    if (ret)
    {
        LOG_ERROR("trans to snapshot error\r\n");
    }
    else
    {
        ret = get_info_and_set_uuid();
        set_creation_time(boost::posix_time::microsec_clock::universal_time()); //this info is not stable
    }
    return ret;
}
int snapshot_instance::transition_incremental() {
    FUNC_TRACER;

    int ret = dattobd::dattobd_transition_incremental(_minor);
    if (!ret)
        ret = get_info_and_set_uuid();
    return ret;
}

void snapshot_instance::clear_all_cow()
{
    FUNC_TRACER;
    for (int i = 0; i < MAX_COW_INDEX; ++i)
    {
        string cow_file_path = mp->get_mounted_on() + SNAPSHOT_NAME + to_string(i);
        remove(cow_file_path.c_str());
    }
}
int snapshot_instance::destroy(bool total_destroyed) {
    FUNC_TRACER; 
    int result = dattobd::dattobd_destroy(_minor);
    if (result)
    {
        LOG_ERROR("dattobd_destroy %d error", result);
    }
    else
    {
        clear_all_cow();
        destroyed = total_destroyed;
    }
    return result;
}
int snapshot_instance::reconfigure(unsigned long cache_size) {
    FUNC_TRACER;
    _cache_size = cache_size;
    dattobd::dattobd_reconfigure(_minor, _cache_size);
}

int snapshot_instance::get_info_and_set_uuid() {
    int ret;
    int retry_count = 5;
    ret = get_info(true);
    if (!ret)
        setting_uuid();
    return ret;
}

void snapshot_instance::setting_uuid() 
{
    FUNC_TRACER;
    if (uuid.empty())
        uuid = system_tools::gen_random_uuid();
}

int snapshot_instance::get_info(bool retry ) {
    FUNC_TRACER;
    int ret;
    int retry_count = (retry)?5:1;
    do
    {
        ret = dattobd::dattobd_get_info(_minor, &di, datto_version);
        if (ret)
            sleep(5);
    } while (ret && --retry_count);
    if (ret)
    {    
        LOG_ERROR("get datto%d info error\r\n", _minor);
        return ret;
    }
    /*GUID * guid = (GUID *)di.uuid;
    uuid = guid->to_string();*/
    //printf("uuid = %s\r\n", uuid.c_str());
    _fallocated_space = di.falloc_size >> 20;
    LOG_TRACE("_fallocated_space = %llu", _fallocated_space);
    _cache_size = di.cache_size >> 20 ;
    state = di.state;
    return ret;
}
int snapshot_instance::copy(bool b_full)
{
    FUNC_TRACER;
    int ret;
    FILE *cow = NULL;
    int snap_fd = 0; //file description
    size_t snap_size, bytes, blocks_to_read;
    sector_t i, j, blocks_done = 0, count = 0, err_count = 0;
    uint64_t *mappings = NULL;
    string tempstring, abs_cow_path;
    string cow_file_path = previous_cow_path;
    std::string str_dst = "/image" + to_string(get_minor());
    universal_disk_rw::ptr src;
    universal_disk_rw::ptr dst;
    src = get_src_rw();
    if (!src)
    {
        ERROR_HANDLE("set_src failed\r\n");
    }
    dst = general_io_rw::open_rw(str_dst, false);
    if (!dst)
    {
        ERROR_HANDLE("set_dst : %s failed\r\n", str_dst.c_str());
    }
    
    if (!b_full)
    {
        if (previous_cow_path.empty())
        {
            ERROR_HANDLE("previous_cow_path empty\n");
        }
        LOG_TRACE("previous_cow_path.c_str() = %s\r\n", previous_cow_path.c_str());
        cow = fopen(previous_cow_path.c_str(), "r"); //maybe this is not nessuary for full dd
        if (!cow) {
            ERROR_HANDLE("error opening cow file\n");
        }
        if (verify_files(cow))
        {
            ERROR_HANDLE("verify files error\n");
        }
    }
    tempstring = "/image" + to_string(di.minor);

    LOG_TRACE("snapshot is %llu blocks large\n", total_blocks);

    mappings = (uint64_t *)malloc(INDEX_BUFFER_SIZE * sizeof(uint64_t));
    if (!mappings) {
        ERROR_HANDLE("error allocating mappings\n");
    }
    if (b_full)
    {
        for (int i = 0; i < INDEX_BUFFER_SIZE; ++i)
            mappings[i] = 1;
    }
    LOG_TRACE("copying blocks : total_chunks = %d\n", total_chunks);
    for (i = 0; i < total_chunks; i++) {
        //read a chunk of mappings from the cow file
        blocks_to_read = MIN(INDEX_BUFFER_SIZE, total_blocks - blocks_done);

        if (b_full) {}
        else
        {
            bytes = pread(fileno(cow), mappings, blocks_to_read * sizeof(uint64_t), COW_HEADER_SIZE + (INDEX_BUFFER_SIZE * sizeof(uint64_t) * i));
            if (bytes != blocks_to_read * sizeof(uint64_t)) {
                ERROR_HANDLE("error reading mappings into memory\n");
            }
        }

        //copy blocks where the mapping is set
        for (j = 0; j < blocks_to_read; j++) {
            if (!mappings[j]) continue;

            ret = copy_block(src,dst,(INDEX_BUFFER_SIZE * i) + j); // NULL is just test
            if (ret) err_count++;

            count++;
        }

        blocks_done += blocks_to_read;
    }

    //print number of blocks changed
    LOG_TRACE("copying complete: %llu blocks changed, %llu errors\n", count, err_count);

error:
    if (mappings) free(mappings);
    if (cow) fclose(cow);
    //rm_dst();
    //rm_src();
    return ret;
}

int snapshot_instance::copy_by_changed_area(bool b_full)
{
    FUNC_TRACER;
    int ret;
    FILE *cow = NULL;
    int snap_fd = 0; //file description
    size_t snap_size, bytes, blocks_to_read;
    sector_t i, j, blocks_done = 0, count = 0, err_count = 0;
    string tempstring, abs_cow_path;
    string cow_file_path = previous_cow_path;
    changed_area::vtr cas;
    std::string str_dst = "/image" + to_string(get_minor());
    universal_disk_rw::ptr src;
    universal_disk_rw::ptr dst;
    src = get_src_rw();
    if (!src)
    {
        ERROR_HANDLE("set_src failed\r\n");
    }
    dst = general_io_rw::open_rw(str_dst, false);
    if (!dst)
    {
        ERROR_HANDLE("set_dst : %s failed\r\n", str_dst.c_str());
    }
    if (!b_full)
    {
        if (previous_cow_path.empty())
        {
            ERROR_HANDLE("previous_cow_path empty\n");
        }
        LOG_TRACE("previous_cow_path.c_str() = %s\r\n", previous_cow_path.c_str());
        cow = fopen(previous_cow_path.c_str(), "r"); //maybe this is not nessuary for full dd
        if (!cow) {
            ERROR_HANDLE("error opening cow file\n");
        }
        if (verify_files(cow))
        {
            ERROR_HANDLE("verify files error\n");
        }
    }

    LOG_TRACE("snapshot is %llu blocks large\n", total_blocks);
    cas = get_changed_area(b_full);
    //copy blocks where the mapping is set
    for (auto & ca : cas) {

        ret = copy_block_by_changed_area(src,dst,ca.start << 12, ca.length << 12); // NULL is just test
        if (ret) err_count++;
        count++;
    }
    //print number of blocks changed
    LOG_TRACE("copying complete: %llu blocks changed, %llu errors\n", count, err_count);
error:
    if (cow) fclose(cow);
    //rm_dst();
    //rm_src();
    return ret;
}

changed_area::vtr snapshot_instance::get_changed_area(bool b_full)
{
    FUNC_TRACER;
    changed_area::vtr out_changed_area;
    int i, j;
    size_t blocks_to_read = 0, blocks_done = 0, bytes = 0;
    uint64_t *mappings = (uint64_t *)malloc(INDEX_BUFFER_SIZE * sizeof(uint64_t));
    if (!mappings) {
        LOG_ERROR("error allocating mappings\n");
    }
    FILE * cow = NULL;
    if (!b_full)
    {
        cow = fopen(previous_cow_path.c_str(), "r"); //maybe this is not nessuary for full dd
        if (!cow) {
            LOG_ERROR("error opening cow file %s !! \r\n", previous_cow_path.c_str());
        }
        LOG_TRACE("previous_cow_path.c_str() = %s\r\n", previous_cow_path.c_str());
        if (verify_files(cow))
        {
            LOG_ERROR("verify files error\n");
        }
    }
    if (!mappings) {
        LOG_ERROR("error allocating mappings\n");
    }
    uint64_t block_count = 0;
    changed_area tempca;
    if (b_full)
    {
        out_changed_area.push_back(changed_area(block_device_offset, total_blocks, 0, get_src_rw()));
    }
    else
    {
        bool con = false;
        uint64_t start = 0, length = 0, src_start = 0;
        for (i = 0; i < total_chunks; i++) {
            //read a chunk of mappings from the cow file
            blocks_to_read = MIN(INDEX_BUFFER_SIZE, total_blocks - blocks_done);

            bytes = pread(fileno(cow), mappings, blocks_to_read * sizeof(uint64_t), COW_HEADER_SIZE + (INDEX_BUFFER_SIZE * sizeof(uint64_t) * i));
            if (bytes != blocks_to_read * sizeof(uint64_t)) {
                LOG_ERROR("error reading mappings into memory\n");
            }
            block_count = INDEX_BUFFER_SIZE * i;
            //copy blocks where the mapping is set
            for (j = 0; j < blocks_to_read; j++) {
                if (!mappings[j])
                {
                    if (con)
                    {
                        con = false;
                        out_changed_area.push_back(changed_area(block_device_offset + (src_start << 12), length << 12, src_start << 12, get_src_rw()));
                    }
                    continue;
                }
                if (!con)
                {
                    start = block_count + j;
                    src_start = start;
                    length = 1;
                }
                else
                {
                    ++length;
                }
                con = true;
            }
            blocks_done += blocks_to_read;
        }
        if (con)
        {
            con = false;
            out_changed_area.push_back(changed_area(block_device_offset + (src_start << 12), length << 12, src_start << 12, get_src_rw()));
        }
    }
    if (mappings) free(mappings);
    if (cow) fclose(cow);

    return out_changed_area;
}

sector_t snapshot_instance::got_block_size()
{
    FUNC_TRACER;
    sector_t total_blocks;
    size_t snap_size;
    FILE *snap = NULL;
    string tempstring = "/dev/datto" + to_string(di.minor);
    /*struct stat ss;
    while (stat(tempstring.c_str(), &ss) == -1);*/
    //int retry_count = 5;
    do
    {
        snap = fopen(tempstring.c_str(), "r");
        if(!snap)
            sleep(5);
    } while (!snap /*&& --retry_count*/);
    if (!snap) {
        LOG_ERROR("open snapshot file %s error\r\n", tempstring.c_str());
        return 0;
    }

    fseeko(snap, 0, SEEK_END);
    snap_size = ftello(snap);
    LOG_TRACE("snap_size = %llu\r\n", snap_size);
    total_blocks = (snap_size + COW_BLOCK_SIZE - 1) / COW_BLOCK_SIZE;
    fclose(snap);
    return total_blocks;
}

int snapshot_instance::copy_block_by_changed_area(universal_disk_rw::ptr src, universal_disk_rw::ptr dst, uint64_t start, uint64_t lenght) //maybe change to byte is better
{
    FUNC_TRACER;
    char buf[MAX_RW_BLOCK_SIZE];
    int ret;
    uint32_t bytes;
    uint64_t remain_bytes = lenght;
    uint64_t bytes_dones = 0;
    while (remain_bytes > 0)
    {
        uint64_t bytes_to_copy = MIN(MAX_RW_BLOCK_SIZE, remain_bytes);
        src->read(start, bytes_to_copy, buf, bytes);
        if (bytes != bytes_to_copy) {
            ret = errno;
            errno = 0;
            LOG_ERROR("error reading data block from snapshot\n");
            goto error;
        }
        dst->write(start, buf, bytes_to_copy, bytes);
        if (bytes != COW_BLOCK_SIZE) {
            ret = errno;
            errno = 0;
            LOG_ERROR("error writing data block to output image\n");
            goto error;
        }
        remain_bytes -= bytes_to_copy;
        bytes_dones += bytes_to_copy;
        start += bytes_to_copy;
    }
    return 0;
error:
    LOG_ERROR("error copying sector to output image\n");
    return ret;
}

int snapshot_instance::copy_block(universal_disk_rw::ptr src, universal_disk_rw::ptr dst, sector_t block)
{
    FUNC_TRACER;
    char buf[COW_BLOCK_SIZE];
    int ret;
    uint32_t bytes;

    if (get_block(src,buf, COW_BLOCK_SIZE, block)) {
        LOG_ERROR("error reading data block from snapshot\n");
        goto error;
    }
    dst->write(block * COW_BLOCK_SIZE, buf, COW_BLOCK_SIZE, bytes);
    if (bytes != COW_BLOCK_SIZE) {
        ret = errno;
        errno = 0;
        LOG_ERROR("error writing data block to output image\n");
        goto error;
    }
    return 0;
error:
    LOG_ERROR("error copying sector to output image\n");
    return ret;
}

int snapshot_instance::get_block(universal_disk_rw::ptr src, char * buf, int size, sector_t block)
{
    FUNC_TRACER;
    int ret;
    uint32_t bytes;
    if (size < COW_BLOCK_SIZE)
    {
        LOG_ERROR("error buf size is too small\n");
        goto error;
    }
    src->read(block * COW_BLOCK_SIZE, COW_BLOCK_SIZE, buf, bytes);
    /*bytes = pread(fileno(snap), buf, COW_BLOCK_SIZE, block * COW_BLOCK_SIZE);*/
    if (bytes != COW_BLOCK_SIZE) {
        ret = errno;
        errno = 0;
        LOG_ERROR("error reading data block from snapshot\n");
        goto error;
    }
    return 0;
error:
    LOG_ERROR("error copying sector to output image\n");
    return ret;
}

int snapshot_instance::verify_files(FILE *cow)
{
    FUNC_TRACER;
    int ret;
    dattobd::cow_header ch;
    size_t bytes;

    bytes = pread(fileno(cow), &ch, sizeof(dattobd::cow_header), 0);
    if (bytes != sizeof(dattobd::cow_header)) {
        ret = errno;
        errno = 0;
        LOG_ERROR("error reading cow header\n");
        goto error;
    }

    //check the cow file's magic number
    if (ch.magic != COW_MAGIC) {
        ret = EINVAL;
        LOG_ERROR("invalid magic number from cow file\n");
        goto error;
    }

    //check the uuid
    if (memcmp(ch.uuid, di.uuid, COW_UUID_SIZE) != 0) {
        ret = EINVAL;
        LOG_ERROR("cow file uuid does not match snapshot\n");
        goto error;
    }

    //check the sequence id
    LOG_TRACE("ch.seqid = %d , di.seqid = %d\r\n", ch.seqid, di.seqid);
    if (ch.seqid != di.seqid - 1) {
        ret = EINVAL;
        LOG_ERROR("snapshot provided does not immediately follow the snapshot that created the cow file\n");
        goto error;
    }
    return 0;
error:
    return ret;
}

bool snapshot_instance::set_snapshot_incremental(bool set_uuid)
{
    int total_retry_count = 5;
    bool setup_success = false;
    int ret = false;
    while (!setup_success && total_retry_count)
    {
        int setup_snapshot_retry_count = 5;
        while (setup_snapshot() && setup_snapshot_retry_count)
        {
            setup_snapshot_retry_count--;
            LOG_ERROR("create %s mounted on %s snapshot minor %d failed please see dmesg.,setup_snapshot_retry_count = %d", get_block_device_path().c_str(), get_abs_cow_path().c_str(), _minor, setup_snapshot_retry_count);
            sleep(1);
        }
        if (setup_snapshot_retry_count == 0)
            return false;
        LOG_TRACE("create %s mounted on %s snapshot minor %d OK!!!!!!!!!!!!!", get_block_device_path().c_str(), get_abs_cow_path().c_str(), _minor);
        int get_info_retry_count = 5;
        while ((set_uuid)?get_info_and_set_uuid() : get_info() && get_info_retry_count)
        {
            LOG_ERROR("dattobd_info snapshot minor %d failed, now retry count %d", _minor, get_info_retry_count);
            get_info_retry_count--;
            sleep(1);
        }
        if (get_info_retry_count == 0)
        {
            destroy();
        }
        else
        {
            setup_success = true;
        }
        total_retry_count--;
    }
    if (setup_success)
    {
        sleep(1);
        ret = transition_incremental(); //transto inc mode first
        if (ret)
        {
            LOG_ERROR("dattobd_transition_incremental failed, now retry count");
        }
    }
    return (ret == 0) ? true : false;
}

bool snapshot_instance::mount_snapshot()
{
    boost::filesystem::path dattodevice = boost::filesystem::path(datto_device);
    snapshot_mount_point = "/" + dattodevice.filename().string() + "/";
    mkdir(snapshot_mount_point.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    string mount = "mount ";
    if (get_mounted_point()->get_fstype().find("xfs")!=std::string::npos)
    {
        mount += "-o nouuid,ro,norecovery ";
    }

    string mount_command = mount + datto_device + " "+ snapshot_mount_point;
    string result = system_tools::execute_command(mount_command.c_str());
    return result.empty();
}

bool snapshot_instance::unmount_snapshot()
{
    string mount_command = "umount " + snapshot_mount_point;
    string result = system_tools::execute_command(mount_command.c_str());
    snapshot_mount_point.clear();
    return result.empty();
}

snapshot_instance::ptr snapshot_manager::take_snapshot_block_dev(disk_baseA::ptr block_dev, disk_baseA::ptr dc) //block_dev block_dev , dc parent's device
{
    FUNC_TRACER;
    LOG_TRACE("block_dev->get_ab_path() = %s", block_dev->get_ab_path().c_str());
    snapshot_instance::ptr si = NULL;
    /*we should find the block_dev is snapshotted by */
    if (snapshots_map_by_parttions.count(block_dev->get_ab_path()))
    {
        return NULL;//snapshots_map_by_parttions[block_dev->get_ab_path()];
    }

    if (block_dev->mp && !block_dev->mp->get_mounted_on().empty())
    {
        if (!snapshot_exist(block_dev))
        {
            dattobd::dattobd_info di;
            int i;
            for (i = 0;; ++i)
            {
                if (!snapshot_map.count(i) && dattobd::dattobd_get_info(i, &di, datto_version) != 0)
                    break;
            }
            for (int i = 0; i < MAX_COW_INDEX; ++i)
            {
                string cow_file_path = block_dev->mp->get_mounted_on() + SNAPSHOT_NAME + to_string(i);
                remove(cow_file_path.c_str());
            }
            /*initial snapshot*/
            si = snapshot_instance::ptr(new snapshot_instance());
            si->set_disk_path(dc->ab_path);
            si->set_block_device_path(block_dev->ab_path);
            if(block_dev->mp)
                si->set_mount_point(block_dev->mp);
            if (block_dev->block_type == disk_baseA::block_dev_type::partition ||
                block_dev->block_type == disk_baseA::block_dev_type::lvm)
            {
                partitionA::ptr partition = boost::static_pointer_cast<partitionA>(block_dev);
                si->set_block_device_offset(partition->partition_start_offset);
            }     
            else
                si->set_block_device_offset(0);
            si->set_block_device_size(block_dev->blocks);
            disk::ptr dsk = boost::static_pointer_cast<disk>(dc);
            LOG_TRACE("dsk->string_uri = %s", dsk->string_uri.c_str());
            si->set_string_uri(dsk->string_uri);
            si->set_index(0);
            si->set_abs_cow_path(si->get_mounted_point()->get_mounted_on() + SNAPSHOT_NAME + std::to_string(si->get_index()));
            si->set_minor(i);
            int cow_ratio = 10;
            if (snapshots_cow_space_user_config.count(si->get_block_device_path())!=0)
                cow_ratio = (snapshots_cow_space_user_config[si->get_block_device_path()] > 20) ? 20 : (snapshots_cow_space_user_config[si->get_block_device_path()] < 5) ? 5 : snapshots_cow_space_user_config[si->get_block_device_path()];
            si->set_cow_ratio(cow_ratio);
            si->set_fallocated_space((block_dev->blocks*si->get_cow_ratio() /100)>>20);
            //LOG_TRACE("partition-> = %llu", partition->blocks * 5 / 100);
            si->set_cache_size(0);
            si->set_datto_version(datto_version);
            if (si->set_snapshot_incremental(true))
            {
                snapshot_map[i] = si;
                LOG_TRACE("snapshot_map[%d] = %s!\r\n", i, si->get_block_device_path().c_str());
                write_snapshot_config_file();
            }
            else
            {
                si = NULL;
            }
        }
    }
    return si;
}

snapshot_instance::vtr snapshot_manager::take_snapshot_by_disk_ab_path(std::string device_ab_path, unsigned char * result, storage::ptr str)
{
    FUNC_TRACER;
    snapshot_instance::vtr out;
    if (str == NULL)
    {
        str = storage::get_storage();
    }
    std::set<std::string> device_ab_paths;
    device_ab_paths.insert(device_ab_path);
    disk_baseA::ptr dc = str->get_instance_from_ab_path(device_ab_path);
    if (dc->block_type == disk_baseA::partition)
    {
        partitionA::ptr p = boost::static_pointer_cast<partitionA>(dc);
        snapshot_instance::ptr si = take_snapshot_block_dev(p, p->parent_disk);
        if (si != NULL)
            out.push_back(si);
    }
    else if (dc->block_type == disk_baseA::disk)
    {
        disk::ptr dc = boost::static_pointer_cast<disk>(dc);
        for (auto & p : dc->partitions)
        {
            snapshot_instance::ptr si = take_snapshot_block_dev(p, dc);
            if (si != NULL)
                out.push_back(si);
        }
    }
    return out;
}

void snapshot_manager::clear_umount_snapshots(storage::ptr str)
{
    if (str == NULL)
    {
        str = storage::get_storage();
    }
    disk::vtr all_disk = str->get_all_disk();
    mount_point::vtr mps;
    snapshot_instance::vtr sis;
    for (auto & dsk : all_disk)
    {
        for (auto & pt : dsk->partitions)
        {
            if (pt->mp != NULL && std::count(mps.begin(), mps.end(), pt->mp) == 0)
                mps.push_back(pt->mp);
            for (auto & lvm : pt->lvms)
            {
                if (lvm->mp != NULL && std::count(mps.begin(), mps.end(), lvm->mp) == 0)
                    mps.push_back(lvm->mp);
            }
        }
    }
    /*add code to get the systemuuid special fit with datto device if the datto deivice state is 3*/
    if (!mps.empty())
    {
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
        for (auto & snapshot : snapshot_map)
        {
            if (snapshot.second->get_dattobd_info().state == 3)
            {
                int fr = snapshot.second->get_datto_device().rfind("/");
                if (fr != string::npos)
                {
                    string device_filename = snapshot.second->get_datto_device().substr(fr + 1);
                    if (uuids.count(device_filename))
                    {
                        for (auto & mp : mps)
                        {
                            if (snapshot.second->get_block_device_path() == mp->get_device())
                            {
                                mp->set_system_uuid(uuids[device_filename]);
                            }
                        }
                    }
                }
            }
        }
        for (auto & snapshot : snapshot_map)
        {
            bool miss = true;
            for (auto & mp : mps)
            {
                LOG_TRACE("snapshot.second->mp->get_system_uuid() = %s", snapshot.second->get_mounted_point()->get_system_uuid().c_str());
                LOG_TRACE("mp->get_system_uuid() = %s", mp->get_system_uuid().c_str());
                if (snapshot.second->get_mounted_point()->get_system_uuid() == mp->get_system_uuid())
                {
                    miss = false;
                    break;
                }
            }
            if (miss)
                sis.push_back(snapshot.second);
        }
    }
    destroy_snapshots(sis);
    write_snapshot_config_file();
    return;
}

void snapshot_manager::remove_btrfs_under_a_disk_device(disk_baseA::ptr pdb)
{
    /*first check there are a */
    FUNC_TRACER;
    btrfs_snapshot_instance::vtr::iterator bs_node;
    LOG_TRACE("btrfs_snapshot_vtr.size() = %d", btrfs_snapshot_vtr.size());
    for (bs_node = btrfs_snapshot_vtr.begin(); bs_node != btrfs_snapshot_vtr.end(); ++bs_node)
    {
        LOG_TRACE("pdb->mps[0]->get_mounted_on() = %s", pdb->mps[0]->get_mounted_on().c_str());
        LOG_TRACE("(*bs_node)->get_mounted_points()[0]->get_mounted_on() = %s", (*bs_node)->get_mounted_points()[0]->get_mounted_on().c_str());
        if (pdb->mps[0]->get_mounted_on() == (*bs_node)->get_mounted_points()[0]->get_mounted_on())
        {
            if (!(*bs_node)->delete_snapshot())
            {
                btrfs_snapshot_vtr.erase(bs_node);
                break;//i think it sould be continue;
            }
            else
            {
                LOG_ERROR("delete btrfs snapshot error.");
                break;
            }
        }
    }
}

btrfs_snapshot_instance::vtr snapshot_manager::take_btrfs_snapshot_by_disk_uri(std::set<std::string> disks_uri , storage::ptr str)
{
    FUNC_TRACER;
    btrfs_snapshot_instance::vtr out;
    if (str == NULL)
    {
        str = storage::get_storage();
    }
    /* this is for the case that filesystem is directly build on the raw disk not on the partition */
    disk::vtr all_disk = str->get_all_disk();
    for (auto & dsk : all_disk)
    {
        for (auto & uris : disks_uri)
        {
            if (uris == dsk->string_uri &&
                !dsk->mps.empty() &&
                !dsk->mps[0]->get_mounted_on().empty() &&
                dsk->mps[0]->get_fstype() == "btrfs")
            {
                /*first check there are a */
                remove_btrfs_under_a_disk_device(dsk);
                //init snapshot first
                btrfs_snapshot_instance::ptr bsi = btrfs_snapshot_instance::ptr(new btrfs_snapshot_instance(dsk->ab_path, dsk->mps));
                //take snapshot
                if (bsi->take_snapshot())
                {
                    LOG_ERROR("btrfs_snapshot_instance take snapshot error.");
                    bsi = NULL;
                    continue;
                }
                btrfs_snapshot_vtr.push_back(bsi);
            }
        }
    }
    std::set<std::string> partition_ab_path = str->enum_partition_ab_path_from_uri(disks_uri);

    disk_baseA::vtr partitions = str->enum_multi_instances_from_multi_ab_paths(partition_ab_path);
    LOG_TRACE("partitions.size() = %d\r\n", partitions.size());
    for (auto & p : partitions)
    {
        partitionA::ptr pc = boost::static_pointer_cast<linux_storage::partitionA>(p);
        LOG_TRACE("partition_ab_path = %s\r\n", p->ab_path.c_str());
        for (auto & lv : pc->lvms)
        {
            LOG_TRACE("partition_ab_path = %s\r\n", p->ab_path.c_str());
            if (!lv->mps.empty() && lv->mps[0]->get_mounted_on().size() > 0 && lv->mps[0]->get_fstype() == "btrfs")
            {
                remove_btrfs_under_a_disk_device(lv);
                //init snapshot first
                btrfs_snapshot_instance::ptr bsi = btrfs_snapshot_instance::ptr(new btrfs_snapshot_instance(lv->ab_path, lv->mps));
                //take snapshot
                if (bsi->take_snapshot())
                {
                    LOG_ERROR("btrfs_snapshot_instance take snapshot error.");
                    bsi = NULL;
                    continue;
                }
                btrfs_snapshot_vtr.push_back(bsi);
            }
        }
        if (!pc->mps.empty() && pc->mps[0]->get_mounted_on().size() > 0 && pc->mps[0]->get_fstype() == "btrfs")
        {
            remove_btrfs_under_a_disk_device(pc);
            //init snapshot first
            btrfs_snapshot_instance::ptr bsi = btrfs_snapshot_instance::ptr(new btrfs_snapshot_instance(pc->ab_path, pc->mps));
            //take snapshot
            if (bsi->take_snapshot())
            {
                LOG_ERROR("btrfs_snapshot_instance take snapshot error.");
                bsi = NULL;
                continue;
            }
            btrfs_snapshot_vtr.push_back(bsi);
            out.push_back(bsi);
        }
    }
    return out;
}

snapshot_instance::vtr snapshot_manager::take_snapshot_by_disk_uri(std::set<std::string> disks_uri, unsigned char * result, storage::ptr str)
{
    FUNC_TRACER;
    snapshot_instance::vtr out;
    if (str == NULL)
    {
        str = storage::get_storage();
    }
    /* this is for the case that filesystem is directly build on the raw disk not on the partition */
    disk::vtr all_disk = str->get_all_disk();
    for (auto & dsk : all_disk)
    {
        if (snapshot_exist(dsk))
            continue;
        for (auto & uris : disks_uri)
        {
            if (uris == dsk->string_uri &&
                dsk->mp &&
                !dsk->mp->get_mounted_on().empty() &&
                dsk->mp->get_fstype()!="btrfs")
            {
                snapshot_instance::ptr si = take_snapshot_block_dev(dsk, dsk);
                if (si != NULL)
                    out.push_back(si);
                else
                {
                    LOG_ERROR("take_snapshots on dsk %s error" ,dsk->ab_path.c_str());
                    *result = 1;
                    //snapshot_exist(dsk);
                }
            }
        }
    }
    /* this is for the case that filesystem is directly build on the raw disk not on the partition */

    std::set<std::string> partition_ab_path = str->enum_partition_ab_path_from_uri(disks_uri);
    
    disk_baseA::vtr partitions = str->enum_multi_instances_from_multi_ab_paths(partition_ab_path);
    LOG_TRACE("partitions.size() = %d\r\n", partitions.size());
    for (auto & p : partitions)
    {
        partitionA::ptr pc = boost::static_pointer_cast<linux_storage::partitionA>(p);
        LOG_TRACE("partition_ab_path = %s\r\n", p->ab_path.c_str());
        if (snapshot_exist(pc))
            continue;
        for (auto & lv : pc->lvms)
        {
            LOG_TRACE("partition_ab_path = %s\r\n", p->ab_path.c_str());
            if (snapshot_exist(lv))
                continue;
            if (lv->mp && lv->mp->get_mounted_on().size() > 0 && lv->mp->get_fstype() != "btrfs")
            {              
                snapshot_instance::ptr si = take_snapshot_block_dev(lv, lv->parent_disk);
                if (si != NULL)
                {
                    si->set_lvm();
                    out.push_back(si);
                }
                else
                {
                    LOG_ERROR("(lv->mounted_path.size() > 0)%s take_snapshot_partitione failed!, maybe the snapshot is already set.\r\n", lv->parent_disk->ab_path.c_str());
                    *result = 1;
                    //snapshot_exist(lv);
                }
            }
        }        
        if (pc->mp && pc->mp->get_mounted_on().size() > 0 && pc->mp->get_fstype() != "btrfs")
        {
            snapshot_instance::ptr si = take_snapshot_block_dev(pc, pc->parent_disk);
            if (si != NULL)
                out.push_back(si);
            else
            {
                LOG_ERROR("(pc->mounted_path.size() > 0)%s take_snapshot_partitione failed!, maybe the snapshot is already set.\r\n", pc->parent_disk->ab_path.c_str());
                *result = 1;
                //snapshot_exist(pc);
            }
        }
    }
    return out;
}

int snapshot_manager::write_snapshot_config_file()
{
    FUNC_TRACER;
    ptree root;
    root.put("datto_version", datto_version);
    ptree snapshots;
    for (auto & m : snapshot_map)
    {
        ptree psnapshot;
        auto & s = m.second;
        psnapshot.put("minor", m.first);
        psnapshot.put("index", s->get_index());
        psnapshot.put("block_device_path", s->get_block_device_path());
        LOG_TRACE("string_uri = %s", s->get_string_uri().c_str());
        psnapshot.put("string_uri", s->get_string_uri());
        psnapshot.put("disk_path", s->get_disk_path());
        psnapshot.put("datto_device", s->get_datto_device());
        psnapshot.put("abs_cow_path", s->get_abs_cow_path());
        psnapshot.put("previous_cow_path", s->get_previous_cow_path());
        psnapshot.put("total_chunks", s->get_total_chunks());
        psnapshot.put("total_blocks", s->get_total_blocks());
        psnapshot.put("creation_time", boost::posix_time::to_simple_string(s->get_creation_time()));
        psnapshot.put("fallocated_space", s->get_fallocated_space());
        psnapshot.put("cache_size", s->get_cache_size());
        psnapshot.put("device_block_offset", s->get_block_device_offset());
        psnapshot.put("device_block_size", s->get_block_device_size());
        psnapshot.put("uuid", s->get_uuid());
        psnapshot.put("set_id", s->get_set_uuid());
        psnapshot.put("cow_ratio", s->get_cow_ratio());
        psnapshot.put("state", s->get_state());
        psnapshot.put("is_lvm", s->get_lvm());
        snapshots.push_back(std::make_pair("", psnapshot));
    }
    root.add_child("snapshots", snapshots);
    ptree btrfs_snapshots;

    for (auto & m : btrfs_snapshot_vtr)
    {
        ptree psnapshot;
        auto & s = m;
        psnapshot.put("disk_path", s->get_disk_path());
        psnapshot.put("snapshot_path", s->get_snapshot_path());
        psnapshot.put("uuid", s->get_uuid());
        psnapshot.put("set_uuid", s->get_set_uuid());
        btrfs_snapshots.push_back(std::make_pair("", psnapshot));
    }
    root.add_child("btrfs_snapshots", btrfs_snapshots);

    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    p = p / config_filename;
    fstream fs;
    fs.open(p.string(), ios::out | ios::trunc);
    if (fs)
    {
        write_json(fs, root, true);
        fs.close();
    }
    else
    {
        LOG_ERROR("%s create failed!\r\n", config_filename.c_str());
    }
    snapshot_map_by_uri.clear();
    snapshots_map_by_parttions.clear();
    snapshot_map_by_disks.clear();

    enumerate_snapshots_by_uri(snapshot_map_by_uri);
    enumerate_snapshots_by_parttions(snapshots_map_by_parttions);
    enumerate_snapshots_by_disks(snapshot_map_by_disks);

    //read_snapshot_config_file(); //read after write to ensure the context is latest.
    return 0;
}
using namespace json_tools;

int snapshot_manager::only_read_snapshot_config_file()
{
    FUNC_TRACER;
    ptree root;
    LOG_TRACE("config_filename = %s\r\n", config_filename.c_str());
    boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
    p = p / config_filename;
    try {
        read_json(p.string(), root);
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("read %s fail\r\n", p.string().c_str());
    }
    if (!root.empty())
    {
        LOG_TRACE("start getting snapshots info!\r\n");
        datto_version = boost_json_get_value_helper<string>(root, "datto_version", "");
        LOG_TRACE("datto_version = %s", datto_version.c_str());
        if (datto_version.size() == 0)
        {
            ptree root_datto_info;
            try {
                LOG_TRACE("read dattodb_info_filename = %s", dattodb_info_filename.c_str());
                boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
                p = p / ".datto_info_temp";
                string cmd = "cp " + dattodb_info_filename + " " + p.string();
                system_tools::execute_command(cmd.c_str());
                /*if (!system_tools::copy_file(dattodb_info_filename.c_str(), p.string().c_str()))
                return -1;*/
                read_json(p.string(), root_datto_info);
                remove(p.string().c_str());
            }
            catch (const std::exception &e)
            {
                LOG_ERROR("can't find the datto info file, you should check the datto is installed or not.")
            }
            if (root_datto_info.empty())
            {
                datto_version = string("0.10.6");
            }
            datto_version = root_datto_info.get<string>("version");
        }
        ptree snapshots;
        snapshots = boost_json_get_child_helper(root,"snapshots", snapshots);
        LOG_TRACE("snapshots.size() = %d\r\n", snapshots.size());
        if (!snapshots.empty())
        {
            LOG_TRACE("snapshots enter!\r\n");
            /*clear the all snapshot*/
            snapshot_map.clear();
            snapshot_map_by_uri.clear();
            snapshots_map_by_parttions.clear();
            snapshot_map_by_disks.clear();

            for (auto & m : snapshots)
            {
                auto &sn = m.second;
                int minor = sn.get<int>("minor");
                dattobd::dattobd_info di;
                if (snapshot_map.count(minor))
                {
                    LOG_ERROR("minor = %d already found in snapshot_map!\r\n", minor);
                    continue;
                }
                mount_point::ptr mp = disk_baseA::get_mount_point(boost_json_get_value_helper<string>(sn, "block_device_path", ""));
                //if (mp)
                //{
                    snapshot_instance::ptr si = snapshot_instance::ptr(new snapshot_instance(boost_json_get_value_helper<string>(sn, "disk_path", ""),
                        boost_json_get_value_helper<string>(sn, "block_device_path", ""),
                        mp?mp:NULL,
                        boost_json_get_value_helper<string>(sn, "string_uri", ""),
                        boost_json_get_value_helper<string>(sn, "datto_device", ""),
                        boost_json_get_value_helper<string>(sn, "abs_cow_path", ""),
                        boost_json_get_value_helper<string>(sn, "previous_cow_path", ""),
                        boost_json_get_value_helper<string>(sn, "creation_time", ""),
                        boost_json_get_value_helper<sector_t>(sn, "total_chunks", 0),
                        boost_json_get_value_helper<sector_t>(sn, "total_blocks", 0),
                        minor,
                        boost_json_get_value_helper<uint64_t>(sn, "device_block_offset", 0),
                        boost_json_get_value_helper<uint64_t>(sn, "device_block_size", 0),
                        boost_json_get_value_helper<string>(sn, "uuid", ""),
                        datto_version,
                        boost_json_get_value_helper<int>(sn, "index", 0),
                        boost_json_get_value_helper<uint64_t>(sn, "fallocated_space", 0),
                        boost_json_get_value_helper<uint64_t>(sn, "cache_size", 0)));
                    si->get_info();
                    if (boost_json_get_value_helper<bool>(sn, "is_lvm", 0))
                        si->set_lvm();
                    //si->set_src();
                    si->set_set_uuid(boost_json_get_value_helper<string>(sn, "set_id", ""));
                    si->set_state(boost_json_get_value_helper<unsigned long>(sn, "state", 0));
                    int cow_ratio = boost_json_get_value_helper<int>(sn, "cow_ratio", 10);
                    si->set_cow_ratio(cow_ratio);
                    /*check the cow's size is correct or not*/
                    snapshot_map[minor] = si;
                //}
            }
        }
        ptree btrfs_snapshots;
        btrfs_snapshots = boost_json_get_child_helper(root,"btrfs_snapshots", btrfs_snapshots);
        LOG_TRACE("btrfs_snapshots.size() = %d\r\n", btrfs_snapshots.size());
        if (!btrfs_snapshots.empty())
        {
            btrfs_snapshot_vtr.clear();
            for (auto & m : btrfs_snapshots)
            {
                auto &sn = m.second;
                mount_point::vtr mps;
                string disk_path = boost_json_get_value_helper<string>(sn, "disk_path", "");
                if(!disk_path.empty())
                     mps = disk_baseA::get_mount_points(disk_path);
                if (!mps.empty())
                {
                    btrfs_snapshot_instance::ptr si = btrfs_snapshot_instance::ptr(new btrfs_snapshot_instance(disk_path, mps));
                    si->set_snapshot_path(boost_json_get_value_helper<string>(sn, "snapshot_path", ""));
                    si->set_uuid(boost_json_get_value_helper<string>(sn, "uuid", ""));
                    si->set_set_uuid(boost_json_get_value_helper<string>(sn, "set_uuid", ""));
                    btrfs_snapshot_vtr.push_back(si);
                }
            }
        }
    }
}

int snapshot_manager::read_snapshot_config_file()
{
    FUNC_TRACER;
    only_read_snapshot_config_file();

    snapshot_instance::int_map::iterator si;
    for (si = snapshot_map.begin(); si!= snapshot_map.end(); )
    {
        if (si->second->get_mounted_point() == NULL)
        {
            si->second->clear_all_cow();
            si = snapshot_map.erase(si);
            continue;
        }
        int kk = snapshots_cow_space_user_config.count(si->second->get_block_device_path());
        int ret = si->second->verify_info();
        if (((snapshots_cow_space_user_config.count(si->second->get_block_device_path())==0) || (snapshots_cow_space_user_config.count(si->second->get_block_device_path()) != 0 &&
            snapshots_cow_space_user_config[si->second->get_block_device_path()] == si->second->get_cow_ratio() ))&&
            !ret)
            ++si;
        else
        {
            if (ret == 1 || ((snapshots_cow_space_user_config.count(si->second->get_block_device_path())!=0) && (snapshots_cow_space_user_config[si->second->get_block_device_path()] != si->second->get_cow_ratio())))
                si->second->destroy();
            si->second->clear_all_cow();
            si = snapshot_map.erase(si);
            continue;
        }
    }
    /*read the dattobd_info*/
    ptree root_datto_info;
    try {
        LOG_TRACE("read dattodb_info_filename = %s", dattodb_info_filename.c_str());
        boost::filesystem::path p = boost::filesystem::path(system_tools::get_execution_file_path_linux()).parent_path();
        p = p / ".datto_info_temp";
        string cmd = "cp " + dattodb_info_filename + " " + p.string();
        system_tools::execute_command(cmd.c_str());
        /*if (!system_tools::copy_file(dattodb_info_filename.c_str(), p.string().c_str()))
            return -1;*/
        read_json(p.string(), root_datto_info);
        remove(p.string().c_str());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("can't find the datto info file, you should check the datto is installed or not.")
    }
    if (root_datto_info.empty())
    {
        LOG_ERROR("can't find the datto info file, you should check the datto is installed or not.")
        return -1;
    }
    datto_version = root_datto_info.get<string>("version");
    LOG_TRACE("datto_version = %s", datto_version.c_str());
    ptree devices = root_datto_info.get_child("devices");
    LOG_TRACE("there are %d devices are created by datto\r\n", devices.size());
    for (auto & m : devices)
    {
        auto & dv = m.second;
        dattobd::dattobd_info di;
        dattobd::dattobd_get_info(dv.get<unsigned int>("minor"), &di,datto_version);
        dattobd_infos.push_back(di);
        LOG_TRACE("minor = %d ,cow = %s ,datto device is created.", di.minor, di.cow);
    }
    /*remove the datto device if there are not be record by linux_packer*/

    for (auto & di : dattobd_infos)
    {
        LOG_TRACE("di.bdev = %s\r\n", di.bdev);
        bool b_found = false;
        bool b_error_fit = false;
        int error_minor = -1;
        for (auto & sn : snapshot_map)
        {
            LOG_TRACE("sn.second.get_disk_path() = %s\r\n", sn.second->get_disk_path().c_str());

            if (sn.second->get_block_device_path() == string(di.bdev))
            {
                LOG_TRACE("sn.first = %d, di.minor = %u, di.error = %d", sn.first, di.minor, di.error);
                if (sn.first == di.minor && di.error == 0)
                    b_found = true;
                else
                    error_minor = sn.first;
                break;
            }
        }
        if (!b_found)
        {
            dattobd::dattobd_destroy(di.minor);

            sleep(1);
            LOG_TRACE("minor = %d , datto device is removed.", di.minor);
            if (error_minor != -1)
                snapshot_map.erase(error_minor);
        }
    }
    enumerate_snapshots_by_uri(snapshot_map_by_uri);
    enumerate_snapshots_by_parttions(snapshots_map_by_parttions);
    enumerate_snapshots_by_disks(snapshot_map_by_disks);
    return 0;
}

bool snapshot_manager::enumerate_snapshots_by_uri(snapshot_instance::disk_sn_map & map)
{
    FUNC_TRACER;
    LOG_TRACE("snapshot_map.size() = %d\r\n", snapshot_map.size());
    for (auto & sn : snapshot_map)
    {
        LOG_TRACE("sn.second->get_string_uri() = %s\r\n", sn.second->get_string_uri().c_str());
        map[sn.second->get_string_uri()].push_back(sn.second);
    }
    return true;
}
bool snapshot_manager::enumerate_snapshots_by_parttions(snapshot_instance::map & map)
{
    FUNC_TRACER;
    for (auto & sn : snapshot_map)
        map[sn.second->get_block_device_path()] = sn.second;
    return true;
}
bool snapshot_manager::enumerate_snapshots_by_disks(snapshot_instance::disk_sn_map & map)
{
    FUNC_TRACER;
    for (auto & sn : snapshot_map)
        map[sn.second->get_disk_path()].push_back(sn.second);
    for (auto & sns : map)
    {
        for (auto & sn : sns.second)
        {
            LOG_TRACE("sn.second.datto_name = %s\r\n", sn->get_datto_device().c_str());
        }
    }

    return true;
}

snapshot_instance::vtr snapshot_manager::enumerate_snapshot_by_disk_ab_path(const std::string & disk_ab_path)
{
    FUNC_TRACER;
    LOG_TRACE("snapshot_map_by_disks.size() = %d", snapshot_map_by_disks.size());
    for (auto & sn : snapshot_map_by_disks[disk_ab_path])
    {
        LOG_TRACE("disk_ab_path = %s , sn.second.datto_name = %s\r\n", disk_ab_path.c_str(), sn->get_datto_device().c_str());
    }
    return snapshot_map_by_disks[disk_ab_path];
}

snapshot_instance::ptr snapshot_manager::get_snapshot_by_datto_device_name(const std::string & datto_device_name)
{
    FUNC_TRACER;
    for (auto & sn : snapshot_map)
    {
        if (datto_device_name == sn.second->get_datto_device())
            return sn.second;
    }
    return NULL;
}

snapshot_instance::ptr snapshot_manager::get_snapshot_by_lvm_name(const std::string & lvm_name, const std::string & vguuid, const std::string & lvuuid)
{
    FUNC_TRACER;
    storage::ptr str = storage::get_storage();
    for (auto & sn : snapshot_map)
    {
        string snapshot_block_device_path = sn.second->get_block_device_path();

        partitionA::ptr par = boost::static_pointer_cast<partitionA>(str->get_instance_from_ab_path(snapshot_block_device_path));
        LOG_TRACE("par->lvname = %s, lvm_name = %s", par->lvname.c_str(), lvm_name.c_str());
        if (par->lvname == lvm_name)
            return sn.second;
    }
    /*if not found check the uuid*/
    std::string binded_uuid = vguuid + lvuuid;
    std::vector<string> strVec;
    boost::split(strVec, binded_uuid, boost::is_any_of("-"));
    binded_uuid.clear();
    for (auto & s : strVec)
    {
        binded_uuid += s;
    }
    for (auto & sn : snapshot_map)
    {
        string snapshot_block_device_path = sn.second->get_block_device_path();

        partitionA::ptr par = boost::static_pointer_cast<partitionA>(str->get_instance_from_ab_path(snapshot_block_device_path));
        LOG_TRACE("par->uuid = %s, binded_uuid = %s", par->uuid.c_str(), binded_uuid.c_str());
        if (par->uuid == binded_uuid)
            return sn.second;
    }

    return NULL;
}

snapshot_instance::ptr snapshot_manager::get_snapshot_by_offset(const std::string & disk_name, const uint64_t & offset)
{
    FUNC_TRACER;
    snapshot_instance::ptr out = NULL;
    storage::ptr str = storage::get_storage();
    snapshot_instance::vtr disk_snaps = enumerate_snapshot_by_disk_ab_path(disk_name);
    for (auto & dsn : disk_snaps)
    {
        LOG_TRACE("dsn->get_block_device_path = %s\r\n", dsn->get_block_device_path().c_str());
        if (dsn->get_block_device_offset() == offset)
        {
            out = dsn;
            break;
        }
    }
    return out;
}

snapshot_instance::vtr snapshot_manager::change_snapshot_mode_by_uri(std::set<std::string> disks_uri,bool b_inc,int & datto_result, std::set<std::string> & modify_disk_uris)
{
    FUNC_TRACER;
    snapshot_instance::vtr out;
    datto_result = 0;
    for (auto & uri : disks_uri)
    {
        LOG_TRACE("snapshot_map_by_uri's size = %d\r\n", snapshot_map_by_uri.size());
        for (auto & s : snapshot_map_by_uri)
        {
            LOG_TRACE("s.first = %s\r\n", s.first.c_str());
            LOG_TRACE("s.second.size() = %d\r\n", s.second.size());
            LOG_TRACE("uri = %s\r\n", uri.c_str());
            if (disk_universal_identify(s.first) == disk_universal_identify(uri))
            {
                LOG_TRACE("uri = %s\r\n", uri.c_str());
                for (auto & si : s.second)
                {
                    //LOG_TRACE("uri = %s\r\n", uri.c_str());
                    int result = -1;
                    if (!b_inc)
                    {
                        while (si->get_state() == 3)
                        {
                            result = si->transition_incremental();
                            if (!result)
                                modify_disk_uris.insert(uri);
                        }
                        result = si->transition_snapshot();
                    }
                    else
                        result = si->transition_incremental();
                    LOG_TRACE("result = %d", result);
                    if (!result)
                        out.push_back(si);
                    else
                    {
                        datto_result = result;
                        return out;
                    }
                }
                break;
            }
        }
    }
    clear_destroyed_snapshot();
    /*because the snapshot status is modify, so record the snapshot*/
    write_snapshot_config_file();
    return out;
}

int snapshot_instance::verify_info()
{
    FUNC_TRACER;
    if (!get_info())
    {
        LOG_TRACE("string(di.bdev) = %s, block_device_path = %s", string(di.bdev).c_str(), block_device_path.c_str());
        if (string(di.bdev) != block_device_path)
        {
            LOG_ERROR("string(di.bdev) != block_device_path\r\n");
            return 1;
        }
    }
    else
    {
        LOG_ERROR("get_info error\r\n");
        return -1;
    }
    setting_uuid();
    return 0;
}
bool snapshot_manager::snapshot_exist(disk_baseA::ptr block_dev)
{
    FUNC_TRACER;
    bool b_snapshot_already_created = false;
    for (auto & sn : snapshot_map)
    {
        if (block_dev->mp && (sn.second->get_mounted_point()->get_mounted_on() == block_dev->mp->get_mounted_on()))
        {
            b_snapshot_already_created = true;
            //LOG_TRACE("block_dev->block_type = %d", block_dev->block_type);
            if (block_dev->block_type == disk_baseA::block_dev_type::partition ||
                block_dev->block_type == disk_baseA::block_dev_type::lvm)
            {
                partitionA::ptr partition = static_pointer_cast<partitionA>(block_dev);
                disk::ptr dsk = boost::static_pointer_cast<disk>(partition->parent_disk);
                //LOG_TRACE("dsk->string_uri = %s\r\n", dsk->string_uri.c_str());
                sn.second->set_string_uri(dsk->string_uri);
            }
            else
            {
                disk::ptr dsk = boost::static_pointer_cast<disk>(block_dev);
                //LOG_TRACE("dsk->string_uri = %s\r\n", dsk->string_uri.c_str());
                sn.second->set_string_uri(dsk->string_uri);
            }
            break;
        }
    }
    return b_snapshot_already_created;
}

void snapshot_manager::destroy_snapshots(snapshot_instance::vtr sis)
{
    for (auto & sp : snapshot_map)
    {
        auto &si = sp.second;
        for (auto & dsi : sis)
        {
            if(dsi.get() == si.get())
                si->destroy(true);
        }
    }
    clear_destroyed_snapshot();
}


void snapshot_manager::destroy_all_snapshot(bool total_destroyed)
{
    FUNC_TRACER;
    for (auto & sp : snapshot_map)
    {
        auto &si = sp.second;
        si->destroy(total_destroyed);
    }
    if (total_destroyed)
    {
        snapshot_map.clear();
        snapshot_map_by_uri.clear();
        snapshot_map_by_disks.clear();
        snapshots_map_by_parttions.clear();
    }
}

bool snapshot_manager::reset_all_snapshot()
{
    FUNC_TRACER;
    destroy_all_snapshot(false);
    for (auto & sp : snapshot_map)
    {
        if (!sp.second->set_snapshot_incremental(false))
        {
            LOG_ERROR("set_snapshot_incremental false");
            return false;
        }
    }
    return true;
}




void snapshot_manager::clear_destroyed_snapshot()
{
    FUNC_TRACER;
    LOG_TRACE("snapshot_map.size = %d\r\n", snapshot_map.size());
    for (snapshot_instance::int_map::iterator ica = snapshot_map.begin(); ica != snapshot_map.end();)
    {
        if (ica->second->destroyed)
        {
            LOG_TRACE("ica->first = %d is erase\r\n", ica->first);
            ica = snapshot_map.erase(ica);
        }
        else
        {
            ++ica;
        }
    }
    enumerate_snapshots_by_uri(snapshot_map_by_uri);
    enumerate_snapshots_by_parttions(snapshots_map_by_parttions);
    enumerate_snapshots_by_disks(snapshot_map_by_disks);
}

uint8_t btrfs_snapshot_instance::preoper()
{
    FUNC_TRACER;
    //make the root dir to mount, naming rule is /{block_device_path}_root_device
    //        mkdir(path, mode) != 0 && errno != EEXIST)

    if (mkdir(btrfs_rootdir_mount_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 && errno != EEXIST)
    {
        LOG_ERROR("btrfs take_snapshot error: create root dir fail");
        return 1;
    }

    //mount 
    string command = "mount -o subvolid=0 " + disk_path + " " + btrfs_rootdir_mount_path;
    string result = system_tools::execute_command(command.c_str());
    if (!result.empty())
    {
        LOG_ERROR("btrfs take_snapshot error: mount root subvolume fail");
        return 1;
    }
    
    return 0;
}
uint8_t btrfs_snapshot_instance::postoper()
{
    FUNC_TRACER;
    string command = "umount " + btrfs_rootdir_mount_path;
    string result = system_tools::execute_command(command.c_str());

    //final remove the root dir;
    rmdir(btrfs_rootdir_mount_path.c_str());
    if (result.find("ERROR") != std::string::npos)
    {
        LOG_ERROR("btrfs take_snapshot error: %s", result.c_str());
        return 1;
    }
    return 0;
}


uint8_t btrfs_snapshot_instance::take_snapshot()
{
    FUNC_TRACER;
    if (preoper())
        return 1;
    mount_info::filter::ptr flt = mount_info::filter::ptr(new mount_info::filter());
    flt->fs_type = "btrfs";
    mount_info::vtr mis = storage::get_mount_info(flt);
    //remove others that's not btrfs;
    for (auto & mp : mps)
    {
        for (auto & mi : mis)
        {
            if (mp->get_mounted_on() == mi->mount_path)
            {
                vector<string> strVec;
                boost::split(strVec, mi->option, boost::is_any_of(","));
                pair<string, string> vol_snapshot_path_pair;
                string find_target = "subvol=";
                bool found = false;
                for (auto & str : strVec)
                {
                    size_t  search_index = str.find(find_target);
                    if (search_index != string::npos)
                    {
                        vol_snapshot_path_pair.second = str.substr(search_index + find_target.size(), str.size() - find_target.size() - search_index);
                        string vol_path_remove_head_backslash;
                        if (vol_snapshot_path_pair.second[0] == '/')
                            vol_snapshot_path_pair.first = vol_snapshot_path_pair.second.substr(1, vol_snapshot_path_pair.second.size() - 1);
                        for (int i = 0; i < vol_snapshot_path_pair.second.size(); ++i)
                        {
                            if (vol_snapshot_path_pair.second[i] == '/')
                                vol_snapshot_path_pair.second[i] = '_';
                        }
                        LOG_TRACE("vol_snapshot_path_pair.first = %s, vol_snapshot_path_pair.second = %s", vol_snapshot_path_pair.first.c_str(), vol_snapshot_path_pair.second.c_str());
                        vol_snapshot_path_pairs.push_back(vol_snapshot_path_pair);
                        found = true;
                        break;
                    }
                }
                if (found)
                    break;
            }
        }
    }
    //now therer are a vector that have vol path and the "/ is changed to _".
    for (auto & v : vol_snapshot_path_pairs)
    {
        string snapshot_path = btrfs_rootdir_mount_path + v.second;
        string snapshot_vol = btrfs_rootdir_mount_path + v.first;
        LOG_TRACE("snapshot_path = %s, snapshot_vol = %s", snapshot_path.c_str(), snapshot_vol.c_str());
        string command = "btrfs subvolume snapshot " + snapshot_vol + " " + snapshot_path;
        string result = system_tools::execute_command(command.c_str());
        LOG_TRACE("command = %s", command.c_str());
        LOG_TRACE("result = %s", result.c_str());
        if (result.find("ERROR") != std::string::npos)
        {
            LOG_ERROR("btrfs take_snapshot error: %s", result.c_str());
            return 1;
        }
    }
    

    if (postoper())
        return 1;

    //umount
    if (uuid.empty())
        uuid = system_tools::gen_random_uuid();
    return 0;
}

uint8_t btrfs_snapshot_instance::delete_snapshot()
{
    FUNC_TRACER;
    if (preoper())
        return 1;

    for (auto & v : vol_snapshot_path_pairs)
    {
        string snapshot_path = btrfs_rootdir_mount_path + v.second;
        string command = "btrfs subvolume delete " + snapshot_path;
        string result = system_tools::execute_command(command.c_str());
        if (result.find("ERROR") != std::string::npos)
        {
            LOG_ERROR("btrfs delete_snapshot error: %s", result.c_str());
            return 1;
        }
    }
    if (postoper())
        return 1;

    return 0;
}

btrfs_snapshot_instance::ptr snapshot_manager::get_btrfs_snapshot_by_disk_ab_path(const std::string & disk_ab_path)
{
    btrfs_snapshot_instance::ptr out = NULL;
    for (auto & a : btrfs_snapshot_vtr)
    {
        if (a->get_disk_path() == disk_ab_path)
        {
            out = a;
            break;
        }
    }
    return out;
}

btrfs_snapshot_instance::ptr snapshot_manager::get_btrfs_snapshot_by_offset(const std::string & disk_ab_path, const uint64_t & offset)
{
    storage::ptr str = storage::get_storage();
    for (auto & sn : btrfs_snapshot_vtr)
    {
        string snapshot_block_device_path = sn->get_disk_path();

        partitionA::ptr par = boost::static_pointer_cast<partitionA>(str->get_instance_from_ab_path(snapshot_block_device_path));
        LOG_TRACE("par->partition_start_offset = %u , offset = %u", par->partition_start_offset, offset);
        if (par->partition_start_offset == offset)
            return sn;
    }
    return NULL;
}
btrfs_snapshot_instance::ptr snapshot_manager::get_btrfs_snapshot_by_lvm_name(const std::string & lvm_name, const std::string & vguuid, const std::string & lvuuid)
{
    FUNC_TRACER;
    storage::ptr str = storage::get_storage();
    for (auto & sn : btrfs_snapshot_vtr)
    {
        string snapshot_block_device_path = sn->get_disk_path();

        partitionA::ptr par = boost::static_pointer_cast<partitionA>(str->get_instance_from_ab_path(snapshot_block_device_path));
        LOG_TRACE("par->lvname = %s, lvm_name = %s", par->lvname.c_str(), lvm_name.c_str());
        if (par->lvname == lvm_name)
            return sn;
    }
    /*if not found check the uuid*/
    std::string binded_uuid = vguuid + lvuuid;
    std::vector<string> strVec;
    boost::split(strVec, binded_uuid, boost::is_any_of("-"));
    binded_uuid.clear();
    for (auto & s : strVec)
    {
        binded_uuid += s;
    }
    for (auto & sn : btrfs_snapshot_vtr)
    {
        string snapshot_block_device_path = sn->get_disk_path();

        partitionA::ptr par = boost::static_pointer_cast<partitionA>(str->get_instance_from_ab_path(snapshot_block_device_path));
        LOG_TRACE("par->uuid = %s, binded_uuid = %s", par->uuid.c_str(), binded_uuid.c_str());
        if (par->uuid == binded_uuid)
            return sn;
    }
    return NULL;
}



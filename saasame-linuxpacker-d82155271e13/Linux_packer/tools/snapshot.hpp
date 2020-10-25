#pragma once 
#ifndef SNAPSHOT_H
#define SNAPSHOT_H
/*need rename*/
#ifndef __KERNEL__
#include <stdint.h>
#endif


#include <linux/ioctl.h>
#include <linux/limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>


#define DATTOBD_VERSION "0.9.16"
#define IOCTL_MAGIC 0x91

/*for json*/
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
/*for ptime*/
#include "boost/date_time/posix_time/posix_time.hpp"

/*for map*/
#include <map>
/*for string*/
#include <string>
#include <vector>

#include "log.hpp"
#include "universal_disk_rw.hpp"
#include "storage.hpp"


using namespace boost::property_tree;
using namespace std;
using namespace linux_storage;
typedef unsigned long long sector_t;

#define INDEX_BUFFER_SIZE 8192
#define COW_UUID_SIZE 16
#define COW_BLOCK_LOG_SIZE 12
#define COW_BLOCK_SIZE (1 << COW_BLOCK_LOG_SIZE)
#define MAX_RW_BLOCK_SIZE COW_BLOCK_SIZE
#define COW_HEADER_SIZE 4096
#define COW_MAGIC ((uint32_t)4776)
#define COW_CLEAN 0
#define COW_INDEX_ONLY 1
#define COW_VMALLOC_UPPER 2
#define IOCTL_SETUP_SNAP _IOW(IOCTL_MAGIC, 1, dattobd::setup_params) //in: see above
#define IOCTL_RELOAD_SNAP _IOW(IOCTL_MAGIC, 2, dattobd::reload_params) //in: see above
#define IOCTL_RELOAD_INC _IOW(IOCTL_MAGIC, 3, dattobd::reload_params) //in: see above
#define IOCTL_DESTROY _IOW(IOCTL_MAGIC, 4, unsigned int) //in: minor
#define IOCTL_TRANSITION_INC _IOW(IOCTL_MAGIC, 5, unsigned int) //in: minor
#define IOCTL_TRANSITION_SNAP _IOW(IOCTL_MAGIC, 6, dattobd::transition_snap_params) //in: see above
#define IOCTL_RECONFIGURE _IOW(IOCTL_MAGIC, 7, dattobd::reconfigure_params) //in: see above
#define IOCTL_DATTOBD_INFO _IOR(IOCTL_MAGIC, 8, dattobd::dattobd_info) //in: see above

#define SNAPSHOT_NAME "/.snapshot"

class dattobd
{
public:
    struct setup_params {
        char *bdev; //name of block device to snapshot
        char *cow; //name of cow file for snapshot
        unsigned long fallocated_space; //space allocated to the cow file (in megabytes)
        unsigned long cache_size; //maximum cache size (in bytes)
        unsigned int minor; //requested minor number of the device
    };

    struct reload_params {
        char *bdev; //name of block device to snapshot
        char *cow; //name of cow file for snapshot
        unsigned long cache_size; //maximum cache size (in bytes)
        unsigned int minor; //requested minor number of the device
    };

    struct transition_snap_params {
        char *cow; //name of cow file for snapshot
        unsigned long fallocated_space; //space allocated to the cow file (in bytes)
        unsigned int minor; //requested minor
    };

    struct reconfigure_params {
        unsigned long cache_size; //maximum cache size (in bytes)
        unsigned int minor; //requested minor number of the device
    };
    struct cow_header {
        uint32_t magic; //COW header magic
        uint32_t flags; //COW file flags
        uint64_t fpos; //current file offset
        uint64_t fsize; //file size
        uint64_t seqid; //seqential id of snapshot (starts at 1)
        uint8_t uuid[COW_UUID_SIZE]; //uuid for this series of snapshots
    };

    struct dattobd_info {
        unsigned int minor;
        unsigned long state;
        int error;
        unsigned long cache_size;
        unsigned long long falloc_size;
        unsigned long long seqid;
        char uuid[COW_UUID_SIZE];
        char cow[PATH_MAX];
        char bdev[PATH_MAX];
    };

    static int dattobd_setup_snapshot(unsigned int minor, const char *bdev, const char *cow, unsigned long fallocated_space, unsigned long cache_size)
    {
        int fd, ret;
        setup_params sp;
        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        sp.minor = minor;
        sp.bdev = strdup(bdev);
        sp.cow = strdup(cow);
        sp.fallocated_space = fallocated_space;
        sp.cache_size = cache_size;

        ret = ioctl(fd, IOCTL_SETUP_SNAP, &sp);

        close(fd);
        return ret;
    }
    static int dattobd_reload_snapshot(unsigned int minor, const char *bdev, const char *cow, unsigned long cache_size)
    {
        int fd, ret;
        reload_params rp;

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        rp.minor = minor;
        rp.bdev = strdup(bdev);
        rp.cow = strdup(cow);
        rp.cache_size = cache_size;

        ret = ioctl(fd, IOCTL_RELOAD_SNAP, &rp);

        close(fd);
        return ret;
    }
    static int dattobd_reload_incremental(unsigned int minor, const char *bdev, const char *cow, unsigned long cache_size)
    {
        int fd, ret;
        reload_params rp;

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        rp.minor = minor;
        rp.bdev = strdup(bdev);
        rp.cow = strdup(cow);
        rp.cache_size = cache_size;

        ret = ioctl(fd, IOCTL_RELOAD_INC, &rp);

        close(fd);
        return ret;
    }
    static int dattobd_destroy(unsigned int minor)
    {
        int fd, ret;

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        ret = ioctl(fd, IOCTL_DESTROY, &minor);

        close(fd);
        return ret;
    }
    static int dattobd_transition_incremental(unsigned int minor)
    {
        int fd, ret;

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        ret = ioctl(fd, IOCTL_TRANSITION_INC, &minor);

        close(fd);
        return ret;
    }
    static int dattobd_transition_snapshot(unsigned int minor, const char *cow, unsigned long fallocated_space)
    {
        int fd, ret;
        transition_snap_params tp;

        tp.minor = minor;
        tp.cow = strdup(cow);
        tp.fallocated_space = fallocated_space;

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        ret = ioctl(fd, IOCTL_TRANSITION_SNAP, &tp);

        close(fd);
        return ret;
    }
    static int dattobd_reconfigure(unsigned int minor, unsigned long cache_size)
    {
        int fd, ret;
        reconfigure_params rp;

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        rp.minor = minor;
        rp.cache_size = cache_size;

        ret = ioctl(fd, IOCTL_RECONFIGURE, &rp);

        close(fd);
        return ret;
    }
    static int dattobd_get_info(unsigned int minor, struct dattobd_info *info)
    {
        int fd, ret;

        if (!info) {
            errno = EINVAL;
            return -1;
        }

        fd = open("/dev/datto-ctl", O_RDONLY);
        if (fd < 0) return -1;

        info->minor = minor;

        ret = ioctl(fd, IOCTL_DATTOBD_INFO, info);

        close(fd);
        return ret;
    }
};
#define ERROR_HANDLE(fmt, ...)      \
do{                                 \
    ret = errno;                    \
    errno = 0;                      \
    LOG_ERROR(fmt, ##__VA_ARGS__);  \
    goto error;                     \
}while(0)

class changed_area
{
public:
    typedef boost::shared_ptr<changed_area> ptr;
    typedef std::vector<ptr> vtr;
    std::pair<uint64_t, uint64_t> value;
};

class snapshot_instance {
public:
    typedef boost::shared_ptr<snapshot_instance> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    typedef std::map<int, ptr> int_map;
    snapshot_instance() {}
    snapshot_instance(string dp, string bdp, string mp,string uri ,int minor, unsigned long fallocated_space = 0, unsigned long cache_size = 0) :
        disk_path(dp), block_device_path(bdp), mounted_path(mp), index(0), _minor(minor), _fallocated_space(fallocated_space),string_uri(uri),
        _cache_size(cache_size), abs_cow_path(mp + SNAPSHOT_NAME + to_string(0)),src(NULL), dst(NULL) {}
    ~snapshot_instance() {}



    /*here is function*/


    int setup_snapshot() { 
        int ret = dattobd::dattobd_setup_snapshot(_minor, block_device_path.c_str(), abs_cow_path.c_str(), _fallocated_space, _cache_size);
        if (ret == 0)
        {
            ret = get_info();
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
                /*src = general_io_rw::open_rw(datto_device);
                if (!src) {
                    LOG_ERROR("error opening snapshot_rw\n");
                    return -1;
                }*/
            }
        }
        return ret;
    }

    int reload_snapshot() { 
        int ret = dattobd::dattobd_setup_snapshot(_minor, block_device_path.c_str(), abs_cow_path.c_str(), _fallocated_space, _cache_size);
        get_info();
        return ret;
    }
    int reload_incremental() { 
        int ret = dattobd::dattobd_reload_incremental(_minor, block_device_path.c_str(), abs_cow_path.c_str(), _cache_size);
        get_info();
        return ret; }
    int transition_snapshot(unsigned long cache_size = 0) {
        _cache_size = cache_size;
        previous_cow_path = abs_cow_path;
        abs_cow_path = mounted_path + SNAPSHOT_NAME + to_string(++index);
        int ret = dattobd::dattobd_transition_snapshot(_minor, abs_cow_path.c_str(), _cache_size);
        get_info();
        return ret;
    }
    int transition_incremental() { int ret = dattobd::dattobd_transition_incremental(_minor);
        get_info();
        return ret;}
    int destroy() { return dattobd::dattobd_destroy(_minor); }
    int reconfigure(unsigned long cache_size = 0) {
        _cache_size = cache_size;
        dattobd::dattobd_reconfigure(_minor, _cache_size); }
    int get_info() { 
        int ret = dattobd::dattobd_get_info(_minor, &di);
        if (ret)
            return ret;
        _fallocated_space = di.falloc_size;
        _cache_size = di.cache_size;
        return ret;
    }
    int copy(bool b_full)
    {
        int ret;
        FILE *cow = NULL;
        int snap_fd = 0; //file description
        size_t snap_size, bytes, blocks_to_read;
        sector_t i, j, blocks_done = 0, count = 0, err_count = 0;
        uint64_t *mappings = NULL;
        string tempstring, abs_cow_path;
        string cow_file_path = previous_cow_path;
        std::string dst = "/image" + to_string(get_minor());
        if (set_dst(dst))
        {
            ERROR_HANDLE("set_dst : %s failed\r\n", dst.c_str());
        }
        if (set_src())
        {
            ERROR_HANDLE("set_src failed\r\n");
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

                ret = copy_block((INDEX_BUFFER_SIZE * i) + j); // NULL is just test
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
        rm_dst();
        rm_src();
        return ret;
    }

    int copy_by_changed_area(bool b_full)
    {
        int ret;
        FILE *cow = NULL;
        int snap_fd = 0; //file description
        size_t snap_size, bytes, blocks_to_read;
        sector_t i, j, blocks_done = 0, count = 0, err_count = 0;
        string tempstring, abs_cow_path;
        string cow_file_path = previous_cow_path;
        changed_area::vtr cas;
        std::string dst = "/image" + to_string(get_minor());
        if (set_dst(dst))
        {
            ERROR_HANDLE("set_dst : %s failed\r\n", dst.c_str());
        }
        if (set_src())
        {
            ERROR_HANDLE("set_src failed\r\n");
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
            ret = copy_block_by_changed_area(ca->value.first<<12, ca->value.second<<12); // NULL is just test
            if (ret) err_count++;
            count++;
        }
        //print number of blocks changed
        LOG_TRACE("copying complete: %llu blocks changed, %llu errors\n", count, err_count);
    error:
        if (cow) fclose(cow);
        rm_dst();
        rm_src();
        return ret;
    }

    changed_area::vtr get_changed_area(bool b_full)
    {
        changed_area::vtr out_changed_area;
        int i,j;
        size_t blocks_to_read , blocks_done , bytes;
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
        changed_area::ptr cap;
        if (b_full)
        {
            tempca.value.first = 0;
            tempca.value.second = total_blocks;
            cap = changed_area::ptr(new changed_area(tempca));
            out_changed_area.push_back(cap);
        }
        else
        {
            for (i = 0; i < total_chunks; i++) {
                //read a chunk of mappings from the cow file
                blocks_to_read = MIN(INDEX_BUFFER_SIZE, total_blocks - blocks_done);

                bytes = pread(fileno(cow), mappings, blocks_to_read * sizeof(uint64_t), COW_HEADER_SIZE + (INDEX_BUFFER_SIZE * sizeof(uint64_t) * i));
                if (bytes != blocks_to_read * sizeof(uint64_t)) {
                    LOG_ERROR("error reading mappings into memory\n");
                }
                block_count = INDEX_BUFFER_SIZE * i;
                bool con = false;
                //copy blocks where the mapping is set
                for (j = 0; j < blocks_to_read; j++) {
                    if (!mappings[j])
                    {
                        if (con)
                        {
                            con = false;
                            cap = changed_area::ptr(new changed_area(tempca));
                            out_changed_area.push_back(cap);
                        }
                        continue;
                    }
                    if (!con)
                    {
                        tempca.value.first = block_count + j;
                        tempca.value.second = 1;
                    }
                    else
                    {
                        ++tempca.value.second;
                    }
                    con = true;
                }
                blocks_done += blocks_to_read;
            }
        }
        if (mappings) free(mappings);
        if (cow) fclose(cow);

        return out_changed_area;
    }
    
    sector_t got_block_size()
    {
        sector_t total_blocks;
        size_t snap_size;
        FILE *snap = NULL;
        string tempstring = "/dev/datto" + to_string(di.minor);
        snap = fopen(tempstring.c_str(), "r");
        if (!snap) {
            return 0;
        }
        fseeko(snap, 0, SEEK_END);
        snap_size = ftello(snap);
        LOG_TRACE("snap_size = %d\r\n", snap_size);
        total_blocks = (snap_size + COW_BLOCK_SIZE - 1) / COW_BLOCK_SIZE;
        fclose(snap);
        return total_blocks;
    }

    int copy_block_by_changed_area(uint64_t start, uint64_t lenght) //maybe change to byte is better
    {
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

    int copy_block(sector_t block)
    {
        char buf[COW_BLOCK_SIZE];
        int ret;
        uint32_t bytes;

        if (get_block(buf, COW_BLOCK_SIZE, block)) {
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

    int get_block(char * buf, int size, sector_t block)
    {
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

    int verify_files(FILE *cow)
    {
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
private:
	int index;
    int _minor;
    unsigned long _fallocated_space;
    unsigned long _cache_size;
	string block_device_path;
    string disk_path;
	string abs_cow_path;
    string previous_cow_path;
    string string_uri;
    string datto_device;
    boost::posix_time::ptime _creation_time;
    string mounted_path;
    dattobd::dattobd_info di;
    sector_t total_chunks;
    sector_t total_blocks;
    universal_disk_rw::ptr src;
    universal_disk_rw::ptr dst;
    int set_src() {
        src = general_io_rw::open_rw(datto_device);
        if (!src) {
            LOG_ERROR("error opening snapshot_rw\n");
            return -1;
        }
        return 0;
    }
    void rm_src() {
        src.reset();
        src = NULL;
    }
public:
    int get_index() { return index; }
    void set_index(int i) { index = i; }
    int get_minor() { return _minor; }
    unsigned long get_fallocated_space() { return _fallocated_space; }
    unsigned long get_cache_size() { return _cache_size; }
    string get_string_uri() { return string_uri; }
    string get_datto_device() { return datto_device; }
    string get_block_device_path() { return block_device_path; }
    string get_abs_cow_path() { return abs_cow_path; }
    string get_disk_path() { return disk_path; }
    string get_previous_cow_path() { return previous_cow_path; }
    boost::posix_time::ptime get_creation_time() { return _creation_time; }
    void set_creation_time(const boost::posix_time::ptime time) { _creation_time = time; }
    string get_mounted_path() { return mounted_path; }
    dattobd::dattobd_info get_dattobd_info() { return di; }
    int set_dst(std::string out) {
        dst = general_io_rw::open_rw(out, false);
        if (!dst) {
            LOG_ERROR("error opening output_rw\n");
            return -1;
        }
        return 0;
    }
    void rm_dst() {
        dst.reset();
        dst = NULL;
    }

};


class snapshot_manager
{
public:
    snapshot_manager() :config_filename(".snapshot_config") { read_snapshot_config_file(); }
    snapshot_instance::int_map snapshot_map;
    snapshot_instance::map snapshot_map_by_uri;

    //typedef vector<std::pair<int, int>> change_area;
    //typedef boost::shared_ptr<change_area> change_area_ptr;

    /*change_area_ptr get_change_area(std::string partition,storage::ptr str = storage::ptr())
    {

        change_area_ptr cap = change_area_ptr();

    }*/

    snapshot_instance::ptr take_snapshot_partition(partitionA::ptr partition)
    {
        snapshot_instance::ptr si = snapshot_instance::ptr();
        if (!partition->mounted_path.empty())
        {
            disk::ptr dc = boost::static_pointer_cast<disk>(partition->parent_disk);
            dattobd::dattobd_info di;
            int i;
            for (i = 0;; ++i)
            {
                if (!snapshot_map.count(i) && dattobd::dattobd_get_info(i, &di))
                    break;
            }
            si = snapshot_instance::ptr(new snapshot_instance(dc->ab_path, partition->ab_path, partition->mounted_path, dc->string_uri, i));
            int ret = si->setup_snapshot();
            if (ret != 0)
            {
                LOG_ERROR("ret = %d , create %s mounted on %s snapshot minor %d failed please see dmesg.", ret, si->get_block_device_path().c_str(), si->get_abs_cow_path().c_str(), i);
                return NULL;
            }
            int ret2 = si->get_info();
            if (ret2 != 0)
            {
                LOG_ERROR("ret = %d , dattobd_info snapshot minor %d failed please see dmesg.", ret2, i);
                return NULL;
            }
            si->set_creation_time(boost::posix_time::microsec_clock::universal_time()); //this info is not stable
            snapshot_map[i] = si;
        }
        return si;
    }

    snapshot_instance::vtr take_snapshot_by_ab_path(std::string device_ab_path, storage::ptr str = storage::ptr())
    {
        snapshot_instance::vtr out;
        if (str == NULL)
        {
            str = storage::get_storage();
        }
        std::set<std::string> device_ab_paths;
        device_ab_paths.insert(device_ab_path);
        std::set<std::string> partition_ab_path = str->enum_partition_ab_path_from_uri(device_ab_paths);
        if (partition_ab_path.empty()) // this is not the disk path
        {
            partition_ab_path.insert(device_ab_path); //insert the device path to the partitions list
        }
        disk_baseA::vtr partitions = str->enum_multi_instances_from_multi_ab_paths(partition_ab_path);
        for (auto & p : partitions)
        {
            partitionA::ptr pc = boost::static_pointer_cast<partitionA>(p);
            snapshot_instance::ptr si = take_snapshot_partition(pc);
            if (si != NULL)
                out.push_back(si);
        }
        return out;
    }


    snapshot_instance::vtr take_snapshot_by_uri(std::set<std::string> disks_uri , storage::ptr str = storage::ptr())
    {
        snapshot_instance::vtr out;
        if (str == NULL)
        {
            str = storage::get_storage();
        }
        std::set<std::string> partition_ab_path = str->enum_partition_ab_path_from_uri(disks_uri);
        disk_baseA::vtr partitions = str->enum_multi_instances_from_multi_ab_paths(partition_ab_path);
        for (auto & p : partitions)
        {
            partitionA::ptr pc = boost::static_pointer_cast<partitionA>(p);
            snapshot_instance::ptr si = take_snapshot_partition(pc);
            if (si != NULL)
                out.push_back(si);
        }
        return out;
    }

    int write_snapshot_config_file()
    {
        ptree root;
        ptree snapshots;
        for (auto & m : snapshot_map)
        {
            ptree psnapshot;
            auto & s = m.second;
            psnapshot.put("minor", m.first);
            psnapshot.put("index", s->get_index());
            psnapshot.put("block_device_path", s->get_block_device_path());
            psnapshot.put("string_uri", s->get_string_uri());
            psnapshot.put("disk_path", s->get_disk_path());
            psnapshot.put("mounted_path", s->get_mounted_path());
            psnapshot.put("abs_cow_path", s->get_abs_cow_path());
            psnapshot.put("creation_time", boost::posix_time::to_simple_string(s->get_creation_time()));
            psnapshot.put("fallocated_space", s->get_fallocated_space());
            psnapshot.put("cache_size", s->get_cache_size());
            snapshots.push_back(std::make_pair("", psnapshot));
        }
        root.add_child("snapshots", snapshots);
        fstream fs;
        fs.open(config_filename, ios::out | ios::trunc);
        write_json(fs, root, false);
        fs.close();
        return 0;
    }
    int read_snapshot_config_file()
    {
        ptree root;
        try {
            read_json(config_filename, root);
        }
        catch (const std::exception &e)
        {
            /*do nothing*/
        }
        if (root.empty())
            return -1;
        ptree snapshots = root.get_child("snapshots");
        if (snapshots.empty())
            return -1;
        for (auto & m : snapshots)
        {
            auto &sn = m.second;
            int minor = sn.get<int>("minor");
            dattobd::dattobd_info di;
            if (dattobd::dattobd_get_info(minor, &di))
            {
                continue;
            }
            snapshot_instance::ptr si = snapshot_instance::ptr(new snapshot_instance(sn.get<string>("disk_path"),
                sn.get<string>("block_device_path"),
                sn.get<string>("mounted_path"),
                sn.get<string>("string_uri"),
                minor,
                sn.get<unsigned long>("fallocated_space"),
                sn.get<unsigned long>("cache_size")));
            snapshot_map[minor] = si;
        }
        snapshot_map_by_uri = enumerate_shapshots_by_uri();
        return 0;
    }
    snapshot_instance::map enumerate_shapshots_by_uri()
    {
        snapshot_instance::map out;
        for (auto & sn : snapshot_map)
            out[sn.second->get_string_uri()] = sn.second;
        return out;
    }

private:
    string config_filename;
};
#endif
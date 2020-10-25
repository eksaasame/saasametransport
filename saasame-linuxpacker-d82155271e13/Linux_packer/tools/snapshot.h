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
#define REMOVE_DATTO_COW 1
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

#include "log.h"
#include "universal_disk_rw.h"
#include "storage.h"
#include "data_type.h"


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
#define IOCTL_DATTOBD_INFO_10_6 _IOR(IOCTL_MAGIC, 8, dattobd::dattobd_info_10_6) //in: see above



#define SNAPSHOT_NAME ".snapshot"
#define MAX_COW_INDEX 10

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

    struct dattobd_info_10_6 {
        unsigned int minor;
        unsigned long state;
        int error;
        unsigned long cache_size;
        unsigned long long falloc_size;
        unsigned long long seqid;
        char uuid[COW_UUID_SIZE];
        char cow[PATH_MAX];
        char bdev[PATH_MAX];
        unsigned long long version;
        unsigned long long nr_changed_blocks;
    };

    static int dattobd_setup_snapshot(unsigned int minor, const char *bdev, const char *cow, unsigned long fallocated_space, unsigned long cache_size);
    static int dattobd_reload_snapshot(unsigned int minor, const char *bdev, const char *cow, unsigned long cache_size);
    static int dattobd_reload_incremental(unsigned int minor, const char *bdev, const char *cow, unsigned long cache_size);
    static int dattobd_destroy(unsigned int minor);
    static int dattobd_transition_incremental(unsigned int minor);
    static int dattobd_transition_snapshot(unsigned int minor, const char *cow, unsigned long fallocated_space);
    static int dattobd_reconfigure(unsigned int minor, unsigned long cache_size);
    static int dattobd_get_info(unsigned int minor, struct dattobd_info *info,string datto_version);

};
#define ERROR_HANDLE(fmt, ...)      \
do{                                 \
    ret = errno;                    \
    errno = 0;                      \
    LOG_ERROR(fmt, ##__VA_ARGS__);  \
    goto error;                     \
}while(0)

class snapshot_instance {
public:
    typedef boost::shared_ptr<snapshot_instance> ptr;
    typedef std::vector<ptr> vtr;
    typedef std::map<std::string, ptr> map;
    typedef std::map<int, ptr> int_map;
    typedef std::map<std::string, vtr> disk_sn_map;
    snapshot_instance() { default_init(); }
    snapshot_instance(string dp, string bdp, mount_point::ptr imp, string uri, string datto, string cow_path, string pre_cow_path, string create_time, sector_t chunks, sector_t blocks, int minor, uint64_t block_offset, uint64_t block_size,string _uuid, string _datto_version,int ind = 0, uint64_t fallocated_space = 0, uint64_t cache_size = 0) :
        disk_path(dp), block_device_path(bdp), mp(imp), index(ind), _minor(minor), _fallocated_space(fallocated_space), string_uri(uri), datto_device(datto), abs_cow_path(cow_path), previous_cow_path(pre_cow_path), _creation_time(boost::posix_time::time_from_string(create_time)),
        total_chunks(chunks), total_blocks(blocks), _cache_size(cache_size),block_device_offset(block_offset),block_device_size(block_size),uuid(_uuid), datto_version(_datto_version), state(0){
        default_init();
    }
    ~snapshot_instance() { /*destroy();*/ }


    /*class changed_area
    {
    public:
        typedef boost::shared_ptr<changed_area> ptr;
        typedef std::vector<ptr> vtr;
        std::pair<uint64_t, uint64_t> value;
    };*/

    /*here is function*/


    int setup_snapshot();
    bool set_snapshot_incremental(bool set_uuid);
    void clear_all_cow();
    int reload();
    int transition_snapshot(unsigned long cache_size = 0);
    int transition_incremental();
    int destroy(bool total_destroyed = true);
    int reconfigure(unsigned long cache_size = 0);
    int get_info(bool retry = false);
    int get_info_and_set_uuid();
    int copy(bool b_full);
    int copy_by_changed_area(bool b_full);
    changed_area::vtr get_changed_area(bool b_full);
    sector_t got_block_size();
    int copy_block_by_changed_area(universal_disk_rw::ptr src, universal_disk_rw::ptr dst, uint64_t start, uint64_t lenght);
    int copy_block(universal_disk_rw::ptr src, universal_disk_rw::ptr dst,sector_t block);
    int get_block(universal_disk_rw::ptr src, char * buf, int size, sector_t block);
    int verify_files(FILE *cow);
    int verify_info();
    void set_datto_version(string input) { datto_version = input; }
    string get_datto_version() { return datto_version; }
    bool destroyed;
private:
    int index;
    int _minor;
    uint64_t _fallocated_space;
    uint64_t _cache_size;
    uint64_t block_device_offset;
    uint64_t block_device_size;
    string block_device_path;
    string disk_path;
    string abs_cow_path;
    string previous_cow_path;
    string string_uri;
    string datto_device;
    string datto_version;
    string uuid;
    string set_uuid;
    boost::posix_time::ptime _creation_time;
    mount_point::ptr mp;
    dattobd::dattobd_info di;
    sector_t total_chunks;
    sector_t total_blocks;
    universal_disk_rw::ptr src;
    universal_disk_rw::ptr dst;
    int cow_ratio;
    unsigned long state;
    bool is_lvm;
    string snapshot_mount_point;
    void default_init() { src = NULL; dst = NULL; destroyed = false; is_lvm = false; }
    /*int set_src() {
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
    }*/
public:
    void set_cow_ratio(int input) { cow_ratio = input; }
    int get_cow_ratio() { return cow_ratio; }
    void set_state(unsigned long input) { state = input; }
    int get_state() { return state; }

    int get_index() { return index; }
    void set_index(int i) { index = i; }
    int get_minor() { return _minor; }
    void set_minor(int i) { _minor = i; }
    uint64_t get_fallocated_space() { return _fallocated_space; }
    void set_fallocated_space(uint64_t i) { _fallocated_space = i; }
    uint64_t get_cache_size() { return _cache_size; }
    void set_cache_size(uint64_t i) { _cache_size = i; }
    string get_string_uri() { return string_uri; }
    void set_string_uri(string i) { string_uri = i; }
    string get_datto_device() { return datto_device; }
    string get_block_device_path() { return block_device_path; }
    void set_block_device_path(string i) { block_device_path = i; }
    string get_abs_cow_path() { return abs_cow_path; }
    void set_abs_cow_path(string i) { abs_cow_path = i; }

    string get_disk_path() { return disk_path; }
    void set_disk_path(string i) { disk_path = i; }
    string get_previous_cow_path() { return previous_cow_path; }
    boost::posix_time::ptime get_creation_time() { return _creation_time; }
    void set_creation_time(const boost::posix_time::ptime time) { _creation_time = time; }
    mount_point::ptr get_mounted_point() { return mp; }
    void set_mount_point(mount_point::ptr input) { mp = input; }
    uint64_t get_block_device_offset() { return block_device_offset; }
    void set_block_device_offset(uint64_t i) { block_device_offset = i; }
    uint64_t get_block_device_size() { return block_device_size; }
    void set_block_device_size(uint64_t i) { block_device_size = i; }

    sector_t get_total_chunks(){return total_chunks;}
    sector_t get_total_blocks() { return total_blocks; }
    dattobd::dattobd_info get_dattobd_info() { return di; }
    void update_dattobd_info() {}
    void set_duuid(char _uuid[16]) { GUID * guid = (GUID *)_uuid; uuid = guid->to_string();}
    void setting_uuid();
    string get_uuid() { return uuid; }
    string get_set_uuid() { return set_uuid; }
    void set_set_uuid(string _uuid) { set_uuid = _uuid; }
    void set_lvm(){ is_lvm = true; }
    bool get_lvm() { return is_lvm; }
    string get_snapshot_mount_point() { return snapshot_mount_point; }
    bool mount_snapshot();
    bool unmount_snapshot();
    universal_disk_rw::ptr get_src_rw()
    {
        return general_io_rw::open_rw(datto_device);
    }

    
    /*int set_dst(std::string out) {
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
    }*/
};

// this class is for that fs datto can't take snapshot, it use full replication and use the snapshot that fs provides
class btrfs_snapshot_instance {
public:
    typedef boost::shared_ptr<btrfs_snapshot_instance> ptr;
    typedef std::vector<ptr> vtr;
    btrfs_snapshot_instance(string dp, mount_point::vtr imps) : disk_path(dp), mps(imps) { 
        string device_name = boost::filesystem::path(disk_path).filename().string();
        btrfs_rootdir_mount_path = "/" + device_name + "_snapshot/";
        LOG_TRACE("btrfs_rootdir_mount_path = %s", btrfs_rootdir_mount_path.c_str());
    }
    uint8_t take_snapshot();
    uint8_t delete_snapshot();
    mount_point::vtr get_mounted_points() { return mps; }
    void set_mounted_points(mount_point::vtr input) { mps = input; }
    string get_disk_path() { return disk_path; }
    string get_snapshot_path() { return btrfs_rootdir_mount_path; }
    void set_snapshot_path(string input) { btrfs_rootdir_mount_path = input; }
    void set_disk_path(string input) { disk_path = input; }
    string get_uuid() { return uuid; }
    void set_uuid(string input) { uuid = input; }
    string get_set_uuid() { return sets_uuid; }
    void set_set_uuid(string input) { sets_uuid = input; }
    vector<pair<string, string>> vol_snapshot_path_pairs;
private:
    uint8_t preoper();
    uint8_t postoper();
    string uuid;
    string sets_uuid;
    string disk_path;
    string btrfs_rootdir_mount_path;
    mount_point::vtr mps;
};



class snapshot_manager
{
public:
    typedef boost::shared_ptr<snapshot_manager> ptr;
    snapshot_manager(std::map<std::string, int> _snapshots_cow_space_user_config) :config_filename(".snapshot_config"), dattodb_info_filename("/proc/datto-info"), snapshots_cow_space_user_config(_snapshots_cow_space_user_config), merge_size(-1) { read_snapshot_config_file(); write_snapshot_config_file(); 
        for (auto & cc : snapshots_cow_space_user_config)
        {
            LOG_TRACE("4 snapshots_cow_space_user_config[%s] = %d", cc.first.c_str(), cc.second);
        }
    }
    snapshot_manager() :config_filename(".snapshot_config"), dattodb_info_filename("/proc/datto-info"), merge_size(-1){ read_snapshot_config_file(); write_snapshot_config_file(); }
    snapshot_manager(bool nomean) :config_filename(".snapshot_config"), dattodb_info_filename("/proc/datto-info"), merge_size(-1) { only_read_snapshot_config_file(); }
    btrfs_snapshot_instance::vtr btrfs_snapshot_vtr;
    snapshot_instance::int_map snapshot_map;
    snapshot_instance::disk_sn_map snapshot_map_by_uri;
    snapshot_instance::disk_sn_map snapshot_map_by_disks;
    snapshot_instance::map snapshots_map_by_parttions;
    std::vector<dattobd::dattobd_info> dattobd_infos;
    //snapshot_instance::disk_sn_map snapshot_map_by_snaphot_id; //this just for the formal snapshot take root
    snapshot_instance::ptr take_snapshot_block_dev(disk_baseA::ptr partition, disk_baseA::ptr parents_disk);
    snapshot_instance::vtr take_snapshot_by_disk_ab_path(std::string device_ab_path, unsigned char * result, storage::ptr str = storage::ptr());
    snapshot_instance::vtr take_snapshot_by_disk_uri(std::set<std::string> disks_uri, unsigned char * result, storage::ptr str = storage::ptr());
    btrfs_snapshot_instance::vtr take_btrfs_snapshot_by_disk_uri(std::set<std::string> disks_uri, storage::ptr str = storage::ptr());
    snapshot_instance::vtr change_snapshot_mode_by_uri(std::set<std::string> disks_uri, bool b_inc, int & datto_result, std::set<std::string> & modify_disks_uri);
    void clear_umount_snapshots(storage::ptr str = storage::ptr());
    int write_snapshot_config_file();
    int read_snapshot_config_file();
    bool enumerate_snapshots_by_uri(snapshot_instance::disk_sn_map & map);
    bool enumerate_snapshots_by_parttions(snapshot_instance::map & map);
    bool enumerate_snapshots_by_disks(snapshot_instance::disk_sn_map & map);
    bool snapshot_exist(disk_baseA::ptr block_dev);
    snapshot_instance::vtr enumerate_snapshot_by_disk_ab_path(const std::string & disk_ab_path);
    snapshot_instance::ptr get_snapshot_by_lvm_name(const std::string & lvm_name, const std::string & vguuid, const std::string & lvuuid);
    snapshot_instance::ptr get_snapshot_by_offset(const std::string & disk_name, const uint64_t & offset);
    btrfs_snapshot_instance::ptr get_btrfs_snapshot_by_disk_ab_path(const std::string & disk_ab_path);
    btrfs_snapshot_instance::ptr get_btrfs_snapshot_by_offset(const std::string & disk_ab_path, const uint64_t & offset);
    btrfs_snapshot_instance::ptr get_btrfs_snapshot_by_lvm_name(const std::string & lvm_name, const std::string & vguuid, const std::string & lvuuid);


    void destroy_all_snapshot(bool total_destroyed = true);
    void destroy_snapshots(snapshot_instance::vtr sis);
    bool reset_all_snapshot();
    void clear_destroyed_snapshot();
    snapshot_instance::ptr get_snapshot_by_datto_device_name(const std::string & datto_device_name);
    void set_snapshots_cow_space_user_config(std::map<std::string, int> input) { snapshots_cow_space_user_config = input; }
    void set_merge_size(uint64_t input) { merge_size = input; }
    uint64_t get_merge_size() { return merge_size; }
    int  only_read_snapshot_config_file();
    
    set<string> get_all_mount_point() 
    { 
        set<string> result;
        for (auto & a : snapshot_map)
        {
            result.insert(a.second->get_mounted_point()->get_mounted_on());
        }
        return result;
    }
private:
    void remove_btrfs_under_a_disk_device(disk_baseA::ptr pdb);
    string datto_version;
    string config_filename;
    string dattodb_info_filename;
    std::map<std::string, int> snapshots_cow_space_user_config;
    uint64_t merge_size;
};
#endif
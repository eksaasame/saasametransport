#pragma once

#ifndef _CLONE_DISK_H_
#define _CLONE_DISK_H_ 1
#include "universal_disk_rw.h"
//#include "irm_converter/irm_disk.h"
//#include "linuxfs_parser/linuxfs_parser.h"
#include "lvm.h"
#include "../tools/snapshot.h"
#include "data_type.h"

//DEFINE_GUID(PARTITION_SHADOW_COPY_GUID, 0xcaddebf1L, 0x4400, 0x4de8, 0xb1, 0x03, 0x12, 0x11, 0x7d, 0xcf, 0x3c, 0xcf); looks like only virtual packer job use it, temp remove

#define MBR_PART_SIZE_SECTORS   1
#define GPT_PART_SIZE_SECTORS   34
#define MINI_BLOCK_SIZE         65536UL
#define MAX_BLOCK_SIZE          8388608UL
#define WAIT_INTERVAL_SECONDS   2
#define SIGNAL_THREAD_READ      1
#define SECTOR_SIZE             512


//struct changed_area{
//    typedef std::vector<changed_area> vtr;
//    typedef std::map<std::string, vtr> map;
//    changed_area() : start(0), length(0){}
//    changed_area(uint64_t _start, uint64_t _length, uint64_t _src_start = 0, universal_disk_rw::ptr rw_ = NULL) : start(_start), length(_length), src_start(_src_start), _rw(rw_){ /*printf("_start = %llu\r\n, _length = %llu\r\n", _start, _length);*/ }
//    struct compare {
//        bool operator() (changed_area const & lhs, changed_area const & rhs) const {
//            return lhs.start < rhs.start;
//        }
//    };
//    uint64_t start;
//    uint64_t length;
//    uint64_t src_start;
//    universal_disk_rw::ptr _rw;
//};

class clone_disk{
public:
    static bool is_bootable_disk(universal_disk_rw::ptr rw);
    static uint64_t get_boundary_of_partitions(universal_disk_rw::ptr rw);
    //static changed_area::vtr get_clone_data_range(universal_disk_rw::ptr rw, bool file_system_filter_enable = true);
    static changed_area::map get_clone_data_range(universal_disk_rw::vtr rws, storage::ptr str, snapshot_manager::ptr sh, disk::map modify_disks, set<string> & excluded_paths, set<string> & resync_paths,bool file_system_filter_enable = true,bool b_full = true);
    static bool modify_change_area_with_datto_snapshot(changed_area::map & cam, snapshot_manager::ptr sh ,disk::map modify_disks, bool b_full);

private:
    static changed_area::vtr final_changed_area(changed_area::vtr& src, changed_area::vtr& excludes, uint64_t max_size, uint64_t between_size);
    static inline int set_umap_bit(unsigned char* buffer, ULONGLONG bit) {
        return buffer[bit >> 3] |= (1 << (bit & 7));
    }

    static inline int	test_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] & (1 << (bit & 7));
    }

    static inline int clear_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] &= ~(1 << (bit & 7));
    }

    static inline void merge_umap(unsigned char* buffer, unsigned char* source, ULONG size_in_bytes){
        for (ULONG index = 0; index < size_in_bytes; index++)
            buffer[index] |= source[index];
    }

    static inline int clear_umap(unsigned char* buffer, ULONGLONG start, ULONGLONG length, ULONG resolution){
        int ret = 0;
        for (ULONGLONG curbit = (start / resolution); curbit <= (start + length - 1) / resolution; curbit++){
            ret = clear_umap_bit(buffer, curbit);
        }
        return ret;
    }

    static inline int set_umap(unsigned char* buffer, ULONGLONG start, ULONGLONG length, ULONG resolution){
        int ret = 0;
        for (ULONGLONG curbit = (start / resolution); curbit <= ((start + length - 1) / resolution); curbit++){
            if (0 == test_umap_bit(buffer, curbit)){
                ret = set_umap_bit(buffer, curbit);
            }
        }
        return ret;
    }
};
#endif
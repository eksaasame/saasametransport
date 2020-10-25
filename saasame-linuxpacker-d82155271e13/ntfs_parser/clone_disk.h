#pragma once

#ifndef _CLONE_DISK_H_
#define _CLONE_DISK_H_ 1
#include "macho.h"
#include "..\gen-cpp\universal_disk_rw.h"

struct changed_area{
    typedef std::vector<changed_area> vtr;
    changed_area() : start(0), length(0){}
    changed_area(uint64_t _start, uint64_t _length) : start(_start), length(_length){}
    struct compare {
        bool operator() (changed_area const & lhs, changed_area const & rhs) const {
            return lhs.start < rhs.start;
        }
    };
    uint64_t start;
    uint64_t length;
};

class clone_disk{
public:
    static bool is_bootable_disk(universal_disk_rw::ptr rw);
    static uint64_t get_boundary_of_partitions(universal_disk_rw::ptr rw);
    static changed_area::vtr get_clone_data_range(int disk_number, bool file_system_filter_enable = true);
    static changed_area::vtr get_clone_data_range(universal_disk_rw::ptr rw, bool file_system_filter_enable = true);
private:
    static changed_area::vtr  get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions, const uint64_t& start_offset);
    static changed_area::vtr final_changed_area(changed_area::vtr& src, changed_area::vtr& excludes, uint64_t max_size);
    static inline int set_umap_bit(unsigned char* buffer, ULONGLONG bit){
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
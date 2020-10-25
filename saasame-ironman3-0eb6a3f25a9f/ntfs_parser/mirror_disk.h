#pragma once
#include "macho.h"
#include "clone_disk.h"
#include "..\gen-cpp\universal_disk_rw.h"

#define MINI_BLOCK_SIZE         65536UL
#define MAX_BLOCK_SIZE          8388608UL
#define  PARTITION_SWAP	        0x82

class mirror_disk{
public:
    void turn_on_vhd_preparation(){
        _vhd_preparation = true;
    }
    void turn_off_vhd_preparation(){
        _vhd_preparation = false;
    }
    mirror_disk() : _is_interrupted(false), _block_size(MAX_BLOCK_SIZE), _mini_block_size(MINI_BLOCK_SIZE), _backup_size(0), _vhd_preparation(false){}
    bool replicate_beginning_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output);
    bool replicate_partitions_of_disk(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output);
private:
    changed_area::vtr get_epbrs(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition::vtr& partitions, const uint64_t& start_offset);
    bool            replicate_changed_areas(macho::windows::storage::disk& d, macho::windows::storage::partition& p, std::string path, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output);
    bool            replicate_changed_areas(universal_disk_rw::ptr &rw, macho::windows::storage::disk& d, uint64_t& start_offset, uint64_t offset, uint64_t& backup_size, changed_area::vtr &changeds, universal_disk_rw::ptr& output);
    changed_area::vtr  get_changed_areas(std::string path, uint64_t start);
    inline bool             replicate_changed_areas(macho::windows::storage::disk& d, macho::windows::storage::partition& p, std::wstring path, uint64_t& start_offset, uint64_t& backup_size, universal_disk_rw::ptr& output){
        return replicate_changed_areas(d, p, macho::stringutils::convert_unicode_to_ansi(path), start_offset, backup_size, output);
    }
    changed_area::vtr       get_changed_areas(universal_disk_rw::ptr &disk_rw, macho::windows::storage::disk& d, macho::windows::storage::partition& p, const uint64_t& start_offset);
    changed_area::vtr       get_changed_areas(macho::windows::storage::partition& p, const uint64_t& start_offset);
    macho::windows::storage::volume::ptr                get_volume_info(macho::string_array_w access_paths, macho::windows::storage::volume::vtr volumes);
    changed_area::vtr _get_changed_areas(ULONGLONG file_area_offset, LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits, uint32_t bytes_per_cluster, uint64_t& total_number_of_delta);
    bool exclude_file(LPBYTE buff, boost::filesystem::path file);
    BOOL get_fat_first_sector_offset(HANDLE handle, ULONGLONG& file_area_offset);
    bool replicate(universal_disk_rw::ptr &rw, uint64_t& start, const uint32_t length, universal_disk_rw::ptr& output, const uint64_t target_offset = 0);
    bool _is_interrupted;
    uint64_t _block_size;
    uint64_t _mini_block_size;
    inline int clear_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] &= ~(1 << (bit & 7));
    }

    inline int clear_umap(unsigned char* buffer, ULONGLONG start, ULONG length, ULONG resolution){
        int ret = 0;
        for (ULONGLONG curbit = (start / resolution); curbit <= (start + length - 1) / resolution; curbit++){
            ret = clear_umap_bit(buffer, curbit);
        }
        return ret;
    }

    uint64_t _backup_size;
    bool     _vhd_preparation;
};
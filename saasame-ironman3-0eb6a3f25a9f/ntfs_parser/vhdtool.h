#pragma once

#ifndef _VHD_TOOL_H_
#define _VHD_TOOL_H_ 1

#include "macho.h"
#include "..\gen-cpp\universal_disk_rw.h"
/* All fields Big-Endian */

struct vhd_id
{
    uint32_t f1;
    uint16_t f2;
    uint16_t f3;
    uint8_t  f4[8];
};

/* All fields Big-Endian */
struct vhd_chs
{
    uint16_t c;
    uint8_t  h;
    uint8_t  s;
};

/* All fields Big-Endian */
struct vhd_footer
{
    uint64_t cookie;
    uint32_t features;
    uint32_t file_format_ver;
    uint64_t data_offset;
    uint32_t time_stamp;
    uint32_t creator_app;
    uint32_t creator_ver;
    uint32_t creator_os;
    uint64_t original_size;
    uint64_t current_size;
    struct vhd_chs disk_geometry;
    uint32_t disk_type;
    uint32_t checksum;
    struct vhd_id vhd_id;
    uint8_t saved_state;
    uint8_t reserved[427];
};

/* All fields Big-Endian */
struct vhd_ploc
{
    uint32_t code;
    uint32_t sectors;
    uint32_t length;
    uint32_t reserved;
    uint64_t offset;
};

/* All fields Big-Endian */
struct vhd_dyn
{
    uint64_t cookie;
    uint64_t data_offset;
    uint64_t table_offset;
    uint32_t header_version;
    uint32_t max_tab_entries;
    uint32_t block_size;
    uint32_t checksum;
    struct vhd_id parent;
    uint32_t parent_time_stamp;
    uint32_t reserved0;
    uint8_t parent_utf16[512];
    struct vhd_ploc pe[8];
    uint8_t reserved1[256];
};

typedef uint32_t vhd_batent;

class vhd_tool : public universal_disk_rw{
public:
    struct dirty_range{
        typedef std::vector<dirty_range> vtr;
        dirty_range(ULONGLONG s, ULONGLONG l) : start(s), length(l) {}
        ULONGLONG start;
        ULONGLONG length;
        struct compare {
            bool operator() (dirty_range const & lhs, dirty_range const & rhs) const {
                return lhs.start < rhs.start;
            }
        };
    };

    typedef boost::shared_ptr<vhd_tool> ptr;
    typedef boost::signals2::signal<bool(), macho::_or<bool>> is_cancel;
    typedef boost::signals2::signal<bool(const uint64_t&, const uint32_t&, LPBYTE, const size_t len), macho::_or<bool>> flush_data;
    static std::string get_fixed_vhd_footer(uint64_t disk_size, UUID u = macho::guid_::create());
    static vhd_tool* create(uint64_t disk_size, bool is_fixed = false, UUID u = macho::guid_::create());
    static vhd_tool* open(boost::filesystem::path vhd_file); // For debug purpose...
    static dirty_range::vtr get_dirty_ranges(boost::filesystem::path vhd_file, bool is_rough = true);

    inline void register_flush_data_callback_function(flush_data::slot_type slot){
        _flush_data.connect(slot);
    }

    inline void register_cancel_callback_function(is_cancel::slot_type slot){
        _is_cancel.connect(slot);
    }

    virtual universal_disk_rw::ptr clone(){ return NULL; }
    virtual std::wstring path() const { return L""; }

    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    bool close();
    uint64_t estimate_size();
    void reset();
private:    
    #define ZERO_BLOCK_SZ      0x800
    flush_data          _flush_data;
    is_cancel           _is_cancel;
    inline int set_umap_bit(unsigned char* buffer, ULONGLONG bit){
        return buffer[bit >> 3] |= (1 << (bit & 7));
    }
    bool _write(uint64_t offset, void *buf, size_t size);
    bool _vhd_write(void *buf, size_t size);
    bool _vhd_footer(struct vhd_footer &footer, uint64_t data_offset, UUID u);
    bool _vhd_dyn(struct vhd_dyn &dyn);
    bool _create(bool is_fixed, UUID u);

    int  _vhd_chs(struct vhd_footer &footer);
    
    static bool _is_zero(LPBYTE buf, size_t length);
    static uint32_t _vhd_checksum(uint8_t *data, size_t size);
    static unsigned _min_nz(unsigned a, unsigned b);
    
    vhd_tool(uint64_t size);

    struct vhd_footer*      _footer;
    struct vhd_dyn*         _dyn;
    vhd_batent*             _batents;
    boost::shared_ptr<BYTE> _header;
    boost::shared_ptr<BYTE> _block;

    uint64_t                _size;
    uint64_t                _offset;
    unsigned                _flags;
    uint32_t                _type;
    uint32_t                _block_size;
    uint32_t                _current_block;
    uint64_t                _header_size;
    uint32_t                _block_chunk_size;
    size_t                  _sectors_per_disk;
    size_t                  _sectors_per_block;
    size_t                  _bat_entries;
    static BYTE             _zero_buffer[ZERO_BLOCK_SZ];
};

#endif
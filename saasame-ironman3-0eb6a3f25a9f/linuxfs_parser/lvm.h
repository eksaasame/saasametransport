#pragma once
#ifndef lvm_H
#define lvm_H
#include <macho.h>
#include "..\gen-cpp\universal_disk_rw.h"
#include "linuxfs_parser.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LVM_SIGLEN	8
#define LVM_MAGIC_LEN	8
#define UUID_LEN	32

/* Structure to hold Physical Volumes (PV) label*/
typedef struct _PV_LABEL_HEADER {
    char        pv_name[LVM_SIGLEN];   // Physical volume signature
    uint64_t    pv_sector_xl;          // sector number of this label
    uint32_t    pv_crc;                // CRC value
    uint32_t    pv_offset_xl;          // Offset from start of struct to contents
    char        pv_vermagic[LVM_MAGIC_LEN]; // Physical Volume version "LVM2 001"
    char        pv_uuid[UUID_LEN];
    uint64_t    pv_unknown1[5];             // documentation lacks for lvm
    uint64_t    pv_labeloffset;             // location of the label
}PV_LABEL_HEADER, *PPV_LABEL_HEADER;

typedef struct _PV_LABEL {
    uint32_t        pv_magic;
    char            pv_sig[4];          // signature
    uint64_t        unknown1[2];
    uint64_t        pv_offset_low;
    uint64_t        unknown2;
    uint64_t        pv_offset_high;
    uint64_t        pv_length;
} PV_LABEL, *PPV_LABEL;

#ifdef __cplusplus
}
#endif

struct physical_volume{
    typedef boost::shared_ptr<physical_volume> ptr;
    typedef std::vector<ptr> vtr;
    physical_volume() : dev_size(0ULL), pe_start(0), pe_count(0), offset(0){}
    uint64_t               dev_size;
    uint32_t               pe_start;
    uint32_t               pe_count;
    std::string            id;
    std::string            name;
    universal_disk_rw::ptr rw;
    uint64_t               offset;
};

// Multiple stripes NOT Implemented: we only support linear for now.
struct lv_stripe {
    typedef std::vector<lv_stripe> vtr;
    physical_volume::ptr pv;
    std::string          pv_name;
    uint32_t             start_extent;
    lv_stripe(std::string pv, uint32_t _start_extent) : pv_name(pv), start_extent(_start_extent){}
};

class lv_segment {
public:
    typedef std::vector<lv_segment> vtr;
    uint32_t             start_extent;
    uint32_t             extent_count;
    std::string          type;
    lv_stripe::vtr       stripes;
    struct compare {
        bool operator() (lv_segment const & lhs, lv_segment const & rhs) const {
            return lhs.start_extent < rhs.start_extent;
        }
    };
    lv_segment(uint32_t start, uint32_t count, std::string _type)
    {
        start_extent = start;
        extent_count = count;
        type = _type;
    }
};
class volume_group;
class logical_volume{
private:
    int segment_count;
    volume_group* vg;
public:
    typedef boost::shared_ptr<logical_volume> ptr;
    typedef std::vector<ptr> vtr;
    std::string id;
    std::string volname;
    int extent_size;
    lv_segment::vtr segments;
    uint64_t size(){
        uint64_t s = 0;
        foreach(lv_segment &seg, segments){
            s += seg.extent_count;
        }
        return s * extent_size * SECTOR_SIZE;
    }
    logical_volume(std::string &_id, int nsegs, std::string &vname, int _extent_size, volume_group* _vg){
        segment_count = nsegs;
        id = _id;
        volname = vname;
        extent_size = _extent_size;
        vg = _vg;
    }
    ~logical_volume(){}
};

class volume_group {

private:
    void replace_all(std::string& str, const std::string& from, const std::string& to) {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
        }
    }
public:
    typedef boost::shared_ptr<volume_group> ptr;
    typedef std::vector<ptr> vtr;
    std::string vgname;
    std::string id;
    int         extent_size;
    int         seqno;
    int         max_lv;
    int         max_pv;
    physical_volume::vtr pvolumes;
    logical_volume::vtr  lvolumes;
    volume_group(std::string &_id, std::string &name, int seq, int _extent_size) : max_lv(0), max_pv(0){
        vgname = name;
        id = _id;
        seqno = seq;
        extent_size = _extent_size;
    }
    ~volume_group(){}
    physical_volume::ptr find_physical_volume(std::string &_id) {
        std::string __id = _id;
        replace_all(__id, "-", "");
        auto it = std::find_if(pvolumes.begin(), pvolumes.end(), [&__id](const physical_volume::ptr& pv){ return pv->id == __id; });
        if (it != pvolumes.end())
            return *it;
        return NULL;
    }

    physical_volume::ptr add_physical_volume(std::string &_name, std::string &_id, uint64_t devsize, uint32_t start, uint32_t count){
        std::string __id = _id;
        replace_all(__id, "-", "");
        auto it = std::find_if(pvolumes.begin(), pvolumes.end(), [&__id](const physical_volume::ptr& pv){ return pv->id == __id; });
        if (it != pvolumes.end()){
            return *it;
        }
        else{
            physical_volume::ptr pv = physical_volume::ptr(new physical_volume());
            pv->id = __id;
            pv->dev_size = devsize;
            pv->pe_start = start;
            pv->pe_count = count;
            pv->name = _name;
            pvolumes.push_back(pv);
            return pv;
        }
    }

    logical_volume::ptr  find_logical_volume(std::string &_id){
        std::string __id = _id;
        replace_all(__id, "-", "");
        auto it = std::find_if(lvolumes.begin(), lvolumes.end(), [&__id](const logical_volume::ptr& lv){ return lv->id == __id; });
        if (it != lvolumes.end())
            return *it;
        return NULL;
    }
    
    logical_volume::ptr  add_logical_volume(std::string &_id, int count, std::string &vname){
        std::string __id = _id;
        replace_all(__id, "-", "");
        auto it = std::find_if(lvolumes.begin(), lvolumes.end(), [&__id](const logical_volume::ptr& lv){ return lv->id == __id; });
        if (it != lvolumes.end())
            return *it;
        else{
            logical_volume::ptr lv = logical_volume::ptr(new logical_volume(__id, count, vname, extent_size, this));
            lvolumes.push_back(lv);
            return lv;
        }
    }
    logical_volume::vtr logical_mount(logical_volume::vtr& unmounts);
};

struct lvm_metadata{
    universal_disk_rw::ptr rw;
    uint64_t               offset;
    uint64_t               pv_label_hdr_addr;  
    uint32_t               pv_label_hdr_len;   
    uint64_t               pv_label_addr;      
    uint32_t               pv_label_len;  
    uint64_t               vg_meta_addr;       
    uint32_t               vg_meta_len;
    std::string            pv_id;
    std::string            pv_metadata;
    lvm_metadata(universal_disk_rw::ptr _rw, uint64_t _offset) : 
        rw(_rw), 
        offset(_offset), 
        pv_label_hdr_addr(0), 
        pv_label_hdr_len(0), 
        pv_label_addr(0), 
        pv_label_len(0), 
        vg_meta_addr(0), 
        vg_meta_len(0){
    }
};

class logical_volume_manager{
private:
    std::vector<lvm_metadata> metadatas;
    volume_group::vtr         volume_groups;
    bool                      parse_metadata(std::string pv_metadata);
    volume_group::ptr         find_volgroup(std::string &id){
        auto it = std::find_if(volume_groups.begin(), volume_groups.end(), [&id](const volume_group::ptr& vg){ return vg->id == id; });
        if (it != volume_groups.end())
            return *it;
        return NULL;
    }
    volume_group::ptr       add_volgroup(std::string &id, std::string &name, int seq, int size){
        volume_group::ptr vg = find_volgroup(id);
        if (!vg){
            vg = volume_group::ptr(new volume_group(id, name, seq, size));
            volume_groups.push_back(vg);
        }
        return vg;
    }
    volume_group::vtr      get_volume_groups(void) { return volume_groups; }
public:
    typedef std::map<std::string, std::set<std::wstring>> groups_luns_map;
    groups_luns_map        get_groups_luns_mapping();
    bool                   scan_pv(universal_disk_rw::ptr _rw, uint64_t _offset);
    io_range::map get_system_ranges();
    logical_volume_manager() {
    }
    ~logical_volume_manager(){
    }   
};

class logical_volume_rw : public universal_disk_rw{
public:
    static universal_disk_rw::ptr get(logical_volume::ptr lv){
        return universal_disk_rw::ptr(new logical_volume_rw(lv));
    }
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout LPVOID buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool write(__in uint64_t start, __in LPCVOID buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written){
        return false;
    }
    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){
        return false;
    }
    virtual universal_disk_rw::ptr clone(){
        return universal_disk_rw::ptr(new logical_volume_rw(_lv));
    }
    virtual int sector_size() { return _sector_size; }
    virtual std::wstring path() const { return macho::stringutils::convert_utf8_to_unicode(_lv->volname); }
    virtual ~logical_volume_rw(){}
    io_range::map get_ranges_mapper(const io_range::vtr &ranges);
private:
    io_range::map mapper(__in uint64_t start, __in uint64_t length);
    logical_volume_rw(logical_volume::ptr lv) : _lv(lv), _sector_size(SECTOR_SIZE){}
    logical_volume::ptr _lv;
    int                 _sector_size;
};
#endif
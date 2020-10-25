#pragma once

#ifndef _SHRINK_VOLUME_H_
#define _SHRINK_VOLUME_H_ 1

#include "macho.h"
#include "ntfs_parser.h"

class shrink_volume{
public:
    typedef boost::shared_ptr<shrink_volume> ptr;
    static shrink_volume::ptr open(boost::filesystem::path p, size_t size);
    bool is_dirty(){ return _dirty_flag & VOLUME_IS_DIRTY; }
    bool can_be_shrink();
    bool shrink();
private:
    bool defragment();
    static HANDLE defrag_file_open(std::wstring file, DWORD attributes = 0 );
    static void defrag_file_close(HANDLE h);
    bool shrink_prepare();
    bool shrink_commit();
    bool shrink_abort();
    bool grow_partition();
#pragma pack(push, 1)
    typedef struct {
        LARGE_INTEGER VolumeSerialNumber;
        LARGE_INTEGER NumberSectors;
        LARGE_INTEGER TotalClusters;
        LARGE_INTEGER FreeClusters;
        LARGE_INTEGER TotalReserved;
        DWORD BytesPerSector;
        DWORD BytesPerCluster;
        DWORD BytesPerFileRecordSegment;
        DWORD ClustersPerFileRecordSegment;
        LARGE_INTEGER MftValidDataLength;
        LARGE_INTEGER MftStartLcn;
        LARGE_INTEGER Mft2StartLcn;
        LARGE_INTEGER MftZoneStart;
        LARGE_INTEGER MftZoneEnd;
    } NTFS_DATA, *PNTFS_DATA;
#pragma pack(pop)

    struct data_range{
        typedef std::vector<data_range> vtr;
        data_range() : start(0), length(0){}
        data_range(uint64_t _start, uint64_t _length) : start(_start), length(_length){}
        struct compare {
            bool operator() (data_range const & lhs, data_range const & rhs) const {
                return lhs.length > rhs.length;
            }
        };
        uint64_t start;
        uint64_t length;
    };

    struct file_data_range{
        typedef std::vector<file_data_range> vtr;
        file_data_range() : vcn(0), count(0){}
        file_data_range(uint64_t _vcn, uint64_t _lcn, uint64_t _count) : vcn(_vcn), lcn(_lcn), count(_count){}
        struct compare {
            bool operator() (file_data_range const & lhs, file_data_range const & rhs) const {
                return lhs.vcn < rhs.vcn;
            }
        };
        uint64_t vcn;
        uint64_t lcn;
        uint64_t count;
    };

    struct fragment_file{
        fragment_file() : attributes(0){}
        typedef std::vector<fragment_file> vtr;
        struct compare {
            bool operator() (fragment_file& lhs, fragment_file& rhs) const {
                return lhs.total_count() > rhs.total_count();
            }
        };
        uint64_t total_count() {
            uint64_t l = 0;
            foreach(file_data_range &e, extents){
                l += e.count;
            }
            return l;
        }
        boost::filesystem::path file;
        file_data_range::vtr extents;
        DWORD                attributes;
    };

#ifndef FILE_ID
    typedef struct _FILE_ID_64 {
        uint64_t   Value;
        uint64_t   UpperZero;
    } FILE_ID_64;

    typedef struct _FILE_ID{
        union {
            FILE_ID_64  FileId64;
            FILE_ID_128 ExtendedFileId;
        }FileId;
    }FILE_ID;
#endif

    struct record {
        typedef boost::shared_ptr<record> ptr;
        typedef std::vector<ptr> vtr;
        USN				usn;
        DWORD			reason;
        std::wstring    filename;
        FILE_ID         frn;
        FILE_ID		    parent_frn;
        DWORD           attributes;
    };

    shrink_volume(boost::filesystem::path p, size_t size) 
        : _path(p)
        , _new_size(size)
        , _bytes_per_cluster(0)
        , _file_area_offset(0)
        , _number_of_free_clusters(0)
        , _total_number_of_clusters(0)
        , _serial_number(0)
        , _file_system_flags(0)
        , _dirty_flag(0)
        {
            memset(&_ntfs_info, 0, sizeof(NTFS_DATA));
    }
    static BOOL                      _get_fat_first_sector_offset(HANDLE handle, ULONGLONG& file_area_offset);
    std::wstring                     _get_filename_by_handle(HANDLE handle);
    data_range::vtr                  _get_used_space_ranges();
    data_range::vtr                  _get_free_space_ranges(data_range& _latest_used_data_range = data_range());
    file_data_range::vtr             _get_file_retrieval_pointers(HANDLE handle);
    data_range::vtr                  _get_data_ranges(LPBYTE buff, uint64_t start_lcn, uint64_t total_number_of_bits);
    void                             _defragment(fragment_file::vtr& fragment_files);
    bool                             _scan_mft(fragment_file::vtr& fragment_files);
    bool                             _lookup_stream_from_clusters(fragment_file::vtr& fragment_files);
    bool                             _enum_mft(fragment_file::vtr& fragment_files);
    void                             _scan_mft(ntfs::volume::ptr vol, ntfs::file_record::ptr dir, boost::filesystem::path p, fragment_file::vtr& fragment_files);
    bool                             _defragment(boost::filesystem::path dir, fragment_file::vtr& fragment_files);
    void                             _check_file(boost::filesystem::path f, fragment_file::vtr& fragment_files, DWORD attributes = 0);
    bool                             _move_file(HANDLE handle, uint64_t start_vcn, uint64_t start_lcn, DWORD count);
    uint64_t                         _new_size;
    uint64_t                         _size;
    uint64_t                         _size_remaining;
    boost::filesystem::path          _path;
    std::wstring                     _name;
    macho::windows::auto_file_handle _handle;
    uint32_t                         _bytes_per_cluster;
    uint64_t                         _file_area_offset;
    DWORD                            _number_of_free_clusters;
    DWORD                            _total_number_of_clusters;
    DWORD                            _serial_number;
    DWORD		                     _file_system_flags;			// File system flags	
    std::wstring                     _file_system_type;			    // File system
    std::wstring                     _label;
    NTFS_DATA                        _ntfs_info;
    ULONG                            _dirty_flag;
    uint32_t                         _aligned_cluster;
};

#endif

#pragma once
#ifndef ntfs_parser_H
#define ntfs_parser_H

#include <Windows.h>
#include "..\vcbt\vcbt\ntfs.h"
#include "..\gen-cpp\universal_disk_rw.h"
#include <vector>

namespace ntfs {
#define	ATTR_NUMS		16				// Attribute Types count
#define	ATTR_INDEX(at)	(((at)>>4)-1)	// Attribute Type to Index, eg. 0x10->0, 0x30->2
#define	ATTR_MASK(at)	(((DWORD)1)<<ATTR_INDEX(at))	// Attribute Bit Mask

    // Bit masks of Attributes
#define	MASK_STANDARD_INFORMATION	ATTR_MASK(ATTR_TYPE_STANDARD_INFORMATION)
#define	MASK_ATTRIBUTE_LIST			ATTR_MASK(ATTR_TYPE_ATTRIBUTE_LIST)
#define	MASK_FILE_NAME				ATTR_MASK(ATTR_TYPE_FILE_NAME)
#define	MASK_OBJECT_ID				ATTR_MASK(ATTR_TYPE_OBJECT_ID)
#define	MASK_SECURITY_DESCRIPTOR	ATTR_MASK(ATTR_TYPE_SECURITY_DESCRIPTOR)
#define	MASK_VOLUME_NAME			ATTR_MASK(ATTR_TYPE_VOLUME_NAME)
#define	MASK_VOLUME_INFORMATION		ATTR_MASK(ATTR_TYPE_VOLUME_INFORMATION)
#define	MASK_DATA					ATTR_MASK(ATTR_TYPE_DATA)
#define	MASK_INDEX_ROOT				ATTR_MASK(ATTR_TYPE_INDEX_ROOT)
#define	MASK_INDEX_ALLOCATION		ATTR_MASK(ATTR_TYPE_INDEX_ALLOCATION)
#define	MASK_BITMAP					ATTR_MASK(ATTR_TYPE_BITMAP)
#define	MASK_REPARSE_POINT			ATTR_MASK(ATTR_TYPE_REPARSE_POINT)
#define	MASK_EA_INFORMATION			ATTR_MASK(ATTR_TYPE_EA_INFORMATION)
#define	MASK_EA						ATTR_MASK(ATTR_TYPE_EA)
#define	MASK_LOGGED_UTILITY_STREAM	ATTR_MASK(ATTR_TYPE_LOGGED_UTILITY_STREAM)

#define	MASK_ALL					((DWORD)-1)
    class volume;
    class file_record;
    struct io_range{
        typedef std::vector<io_range> vtr;
        io_range(ULONGLONG s, ULONGLONG l) : start(s), length(l) {}
        ULONGLONG start;
        ULONGLONG length;
    };

    struct file_extent {
        typedef std::vector<file_extent> vtr;
        file_extent() : start(0), physical(0), length(0){}
        file_extent(ULONGLONG s, ULONGLONG p, ULONGLONG l) : start(s), physical(p), length(l){}
        struct compare {
            bool operator() (file_extent const & lhs, file_extent const & rhs) const {
                return (rhs).start > (lhs).start;
            }
        };
        ULONGLONG start;
        ULONGLONG physical;
        ULONGLONG length;
    };

    struct data_run : public boost::noncopyable{
        typedef boost::shared_ptr<data_run> ptr;
        typedef std::vector<data_run::ptr> vtr;
        data_run() : lcn(0), clusters(0), start_vcn(0), last_vcn(0){}
        LONGLONG			lcn;		// -1 to indicate sparse data
        ULONGLONG			clusters;
        ULONGLONG			start_vcn;
        ULONGLONG			last_vcn;
    };

    class attributes : public boost::noncopyable{
    public:
        typedef boost::shared_ptr<attributes> ptr;
        typedef std::vector<attributes::ptr> vtr;
        typedef std::map<int, attributes::vtr> map;
        bool read_data(const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural);
    private:
        bool read_non_resident_data(const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural);
        attributes() : ahc(NULL), attr_body(NULL), attr_body_size(0){}
        bool read_virtual_clusters(ULONGLONG vcn, DWORD clusters,
            void *bufv, DWORD bufLen, DWORD *actural);
        bool read_clusters(void *buf, DWORD clusters, LONGLONG lcn);
        friend class file_record;
        friend class volume;
        PNTFS_ATTRIBUTE                           ahc;
        DWORD                                     attr_body_size;
        DWORD                                     cluster_size;
        ULONGLONG                                 offset;
        data_run::vtr                             data_runs;
        boost::shared_ptr<BYTE>                   attr_body;
        universal_disk_rw::ptr                    rw;
    };

    class file_record : public boost::noncopyable{
    public:
        typedef boost::shared_ptr<file_record> ptr;
        typedef std::vector<file_record::ptr> vtr;      
        bool parse_attrs(DWORD mask);
        bool is_encrypted();
        bool is_compressed();
        bool is_sparse();
        bool is_deleted();
        bool is_directory();
        file_extent::vtr extents();
        bool read(const ULONGLONG offset, void *bufv, DWORD bufLen, DWORD *actural);
        ULONGLONG file_size();
        ULONGLONG file_allocated_size();
    private:
        file_record(ULONGLONG _fref, boost::shared_ptr<FILE_RECORD_HEADER> _fr) : fileRef(_fref), fr(_fr), offset(0){}
        bool parse_attr( PNTFS_ATTRIBUTE ahc );
        bool parse_non_resident_attr(PNTFS_ATTRIBUTE ahc, attributes& attr);
        bool pick_data(const BYTE **dataRun, LONGLONG *length, LONGLONG *LCNOffset);
        friend class volume;
        ULONGLONG                                      fileRef;
        DWORD                                          cluster_size;
        DWORD                                          file_record_size;
        ULONGLONG                                      offset;
        boost::shared_ptr<FILE_RECORD_HEADER>          fr;
        attributes::map                                attrs;
        universal_disk_rw::ptr                         rw;
    };

    class volume : public boost::noncopyable{
    public:
        typedef boost::shared_ptr<volume> ptr;
        typedef std::vector<volume::ptr> vtr;
        static volume::vtr get(universal_disk_rw::ptr rw);
        static volume::ptr get(universal_disk_rw::ptr& rw, ULONGLONG _offset);
        file_record::ptr   get_file_record(ULONGLONG fileRef, DWORD mask = (MASK_INDEX_ROOT | MASK_INDEX_ALLOCATION | MASK_FILE_NAME | MASK_DATA));
        file_record::ptr   root(){
            return get_file_record(MFT_IDX_ROOT, MASK_INDEX_ROOT | MASK_INDEX_ALLOCATION);
        }
        file_record::ptr   find_sub_entry(const std::wstring file_name, const file_record::ptr dir = NULL );
        io_range::vtr      file_system_ranges();
        ULONGLONG          start() const { return offset; } //physcial offset, if rw=disk handle; 0, if rw=volume handle
        ULONGLONG          length() const { return total_size; }
    private:
        bool visit_index_block(const file_record::ptr dir, const ULONGLONG vcn, const std::wstring& file_name, ULONGLONG &fileRef);
        boost::shared_ptr<FILE_RECORD_HEADER> read_file_record(ULONGLONG fileRef);
        bool patch_us(WORD *sector, int sectors, WORD usn, WORD *usarray);
        WORD get_volume_version(file_record &fs);

        volume(universal_disk_rw::ptr &_rw, ULONGLONG _offset) :
            rw(_rw),
            offset(_offset),
            sector_size(0),
            cluster_size(0),
            total_size(0),
            file_record_size(0),
            index_block_size(0),
            version(0),
            mft_addr(0)
            {}
        WORD                    sector_size;
        WORD                    version;
        DWORD                   cluster_size;
        DWORD                   file_record_size;
        DWORD                   index_block_size;
        ULONGLONG               offset;
        ULONGLONG               total_size;
        ULONGLONG               mft_addr;
        attributes::ptr         mft_data;
        file_record::ptr        mft;
        universal_disk_rw::ptr  rw;
    };
};

#endif
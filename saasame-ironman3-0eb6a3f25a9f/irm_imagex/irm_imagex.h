#pragma once

#ifndef __IRM_IMAGEX__
#define __IRM_IMAGEX__

#include <string>
#include <vector>
#include <deque>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/map.hpp>
#include <boost/regex/v4/regex.hpp>
#include <boost/regex/v4/regex_match.hpp>
#include "universal_disk_rw.h"
#include "lz4.h"
#include "lz4hc.h"
#include "checksum_db.h"
#if 0
#include "lz4mt.h"
#endif

#define IRM_IMAGE_SECTOR_SIZE       512
#define IRM_QUEUE_SIZE              5
#define IRM_IMAGE_JOURNAL_EXT       L".journal"
#define IRM_IMAGE_JOURNAL_EXT_A     ".journal"
#define IRM_IMAGE_CDR_EXT           L".cdr"
#define IRM_IMAGE_VERSION_2_EXT     L".v2"
#define IRM_IMAGE_CLOSE_EXT         L".close"
#define IRM_IMAGE_CHECKSUM_EXT      L".checksum"
#define IRM_IMAGE_DEDUPE_TMP_EXT    L".dp"
#define IRM_IMAGE_LOCAL_DB_DIR      boost::filesystem::path("db")
#define TEMP_BUFFER_SIZE            1024
#define IRM_IMAGE_BLOCK_MODE        0
#define IRM_IMAGE_TRANSPORT_MODE    2

namespace saasame { namespace ironman { namespace imagex {
struct irm_imagex_base{
    virtual bool save(std::ostream& data, bool forcefull = false) = 0;
    virtual bool load(std::istream&) = 0;
};
struct irm_transport_image_block;
class irm_imagex_op
{
public:
    irm_imagex_op() : _canceled(false){}
    typedef boost::shared_ptr<irm_imagex_op> ptr;
    typedef std::vector<ptr> vtr;
    struct  exception : public macho::exception_base {};
    virtual bool is_file_existing(__in boost::filesystem::path file) = 0;
    virtual bool read_file(__in boost::filesystem::path file, __inout irm_transport_image_block& data) = 0;
    virtual bool write_file(__in boost::filesystem::path file, __in irm_transport_image_block& data, bool forcefull = false) = 0;

    virtual bool read_file(__in boost::filesystem::path file, __inout std::ostream& data) = 0;
    virtual bool write_file(__in boost::filesystem::path file, __in std::istream& data) = 0;
    virtual bool remove_file(__in boost::filesystem::path file) = 0;
    virtual bool is_temp_file_existing(__in boost::filesystem::path file) = 0;

    //virtual bool read_temp_file(__in boost::filesystem::path file, __inout std::ostream& data) = 0;
    //virtual bool write_temp_file(__in boost::filesystem::path file, __in std::istream& data) = 0;

    virtual bool read_temp_file(__in boost::filesystem::path file, __inout irm_transport_image_block& data) = 0;
    virtual bool write_temp_file(__in boost::filesystem::path file, __in irm_transport_image_block& data) = 0;

    virtual bool remove_temp_file(__in boost::filesystem::path file) = 0;
    virtual bool is_metafile_existing(__in boost::filesystem::path file) = 0;

    //virtual bool read_metafile(__in boost::filesystem::path file, __inout std::stringstream& data) = 0;
    virtual bool write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool local_only = false) = 0;

    virtual bool read_metafile(__in boost::filesystem::path file, __inout irm_imagex_base& data) = 0;
    virtual bool write_metafile(__in boost::filesystem::path file, __in irm_imagex_base& data, bool local_only = false) = 0;

    virtual bool remove_metafile(__in boost::filesystem::path file) = 0;
    virtual bool remove_metafiles(__in std::vector<boost::filesystem::path>& files) = 0;
    virtual bool flush_metafiles(__in std::vector<boost::filesystem::path>& files) = 0;
    virtual bool remove_image(__in std::wstring& image_name) = 0;
    virtual bool release_image_lock(__in std::wstring& image_name) = 0;
    virtual macho::windows::lock_able::vtr get_lock_objects(std::wstring& lock_filename) = 0;
    virtual std::wstring get_path_name() const = 0;
    virtual void cancel(){ _canceled = true; }
    virtual bool is_canceled() { return _canceled; }
protected:
    bool                    _canceled;
};

struct irm_transport_image_block : virtual public irm_imagex_base
{
    typedef boost::shared_ptr<irm_transport_image_block> ptr;
    typedef std::map<std::wstring, ptr>  map;
    typedef std::deque<ptr>  queue;
    typedef std::vector<ptr> vtr;

    struct compare 
    {
        bool operator() (irm_transport_image_block::ptr const & lhs, irm_transport_image_block::ptr const & rhs) const
        {
            return (*lhs).clock < (*rhs).clock;
        }
    };

    enum block_flag
    {
        BLOCK_READY         = 0x00,
        BLOCK_MERGE         = 0x01,
        BLOCK_COMPRESSED    = 0x02,
        BLOCK_CRC_MD5       = 0x04,
        BLOCK_FLUSHED       = 0x08,
        BLOCK_FLUSH_TMP     = 0x10
    };
    irm_transport_image_block(uint64_t _start, uint32_t _length, uint32_t _mode = 1);
    enum block_size
    {
        BLOCK_16MB = 1024 * 1024 * 16,
        BLOCK_32MB = 1024 * 1024 * 32,
        BLOCK_64MB = 1024 * 1024 * 64
    };
    int                     index;
    int                     flags;
    uint32_t                mode;
    uint8_t                 md5[16];
    uint32_t                crc;
    uint64_t                start;
    uint32_t                length;             // equal to block_size value
    bool                    duplicated;
    bool                    compressed;
    bool                    dirty;
    std::vector<uint8_t>    bitmap;
    std::vector<uint8_t>    data;
    uint64_t                clock;
    std::wstring            image_block_name;
    const std::string*      p_data;
    static void calc_md5_val(const std::string& src, std::string& digest);
    static void calc_crc32_val(const std::string& src, uint32_t& crc32);

    virtual bool save(std::ostream& outbuf, bool forcefull = false);
    bool load_header(std::istream& inbuf); // exclude data field
    virtual bool load(std::istream& inbuf);
};

struct irm_transport_image_blks_chg_journal : virtual public irm_imagex_base
{
    typedef boost::shared_ptr<irm_transport_image_blks_chg_journal> ptr;
    typedef std::vector<ptr> vtr;
    irm_transport_image_blks_chg_journal() : next_block_index(0){}
    void copy(const irm_transport_image_blks_chg_journal& journal);
    const irm_transport_image_blks_chg_journal &operator =(const irm_transport_image_blks_chg_journal& journal);

    std::wstring                name;
    std::wstring                comment;
    std::wstring                parent_path;            //path of block image file
    std::wstring                extension;              //extension name of block image file
    std::vector<uint32_t>       dirty_blocks_list;      //block image file name
    uint64_t                    next_block_index;
    bool is_cdr() { return std::wstring::npos != extension.find(IRM_IMAGE_CDR_EXT); }
    bool is_version_2() { return std::wstring::npos != extension.find(IRM_IMAGE_VERSION_2_EXT); }
	bool is_closed() { return std::wstring::npos != extension.find(IRM_IMAGE_CLOSE_EXT); }
    virtual bool load(std::istream& inbuf);
    virtual bool save(std::ostream& outbuf, bool forcefull);
};

class irm_transport_image : public universal_disk_rw , virtual public irm_imagex_base
{
public:
    typedef boost::shared_ptr<irm_transport_image> ptr;
    irm_transport_image();
    ~irm_transport_image();

    std::wstring                base_name;
    std::wstring                name;
    std::wstring                comment;
    std::wstring                parent;
    uint64_t                    total_size;
    uint32_t                    block_size;
    uint32_t                    queue_size; // 0 == unlimited
    bool                        compressed;
    uint8_t                     mode;
    bool                        checksum;
    bool                        completed;
    bool                        canceled;
    bool                        checksum_verify;
    bool                        good;
    bool                        cdr;
    std::vector<uint8_t>        blocks;

    static bool create(std::wstring& base_image_name, std::wstring& out_image_name, irm_imagex_op::ptr& op, uint64_t size, irm_transport_image_block::block_size block_size = irm_transport_image_block::BLOCK_32MB, bool compressed = false, bool checksum = false, uint8_t mode = 0, std::wstring parent = L"", bool checksum_verify = true, std::wstring comment = L"", bool cdr = false);
    static irm_transport_image::ptr open(std::wstring& base_name, std::wstring& name, irm_imagex_op::ptr& op, int buffer_size = IRM_QUEUE_SIZE * 32 * 1024 * 1024);
   
    virtual bool read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read);
    virtual bool write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written) override;
    virtual bool write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written) override;
    bool write_ex(__in uint64_t start, __in const std::string& buffer, __in uint32_t number_of_bytes_to_write, __in bool is_compressed);
    bool create_or_load_checksum_db(boost::filesystem::path working_path);
    std::wstring path() const { return name; }
    bool flush(bool is_terminated = false); //flush current block and reset dirty bit, auto flush followed by write
    bool close(const bool is_canceled = false);
    bool is_buffer_free();
    std::wstring                get_block_name(uint32_t index);
    std::wstring                get_block_name(uint64_t index);

    virtual bool sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read);
    virtual bool sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written);
    virtual universal_disk_rw::ptr clone();
    virtual bool load(std::istream& inbuf);
    virtual bool save(std::ostream& outbuf, bool forcefull);

private:
    friend class boost::serialization::access;
    uint64_t                                    _number_of_blocks;
    int                                         _number_of_buffer_blocks;
    irm_transport_image_block::vtr              _buffer_queue;
    irm_transport_image_block::queue            _process_blocks;
    irm_transport_image_block::queue            _error_blocks;
    irm_transport_image_block::map              _working_blocks;

    boost::mutex                                _access_buffer_queue;
    boost::mutex                                _completion_lock;
    boost::mutex                                _write_lock;

    boost::condition_variable                   _flush_cond;
    boost::condition_variable                   _completion_cond;

    bool                                        _irm_transport_error;
    bool                                        _irm_transport_completed;
    bool                                        _closed;
    bool                                        _local_flush;
    uint32_t                                    _written_bytes_per_call;
    uint32_t                                    _write_bytes_per_call;
    uint64_t                                    _total_written_bytes;
    uint64_t                                    _total_bytes_to_write;
    checksum_db::ptr                            _base_checksum_db;
    checksum_db::ptr                            _checksum_db;
    boost::filesystem::path                     _checksum_file_path;
    boost::filesystem::path                     _meta_file_path;

    irm_transport_image_blks_chg_journal::ptr   _journal_db;
    boost::thread_group                         _flush_workers;
   
    irm_imagex_op::ptr                          _op;
    bool                                        _terminated;

#if 0
    Lz4MtContext                                _lz4mt_ctx;
#endif

    bool _write(__in uint64_t start, __in uint32_t number_of_bytes_to_write, __in const void *src, __in irm_transport_image_block::ptr dst, __in std::wstring name = L"", __in bool is_last_chunk = false);

    bool _flush_proc();
    bool _check_queue_size();
    bool _merge_proc(__in irm_transport_image_block* block);
    bool _crc_md5_proc(__in irm_transport_image_block* block);
    bool _compress_proc(__in irm_transport_image_block* block);
    bool _compress_proc(__in irm_transport_image_block* block, __in const std::string& buffer);
    bool _flush_proc(__in irm_transport_image_block* block);
    int	 _compress(__in const char* source, __inout char* dest, __in int source_size, __in int max_compressed_size);
    int  _decompress(__in const char* source, __inout char* dest, __in int compressed_size, __in int max_decompressed_size);
    bool _duplicate_verify(__in const uint64_t& block_index, __in const std::vector<uint8_t> &bitmap, __in const uint32_t& crc, __in const uint8_t *md5, __in const int size);
    bool _create_journal_db(__in const std::wstring& name, __in const std::wstring& comment, __in const bool cdr);
    bool _load_journal_db(__in const std::wstring& name, __inout std::wstring& comment, __inout bool& cdr);
    bool _update_journal_db();
    bool _update_meta_file(__in const bool is_completed = false);
    bool _flush_metafiles();
    void _terminate();
};


}}}

#endif
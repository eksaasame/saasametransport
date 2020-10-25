#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/crc.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <macho.h>
#include <openssl/evp.h>
#include "saasame_types.h"
#if ( defined(CARRIER_DEBUG) && !BOOST_PP_IS_EMPTY(CARRIER_DEBUG) )
#define __CARRIER_DEBUG
#endif

#ifdef __CARRIER_DEBUG
#pragma message("CARRIER debug option enabled")
#endif

#include "irm_imagex.h"

using namespace boost;
using namespace macho; 
using namespace saasame::ironman::imagex;
using namespace saasame::transport;

typedef int(*CompressionFunc) (const char* src, char* dst, int size, int maxOut, int maxOutputSize);

int bridge_LZ4_compress_limitedOutput(const char* src, char* dst, int size, int maxOut, int) 
{
    return LZ4_compress_limitedOutput(src, dst, size, maxOut);
}

std::wstring irm_transport_image::get_block_name(uint32_t index){
    boost::filesystem::path p(name);
    return boost::str(boost::wformat(L"%s-%05d%s") % p.stem().wstring() % index % p.extension().wstring());
}

std::wstring irm_transport_image::get_block_name(uint64_t index){
    boost::filesystem::path p(name);
    return boost::str(boost::wformat(L"%s-%08llu%s") % p.stem().wstring() % index % p.extension().wstring());
}

bool irm_transport_image::sector_read(__in uint64_t start_sector, __in uint32_t number_of_sectors_to_read, __inout void *buffer, __inout uint32_t& number_of_sectors_read){
    uint32_t number_of_bytes_read = 0;
    number_of_sectors_read = 0;
    if (read(start_sector *IRM_IMAGE_SECTOR_SIZE, number_of_sectors_to_read *IRM_IMAGE_SECTOR_SIZE, buffer, number_of_bytes_read)){
        number_of_sectors_read = number_of_bytes_read / IRM_IMAGE_SECTOR_SIZE;
        return true;
    }
    return false;
}

bool irm_transport_image::sector_write(__in uint64_t start_sector, __in const void *buffer, __in uint32_t number_of_sectors_to_write, __inout uint32_t& number_of_sectors_written){
    uint32_t number_of_bytes_written = 0;
    number_of_sectors_written = 0;
    if (write(start_sector *IRM_IMAGE_SECTOR_SIZE, buffer, number_of_sectors_to_write*IRM_IMAGE_SECTOR_SIZE, number_of_bytes_written)){
        number_of_sectors_written = number_of_bytes_written / IRM_IMAGE_SECTOR_SIZE;
        return true;
    }
    return false;
}

universal_disk_rw::ptr irm_transport_image::clone(){
    return NULL;
}

bool irm_transport_image::load(std::istream& inbuf)
{
    uint32_t len = 0;
    using namespace macho;
    uint32_t buff_size = TEMP_BUFFER_SIZE;
    try
    {
        std::auto_ptr<wchar_t> buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        inbuf.exceptions(std::istream::failbit | std::istream::badbit);
        inbuf.seekg(0, std::ios::beg);
        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        base_name = buff.get();

        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        name = buff.get();

        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        parent = buff.get();

        inbuf.read((char *)&total_size, sizeof(total_size));
        inbuf.read((char *)&block_size, sizeof(block_size));
        inbuf.read((char *)&compressed, sizeof(compressed));
        inbuf.read((char *)&mode, sizeof(mode));
        inbuf.read((char *)&checksum, sizeof(checksum));
        inbuf.read((char *)&completed, sizeof(completed));
        inbuf.read((char *)&canceled, sizeof(canceled));
        inbuf.read((char *)&checksum_verify, sizeof(checksum_verify));
        inbuf.read((char *)&good, sizeof(good));
        if (mode != IRM_IMAGE_TRANSPORT_MODE){
            inbuf.read((char *)&len, sizeof(len));
            if (len){
                blocks.resize(len);
                inbuf.read((char *)&blocks[0], len);
            }
        }
        inbuf.seekg(0, std::ios::beg);
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

bool irm_transport_image::save(std::ostream& outbuf, bool forcefull)
{
    uint32_t len = 0;
    using namespace macho;
    try
    {
        outbuf.exceptions(std::ostream::failbit | std::ostream::badbit);
        //outbuf.swap(std::stringstream());
        outbuf.seekp(0, std::ios::beg);

        len = base_name.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)base_name.c_str(), len);

        len = name.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)name.c_str(), len);

        len = parent.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)parent.c_str(), len);

        outbuf.write((char *)&total_size, sizeof(total_size));
        outbuf.write((char *)&block_size, sizeof(block_size));
        outbuf.write((char *)&compressed, sizeof(compressed));
        outbuf.write((char *)&mode, sizeof(mode));
        outbuf.write((char *)&checksum, sizeof(checksum));
        outbuf.write((char *)&completed, sizeof(completed));
        outbuf.write((char *)&canceled, sizeof(canceled));
        outbuf.write((char *)&checksum_verify, sizeof(checksum_verify));
        outbuf.write((char *)&good, sizeof(good));
        if (mode != IRM_IMAGE_TRANSPORT_MODE){
            len = blocks.size() * sizeof(uint8_t);
            outbuf.write((char *)&len, sizeof(len));
            if (len)
                outbuf.write((char *)&blocks[0], len);
        }
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

irm_transport_image::irm_transport_image() :
    total_size(0),
    block_size(0),
    queue_size(0),
    completed(false),
    canceled(false),
    _total_written_bytes(0),
    _written_bytes_per_call(0),
    _irm_transport_error(false),
    _irm_transport_completed(false),
    _closed(false),
    _local_flush(false),
    _terminated(false),
    _number_of_blocks(0),
    good(true),
    cdr(false),
    mode(0)
{
    _checksum_db = NULL;
    _journal_db = NULL;

#if 0
    _lz4mt_ctx = lz4mtInitContext();

    Lz4MtContext lz4mt_ctx = _lz4mt_ctx;

    _lz4mt_ctx.mode = static_cast<Lz4MtMode>(LZ4MT_MODE_PARALLEL);
    _lz4mt_ctx.compressBound = LZ4_compressBound;
    _lz4mt_ctx.decompress = LZ4_decompress_safe;
    _lz4mt_ctx.compressionLevel = 1;
    _lz4mt_ctx.compress = [&lz4mt_ctx]() -> CompressionFunc {
        // NOTE for "-> CompressionFunc" :
        //		It's a workaround for g++-4.6's strange warning.

        if (lz4mt_ctx.compressionLevel >= 3) 
        {
            return LZ4_compressHC2_limitedOutput;
        }
        else 
        {
            return bridge_LZ4_compress_limitedOutput;
        }
    }();
#endif
}

irm_transport_image::~irm_transport_image()
{
    std::wstring lck_file = base_name + L".lck";

    if (_op)
    {
        _flush_metafiles();
        _op->remove_file(lck_file);
    }

    if (_checksum_db)
        _checksum_db = NULL;

    if (_base_checksum_db)
        _base_checksum_db = NULL;

    if (_journal_db)
        _journal_db = NULL;
}

bool irm_transport_image::create(
    std::wstring& base_image_name, 
    std::wstring& out_image_name, 
    irm_imagex_op::ptr& op, 
    uint64_t size, 
    irm_transport_image_block::block_size block_size, 
    bool compressed, 
    bool checksum, 
    uint8_t mode,
    std::wstring parent, 
    bool checksum_verify,
    std::wstring comment,
    bool cdr)
{
    FUN_TRACE;
    bool result = true;
    try
    {
        std::wstring lck_file = base_image_name + L".lck";
        universal_disk_rw::ptr image(new irm_transport_image());
        DWORD wait_time = 10000;

        macho::windows::mutex mutex(base_image_name);

        if (!mutex.lock(wait_time))
        {
            LOG(LOG_LEVEL_ERROR, L"Acquire lock(%s) timeout", base_image_name.c_str());
            return false;
        }

        if (!op->is_file_existing(lck_file) && image)
        {
            try
            {
                irm_transport_image *img = dynamic_cast<irm_transport_image *>(image.get());

                img->name = out_image_name;
                img->comment = comment;
                img->parent = parent;
                img->total_size = size;
                img->block_size = block_size;
                img->compressed = compressed;
                img->mode = mode;
                img->checksum = checksum;
                img->_op = op;
                img->base_name = base_image_name;
                img->checksum_verify = checksum_verify;
                img->cdr = cdr;
                if (IRM_IMAGE_TRANSPORT_MODE != mode){
                    img->_number_of_blocks = ((img->total_size - 1) / img->block_size) + 1;
                    uint32_t len = ((img->_number_of_blocks - 1) / 8) + 1;
                    img->blocks.resize(len);
                    memset(&img->blocks[0], 0, len);
                }
                if (result = img->_create_journal_db(out_image_name, comment, cdr)){
                    result = op->write_metafile(IRM_IMAGE_LOCAL_DB_DIR / base_image_name / out_image_name, *img);
                }
                image = NULL;
            }
            catch (...)
            {
                LOG(LOG_LEVEL_ERROR, L"Got exception when saving journal and image files.");
                result = false;
            }
        }
        else
        {
            LOG(LOG_LEVEL_ERROR, L"File already created by other process or failed to initial image memory.");
            result = false;
        }

       /* if (result)
        {
            std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
            if (!op->write_file(lck_file, buffer))
                result = false;
        }*/
    }
    catch (macho::exception_base& ex)
    {
        std::string errmsg = ex.what();
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(errmsg).c_str());
        result = false;
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }

    return result;
}

irm_transport_image::ptr irm_transport_image::open(std::wstring& base_name, std::wstring& name, irm_imagex_op::ptr& op, int buffer_size)
{
    //FUN_TRACE;

    bool result = true;
    try{
        int32_t compress_workers = 3;
        irm_transport_image::ptr image(new irm_transport_image());
        irm_transport_image *img = (image.get());
        boost::filesystem::path meta_file_path = IRM_IMAGE_LOCAL_DB_DIR / base_name / name;

        if (op->is_metafile_existing(meta_file_path)){
            if (!op->read_metafile(meta_file_path, *img)){
                image = NULL;
                LOG(LOG_LEVEL_ERROR, L"Cannot read Image (%s).", meta_file_path.wstring().c_str());
            }
        }
        else{
            image = NULL;
            LOG(LOG_LEVEL_ERROR, L"Image (%s) does not exist.", meta_file_path.wstring().c_str());
        }
        if (image && img->good && (!img->name.empty()))
        {
            try
            {
                //std::wstring lck_file = base_name + L".lck";
                //if (!op->is_file_existing(lck_file))
                //{
                //    std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
                //    result = op->write_file(lck_file, buffer);
                //}
                //if (result)
                {
                    //initial empty data queue buffer
                    img->_meta_file_path = meta_file_path;
                    img->_buffer_queue.clear();
                    img->_op = op;
                    if (op->is_metafile_existing(boost::filesystem::path(IRM_IMAGE_LOCAL_DB_DIR / base_name / name).wstring()+(IRM_IMAGE_JOURNAL_EXT)))
                        result = img->_load_journal_db(name, img->comment, img->cdr);

                    if (IRM_IMAGE_TRANSPORT_MODE != img->mode){
                        img->_number_of_buffer_blocks = buffer_size > img->block_size ? buffer_size / img->block_size : IRM_QUEUE_SIZE;
                        img->_number_of_blocks = ((img->total_size - 1) / img->block_size) + 1;
                        img->_flush_workers.create_thread(boost::bind(&irm_transport_image::_flush_proc, img));
                        img->_flush_workers.create_thread(boost::bind(&irm_transport_image::_flush_proc, img));
                        for (uint32_t i = 0; i < img->_number_of_blocks; i++)
                        {
                            boost::filesystem::path temp_block_name = IRM_IMAGE_LOCAL_DB_DIR / img->base_name / img->get_block_name(i);
                            if (op->is_temp_file_existing(temp_block_name))
                            {
                                irm_transport_image_block::ptr ar_block = irm_transport_image_block::ptr(new irm_transport_image_block(0, img->block_size));
                                result = false;
                                if (ar_block && (result = op->read_temp_file(temp_block_name, *ar_block.get())))
                                {
                                    ar_block->image_block_name = img->get_block_name(i);
                                    ar_block->index = i;
                                    if (ar_block->compressed)
                                    {
                                        uint32_t decompressed_length = img->block_size;
                                        boost::shared_ptr<uint8_t> decompressed_buf(new uint8_t[img->block_size]);
                                        decompressed_length = img->_decompress((const char *)&ar_block->data[0], (char *)decompressed_buf.get(), ar_block->data.size(), img->block_size);
                                        ar_block->data.resize(decompressed_length);
                                        memcpy(&ar_block->data[0], decompressed_buf.get(), decompressed_length);
                                        ar_block->length = decompressed_length;
                                        ar_block->compressed = false;
                                        ar_block->flags = 0;
                                        img->_buffer_queue.push_back(ar_block);
                                    }
                                    op->remove_temp_file(temp_block_name);
                                }
                            }
                        }
                    }
                }

                //if (!result)
                //    image = NULL;
            }
            catch (const std::exception& ex){
                LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
                image = NULL;
            }
            catch (...){
                LOG(LOG_LEVEL_ERROR, L"Unknown exception");
                image = NULL;
            }
        }
        else{
            if (image){
                image = NULL;
                LOG(LOG_LEVEL_ERROR, L"Image (%s) is not good.", meta_file_path.wstring().c_str());
            }
        }
        return image;
    }
    catch (macho::exception_base& ex){
        std::string errmsg = ex.what();
        LOG(LOG_LEVEL_ERROR, L"%s", macho::stringutils::convert_utf8_to_unicode(errmsg).c_str());
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return NULL;
}

bool irm_transport_image::create_or_load_checksum_db(boost::filesystem::path working_path)
{
    bool result = false;
    try{
        _checksum_file_path = boost::filesystem::path(working_path / IRM_IMAGE_LOCAL_DB_DIR / base_name / base_name).wstring() + (IRM_IMAGE_CHECKSUM_EXT);

        if (!checksum){
            boost::filesystem::remove(_checksum_file_path);
        }
        else{
            _base_checksum_db = checksum_db::ptr(new checksum_db());
            _checksum_db = checksum_db::ptr(new checksum_db());
            if (_base_checksum_db){
                if (_base_checksum_db->open(":memory:")){
                    if (boost::filesystem::exists(_checksum_file_path)){
                        result = _base_checksum_db->load(_checksum_file_path.string());
                    }
                }
            }
            if (_checksum_db){
                result = _checksum_db->open(_checksum_file_path.string());
            }
        }
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        result = false;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    return result;
}

bool irm_transport_image::_create_journal_db(__in const std::wstring& name, __in const std::wstring& comment, __in const bool cdr) //image name
{
    bool result = true;
    try
    {
        _journal_db = boost::shared_ptr<irm_transport_image_blks_chg_journal>(new irm_transport_image_blks_chg_journal());

        if (_journal_db == NULL || _op == NULL)
            return false;
        _journal_db->parent_path = boost::filesystem::path(IRM_IMAGE_LOCAL_DB_DIR / base_name).wstring();
        _journal_db->extension = cdr ? IRM_IMAGE_CDR_EXT : IRM_IMAGE_JOURNAL_EXT;
        if (IRM_IMAGE_TRANSPORT_MODE == mode)
            _journal_db->extension.append(IRM_IMAGE_VERSION_2_EXT);
        _journal_db->name = boost::filesystem::path(IRM_IMAGE_LOCAL_DB_DIR / base_name / name).wstring() + (IRM_IMAGE_JOURNAL_EXT);
        _journal_db->comment = comment;

        if (!_op->write_metafile(_journal_db->name, *_journal_db.get()))
            result = false;
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        result = false;
    }  
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }

    if (!result && _journal_db != NULL)
        _journal_db = NULL;

    return result;
}

bool irm_transport_image::_load_journal_db(__in const std::wstring& name, __inout std::wstring& comment, __inout bool& cdr) //image name
{
    bool result = true;
    boost::filesystem::path journal_file = boost::filesystem::path(IRM_IMAGE_LOCAL_DB_DIR / base_name / name).wstring() + (IRM_IMAGE_JOURNAL_EXT);

    if (_op == NULL)
        return false;
    try
    {
        if (_journal_db == NULL)
            _journal_db = boost::shared_ptr<irm_transport_image_blks_chg_journal>(new irm_transport_image_blks_chg_journal());

        if (_journal_db == NULL){
            return false;
        }
        _journal_db->dirty_blocks_list.clear();
        if (_op->read_metafile(journal_file, *_journal_db.get())){
            cdr = std::wstring::npos != _journal_db->extension.find(IRM_IMAGE_CDR_EXT);
            comment = _journal_db->comment;
        }
        else
            result = false;
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        result = false;
    }  
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }

    if (!result && _journal_db != NULL)
        _journal_db = NULL;

    return result;
}

bool irm_transport_image::_update_journal_db()
{
    bool result = true;

    try
    {
        if (_journal_db && _op){
            result = _op->write_metafile(_journal_db->name, *_journal_db.get());
        }
    }
    catch (...){
        result = false;
    }

    if (!result && _journal_db != NULL)
        _journal_db = NULL;

    return result;
}

bool irm_transport_image::_update_meta_file(__in const bool is_completed)
{
    bool result = true;
    try{
        completed = is_completed;
        if (!_op->write_metafile(_meta_file_path, *this))
            result = false;
    }
    catch (...){
        result = false;
    }

    return result;
}

bool irm_transport_image::close(const bool is_canceled)
{
    bool result = true;
    std::ofstream checksum_of;

    LOG(LOG_LEVEL_DEBUG, _T("Close event triggering..."));
    boost::unique_lock<boost::mutex> lock(_write_lock);

    result = flush();
    _closed = true;

    if (is_canceled)
        canceled = true;
    
    if (_checksum_db && _base_checksum_db){
        _checksum_db = NULL;
        _base_checksum_db = NULL;
    }

    result = _update_meta_file(!is_canceled);

    if (result)
    {
        std::vector<boost::filesystem::path> files;
        std::wstring lck_file = base_name + L".lck";
        files.push_back(IRM_IMAGE_LOCAL_DB_DIR / base_name / name);
        files.push_back(boost::filesystem::path(IRM_IMAGE_LOCAL_DB_DIR / base_name / name).wstring() + (IRM_IMAGE_JOURNAL_EXT));

        if ((result = _flush_metafiles()))
        {
            result = _op->remove_metafiles(files);
            _op->remove_file(lck_file);
        }
        _checksum_db = NULL;
        //boost::filesystem::remove(_checksum_file_path);
    }
    _terminate();
    return result;
}

bool irm_transport_image::read(__in uint64_t start, __in uint32_t number_of_bytes_to_read, __inout void *buffer, __inout uint32_t& number_of_bytes_read)
{
    //FUN_TRACE;

    return true;
}

bool irm_transport_image::_write(__in uint64_t start, __in uint32_t number_of_bytes_to_write, __in const void *src, __in irm_transport_image_block::ptr dst, __in std::wstring name, __in bool is_last_chunk)
{
    bool result = true;
    uint8_t *buf = (uint8_t *)src;

    //if (memcmp(&dst->data[0] + start, &buf[0], number_of_bytes_to_write) != 0) //commit this line to avoid the overwirte data with empty issue
    {
        dst->dirty = true;
        uint32_t start_bit = start / IRM_IMAGE_SECTOR_SIZE;
        uint32_t end_bit = (start + number_of_bytes_to_write) / IRM_IMAGE_SECTOR_SIZE;
        for (uint32_t i = start_bit; i < end_bit; i++)
        {           
            dst->bitmap[i >> 3] |= (1 << (i & 7));     
        }
        memcpy(&dst->data[0] + start, &buf[0], number_of_bytes_to_write);
    }
    _written_bytes_per_call += number_of_bytes_to_write;
    if (_written_bytes_per_call == _write_bytes_per_call)
        _irm_transport_completed = true;
    
    return result;
}

bool irm_transport_image::is_buffer_free(){
    boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
    return _check_queue_size();
}

bool irm_transport_image::write_ex(__in uint64_t start, __in const std::string& buffer, __in uint32_t number_of_bytes_to_write, __in bool is_compressed){
    bool result = false;
    if (!buffer.length() || (start % IRM_IMAGE_SECTOR_SIZE != 0 || number_of_bytes_to_write % IRM_IMAGE_SECTOR_SIZE != 0) || start >= total_size)
    {
        LOG(LOG_LEVEL_ERROR, _T("Invalid arguments, write range: (%I64u, %u)"), start, number_of_bytes_to_write);
        return false;
    }
    boost::unique_lock<boost::mutex> lock(_write_lock);
    if (_terminated || _closed)
    {
        LOG(LOG_LEVEL_ERROR, _T("Invalid write call, write range: (%I64u, %u)"), start, number_of_bytes_to_write);
        return false;
    }
    if (IRM_IMAGE_TRANSPORT_MODE == mode){
        irm_transport_image_block block = irm_transport_image_block(start, 0, mode);
        block.dirty = true;
        block.image_block_name = get_block_name(_journal_db->next_block_index);
        block.length = number_of_bytes_to_write;
        block.compressed = is_compressed;
        block.p_data = &buffer;
        if (!(result = _flush_proc(&block))){
            LOG(LOG_LEVEL_ERROR, _T("Failed to write, write range: (%I64u, %u)"), start, number_of_bytes_to_write);
        }
        else if (_journal_db){
            _journal_db->next_block_index++;
            _update_journal_db();
        }
    }
    return result;
}

bool irm_transport_image::write(__in uint64_t start, __in const std::string& buf, __inout uint32_t& number_of_bytes_written){
    bool result = false;
    if ((start % IRM_IMAGE_SECTOR_SIZE != 0 || buf.length() % IRM_IMAGE_SECTOR_SIZE != 0) || start >= total_size)
    {
        number_of_bytes_written = 0;
        LOG(LOG_LEVEL_ERROR, _T("Invalid arguments, write range: (%I64u, %u)"), start, buf.length());
        return false;
    }

    if (_terminated || _closed)
    {
        LOG(LOG_LEVEL_ERROR, _T("Invalid write call, write range: (%I64u, %u)"), start, buf.length());
        return false;
    }

    if (IRM_IMAGE_TRANSPORT_MODE == mode){
        
        boost::unique_lock<boost::mutex> lock(_write_lock);

        irm_transport_image_block block = irm_transport_image_block(start, 0, mode);
        block.dirty = true;
        block.image_block_name = get_block_name(_journal_db->next_block_index);
        block.length = buf.length();
        if (compressed && !(result = _compress_proc(&block, buf))){
            LOG(LOG_LEVEL_ERROR, _T("Failed to compress, write range: (%I64u, %u)"), start, buf.length());
        }
        else if (!(result = _flush_proc(&block))){
            LOG(LOG_LEVEL_ERROR, _T("Failed to write, write range: (%I64u, %u)"), start, buf.length());
        }
        else if (_journal_db){
            _journal_db->next_block_index++;
            _update_journal_db();
        }
    }
    else{
        result = write(start, (const void *)buf.c_str(), buf.length(), number_of_bytes_written);
    }
    return result;
}

bool irm_transport_image::write(__in uint64_t start, __in const void *buffer, __in uint32_t number_of_bytes_to_write, __inout uint32_t& number_of_bytes_written)
{ 
    bool result = false;
    boost::unique_lock<boost::mutex> lock(_write_lock);
    uint64_t tmp_start = start;
    uint32_t tmp_number_of_bytes_to_write = number_of_bytes_to_write;
    uint32_t block_offset = 0;
    uint8_t *data = (uint8_t *)buffer;
    tmp_number_of_bytes_to_write = (start + number_of_bytes_to_write) > total_size ? (total_size - start) : number_of_bytes_to_write;
    _irm_transport_completed = false;
    _irm_transport_error = false;
    _write_bytes_per_call = tmp_number_of_bytes_to_write;
    boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
    if (!_check_queue_size()){
        invalid_operation       err;
        err.what_op = error_codes::SAASAME_E_QUEUE_FULL;
        err.why = "Queue buffer fulled.";
        throw err;
    }
    _process_blocks.insert(_process_blocks.begin(), _error_blocks.begin(), _error_blocks.end());
    if (_process_blocks.size())
    {
        _error_blocks.clear();
        _flush_cond.notify_all();
        _completion_cond.wait(buf_lock);
        _update_journal_db();
    }

    if (_irm_transport_error)
    {
        LOG(LOG_LEVEL_ERROR, _T("Invalid write call, write range: (%I64u, %u)"), start, number_of_bytes_to_write);
        return false;
    }

    for (size_t i = start / block_size; i < _number_of_blocks; i++)
    {
        uint64_t block_start = i * block_size;
        if (tmp_start >= block_start && tmp_start < block_start + block_size)
        {
            blocks[i >> 3] |= (1 << (i & 7));
            irm_transport_image_block::ptr block = NULL;
            {
                bool found = false;
                for (irm_transport_image_block::vtr::iterator p = _buffer_queue.begin(); p != _buffer_queue.end();)
                {
                    if ((*p)->start == block_start)
                    {
                        block = *p;
                        found = true;
                        p = _buffer_queue.erase(p);
                        break;
                    }
                    else{
                        ++p;
                    }
                }
                if (!found)
                {
                    try{
                        block = irm_transport_image_block::ptr(new irm_transport_image_block(block_start, block_size));
                        if (block)
                        {
                            block->index = i;
                            block->image_block_name = get_block_name((uint32_t)block->index);
                            _buffer_queue.push_back(block);
                        }
                        else
                        {
                            result = false;
                            LOG(LOG_LEVEL_ERROR, _T("Failed to allocate data block."));
                            break;
                        }
                    }
                    catch (const std::exception& ex){
                        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
                        result = false;
                        break;
                    }
                }
                else
                {
                    block->clock = GetTickCount64();
                    _buffer_queue.push_back(block);
                }
            }

            block_offset = tmp_start % block_size;

            if (tmp_start + tmp_number_of_bytes_to_write > block->start + block_size)
            {
                LOG(LOG_LEVEL_DEBUG, _T("Write: split to segment (%I64u/%u, %u)"), block->start, block_offset, block_size - block_offset);

                result = _write(block_offset, block_size - block_offset, data + (number_of_bytes_to_write - tmp_number_of_bytes_to_write), block, block->image_block_name, false);
                if (result)
                {
                    tmp_start = block->start + block_size;
                    tmp_number_of_bytes_to_write -= (block_size - block_offset);
                }
            }
            else
            {
                LOG(LOG_LEVEL_DEBUG, _T("Write: segment (%I64u/%u, %u) is last chunk."), block->start, block_offset, tmp_number_of_bytes_to_write);
                result = _write(block_offset, tmp_number_of_bytes_to_write, data + (number_of_bytes_to_write - tmp_number_of_bytes_to_write), block, block->image_block_name, true);
                if (result)
                    break;
            }
        }
    }

    if (result && !_irm_transport_error)
    {
        while (_number_of_buffer_blocks < _buffer_queue.size())
        {
            irm_transport_image_block::vtr::iterator it = _buffer_queue.begin();
            _process_blocks.push_back((*it));
            _buffer_queue.erase(it);
        }
        if (_process_blocks.size())
        {
            _flush_cond.notify_all();
            _completion_cond.wait(buf_lock);
            result = !_irm_transport_error;
            _update_journal_db();
            _update_meta_file();
        }
        if (result && !_irm_transport_error)
        {
            number_of_bytes_written = _written_bytes_per_call;
            _total_written_bytes += _written_bytes_per_call;
        }
    }
    _write_bytes_per_call = _written_bytes_per_call = 0;

    return result;
}

bool irm_transport_image::_flush_proc(__in irm_transport_image_block* block)
{
    bool result = true;
    {
        try
        {
            if (IRM_IMAGE_TRANSPORT_MODE == mode)
            {
                if (result = _op->write_file(block->image_block_name, *block))
                {
                    block->flags |= irm_transport_image_block::block_flag::BLOCK_FLUSHED;
                    LOG(LOG_LEVEL_INFO, "Block file(%s) write out completed and no error.", block->image_block_name.c_str());
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, _T("Internal write failure (%s: %I64u, %u)."), block->image_block_name.c_str(), block->start, block->length);
                }          
            }
            else if (_local_flush)
            {               
                if (result = _op->write_temp_file(IRM_IMAGE_LOCAL_DB_DIR / base_name / block->image_block_name, *block))
                {
                    block->flags |= irm_transport_image_block::block_flag::BLOCK_FLUSHED;
                    LOG(LOG_LEVEL_INFO, "Block file(%s) write out completed and no error.", block->image_block_name.c_str());
                }
                else
                {
                    LOG(LOG_LEVEL_ERROR, _T("Internal write local temp failure (%s: %I64u, %u)."), block->image_block_name.c_str(), block->start, block->length);
                }
            }
            else 
            {
                try
                {
                    macho::windows::lock_able::vtr locks_ptr = _op->get_lock_objects(block->image_block_name);
                    macho::windows::auto_locks block_file_locks(locks_ptr);

                    if (checksum)
                    {
                        block->duplicated = _duplicate_verify(block->start / block_size, block->bitmap, block->crc, block->md5, sizeof(block->md5));
                    }
              
                    if (block->duplicated)
                    {
                        std::wstring tmp_block_name = block->image_block_name + IRM_IMAGE_DEDUPE_TMP_EXT;
                        if (result = _op->write_file(tmp_block_name, *(block), true))
                        {
                            LOG(LOG_LEVEL_INFO, "Block file(%s) write out completed and no error.", tmp_block_name.c_str());
                        }
                        else
                        {
                            LOG(LOG_LEVEL_ERROR, _T("Internal write failure (%s: %I64u, %u)."), tmp_block_name.c_str(), block->start, block->length);
                        }
                    }

                    if (result)
                    {
                        if (result = _op->write_file(block->image_block_name, *block))
                        {
                            block->flags |= irm_transport_image_block::block_flag::BLOCK_FLUSHED;
                            LOG(LOG_LEVEL_INFO, "Block file(%s) write out completed and no error.", block->image_block_name.c_str());
                        }
                        else
                        {
                            LOG(LOG_LEVEL_ERROR, _T("Internal write failure (%s: %I64u, %u)."), block->image_block_name.c_str(), block->start, block->length);
                        }
                    }
                } 
                catch (macho::windows::lock_able::exception &ex){
                    result = false;
                    LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                }
            }
        }
        catch (boost::archive::archive_exception& ex)
        {
            result = false;
            _irm_transport_error = true;
        }
    }
    return result;
}

bool irm_transport_image::_flush_proc()
{
    //FUN_TRACE;
    bool result = true;

    while (!_terminated)
    {
        irm_transport_image_block::ptr block = NULL;
        {
            boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);           
            while (!_terminated && _process_blocks.size() == 0)
                _flush_cond.wait(buf_lock);
            if (_process_blocks.size() > 0)
            {
                block = _process_blocks.front();
                _working_blocks[block->image_block_name] = block;
                _process_blocks.pop_front();
            }
        }
        if (!_terminated && block)
        {
            bool need_flush = !(block->flags & irm_transport_image_block::block_flag::BLOCK_FLUSHED);
            if (!(result = _merge_proc(block.get())))
            {
                boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
                _error_blocks.push_back(block);
                _irm_transport_error = true;  
            }
            else if (checksum && !(result = _crc_md5_proc(block.get())))
            {
                boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
                _error_blocks.push_back(block);
                _irm_transport_error = true;
            }
            else if (compressed && !(result = _compress_proc(block.get())))
            {
                boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
                _error_blocks.push_back(block);
                _irm_transport_error = true;
            }
            else if (need_flush && !(result = _flush_proc(block.get())))
            {
                boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
                _error_blocks.push_back(block);
                _irm_transport_error = true;
            }
            {
                boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
                _working_blocks.erase(block->image_block_name);
                
                if ((!_local_flush) && need_flush && (block->flags & irm_transport_image_block::block_flag::BLOCK_FLUSHED))
                {
                    if (_journal_db)
                    {
                        _journal_db->dirty_blocks_list.push_back(block->index);
                    }
                }
                if ( _irm_transport_error || (_working_blocks.size() == 0 && _process_blocks.size() == 0))
                {
                    _completion_cond.notify_one();
                }
            }
        }        
    }
    _flush_cond.notify_all();
    return result;
}

bool irm_transport_image::_check_queue_size(){
    bool result = true;
    if (queue_size > 0 && _op){
        if (_journal_db && _journal_db->dirty_blocks_list.size() >= queue_size){
            std::wstring image_block_name = get_block_name(_journal_db->dirty_blocks_list[_journal_db->dirty_blocks_list.size() - queue_size]);
            result = !_op->is_file_existing(image_block_name);
            if (!result){
                LOG(LOG_LEVEL_WARNING, _T("Not enough queue in the target path. Block file(%s) exists."), image_block_name.c_str());
            }
        }
    }
    return result;
}

bool irm_transport_image::_compress_proc(__in irm_transport_image_block* block)
{
    bool result = true;
    try
    {
        if ((!(block->flags & irm_transport_image_block::block_flag::BLOCK_COMPRESSED)) && block->compressed != compressed)
        {
            std::vector<uint8_t> compressed_buf;
            uint32_t compressed_length;
            compressed_buf.resize(block_size);

            compressed_length = _compress(reinterpret_cast<char*>(block->data.data()), reinterpret_cast<char*>(compressed_buf.data()), block->length, block_size);
            if (compressed_length != 0)
            {
                if (compressed_length < block->length)
                {
                    block->data = std::move(compressed_buf);
                    block->data.resize(compressed_length);
                    block->compressed = true;
                }
            }
            block->flags |= irm_transport_image_block::block_flag::BLOCK_COMPRESSED;
        }
    }
    catch (const std::out_of_range& ex)
    {
        result = false;
        _irm_transport_error = true;
    }
    return result;
}

bool irm_transport_image::_compress_proc(__in irm_transport_image_block* block, __in const std::string& buffer)
{
    bool result = true;
    try
    {
        if ((!(block->flags & irm_transport_image_block::block_flag::BLOCK_COMPRESSED)) && block->compressed != compressed)
        {
            std::vector<uint8_t> compressed_buf;
            uint32_t compressed_length;
            compressed_buf.resize(buffer.length());

            compressed_length = _compress(reinterpret_cast<const char*>(buffer.c_str()), reinterpret_cast<char*>(compressed_buf.data()), block->length, buffer.length());
            if (compressed_length != 0)
            {
                if (compressed_length < buffer.length())
                {
                    block->data = std::move(compressed_buf);
                    block->data.resize(compressed_length);
                    block->compressed = true;
                }
            }
            if (!block->compressed){
                block->p_data = &buffer;
            }
            block->flags |= irm_transport_image_block::block_flag::BLOCK_COMPRESSED;
        }
    }
    catch (const std::out_of_range& ex)
    {
        result = false;
        _irm_transport_error = true;
    }
    return result;
}

bool irm_transport_image::_crc_md5_proc(__in irm_transport_image_block* block)
{
    bool result = true;
    boost::crc_32_type crc_processor;
    try
    {
        if (!_local_flush && (!(block->flags & irm_transport_image_block::block_flag::BLOCK_CRC_MD5)))
        {
            std::stringstream buf(std::ios_base::binary | std::ios_base::out | std::ios_base::in);

            //reset crc&md5 before calculate crc&md5
            block->crc = 0;
            memset(block->md5, 0, sizeof(block->md5));

            //crc32
            block->save(buf, true);
            uint32_t crc32 = 0;
            std::string _buf = buf.str();
            irm_transport_image_block::calc_crc32_val(_buf, crc32);

            //md5
            std::string digest;
            irm_transport_image_block::calc_md5_val(_buf, digest);

            block->crc = crc32;
            if (!digest.empty())
            {
                block->flags |= irm_transport_image_block::block_flag::BLOCK_CRC_MD5;
                memcpy(block->md5, digest.c_str(), sizeof(block->md5));
            }
            else
            {
                LOG(LOG_LEVEL_ERROR, _T("Failed to calculate md5 value for block(%s)."), block->image_block_name.c_str());
            }
        }
    }
    catch (boost::archive::archive_exception& ex)
    {
        result = false;
        _irm_transport_error = true;
    }
    return result;
}

bool irm_transport_image::_merge_proc(__in irm_transport_image_block* block)
{
    bool result = true;
    std::wstring block_file = block->image_block_name;
    irm_transport_image_block::ptr ar_block;
    try
    {
        if (!_local_flush && (!(block->flags & irm_transport_image_block::block_flag::BLOCK_MERGE)) && _op->is_file_existing(block_file)) //resume
        {
            try
            {
                if (_op->is_file_existing(block_file + IRM_IMAGE_DEDUPE_TMP_EXT))
                    block_file = block_file + IRM_IMAGE_DEDUPE_TMP_EXT;
                macho::windows::lock_able::vtr locks_ptr = _op->get_lock_objects(block->image_block_name);
                macho::windows::auto_locks block_file_locks(locks_ptr);
                ar_block = irm_transport_image_block::ptr(new irm_transport_image_block(0, block_size));
                result = false;
                if (ar_block && (result = _op->read_file(block_file, *ar_block.get())))
                {
                    if (block->start == ar_block->start)
                    {
                        ar_block->image_block_name = block->image_block_name;
                        if (ar_block->compressed)
                        {
                            uint32_t decompressed_length = block_size;
                            std::vector<uint8_t> decompressed_buf;
                            decompressed_buf.resize(block_size);
                            decompressed_length = _decompress((const char *)&ar_block->data[0], (char *)&decompressed_buf[0], ar_block->data.size(), block_size);
                            ar_block->data = std::move(decompressed_buf);
                            ar_block->length = decompressed_length;
                            ar_block->compressed = false;
                        }

                        LOG(LOG_LEVEL_WARNING, _T("Merging into the block (%s), (start: %I64u, %u)"),
                            block->image_block_name.c_str(), block->start, block->length);
                        int32_t start_bit = 0;
                        int32_t end_bit = block->length / IRM_IMAGE_SECTOR_SIZE;
                        for (int32_t bit = start_bit; bit < end_bit; bit++)
                        {
                            if ((ar_block->bitmap[bit >> 3] & (1 << (bit & 7))) && (!(block->bitmap[bit >> 3] & (1 << (bit & 7)))))
                            {
                                block->bitmap[bit >> 3] |= (1 << (bit & 7));
                                memcpy(&block->data[0] + (bit * IRM_IMAGE_SECTOR_SIZE), &ar_block->data[0] + (bit * IRM_IMAGE_SECTOR_SIZE), IRM_IMAGE_SECTOR_SIZE);
                            }
                        }
                        block->flags |= irm_transport_image_block::block_flag::BLOCK_MERGE;
                    }
                    else
                    {
                        _irm_transport_error = true;
                    }
                    if (ar_block->duplicated)
                        _op->remove_file(block_file);
                }
                else
                {
                    _irm_transport_error = true;
                }
            }
            catch (macho::windows::lock_able::exception &ex){
                result = false;
                LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
            }
        }
    }
    catch (macho::exception_base& ex){
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(ex).c_str());
        result = false;
        _irm_transport_error = true;
    }
    catch (const boost::filesystem::filesystem_error& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        result = false;
        _irm_transport_error = true;
    }
    catch (const boost::exception &ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
        result = false;
        _irm_transport_error = true;
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
        result = false;
        _irm_transport_error = true;
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
        _irm_transport_error = true;
    }
    if (_irm_transport_error)
        result = false;
    return result;
}

bool irm_transport_image::flush(bool is_terminated)
{
    //FUN_TRACE;
    bool result = true;
    LOG(LOG_LEVEL_DEBUG, _T("Flush event triggering..."));
    {
        if (is_terminated && _op)
            _op->cancel();
        boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
        _process_blocks.insert(_process_blocks.begin(), _error_blocks.begin(), _error_blocks.end());
        _irm_transport_error = false;
        _local_flush = is_terminated;
        while (_buffer_queue.size())
        {
            irm_transport_image_block::vtr::iterator it = _buffer_queue.begin();
            _process_blocks.push_back((*it));
            _buffer_queue.erase(it);
        }
        if (_process_blocks.size())
        {
            _flush_cond.notify_all();
            _completion_cond.wait(buf_lock);
            good = result = !_irm_transport_error;
            _update_journal_db();
        }
    }
   
    LOG(LOG_LEVEL_DEBUG, _T("flush_proc worker completed."));

    if (is_terminated)
    {
        _update_meta_file();
        _terminate();
    }
    return result;
}

int irm_transport_image::_compress(__in const char* source, __inout char* dest, __in int source_size, __in int max_compressed_size)
{
    return LZ4_compress_fast(source, dest, source_size, max_compressed_size, 1);
}

int irm_transport_image::_decompress(__in const char* source, __inout char* dest, __in int compressed_size, __in int max_decompressed_size)
{
    return LZ4_decompress_safe(source, dest, compressed_size, max_decompressed_size);
}

// Need to add compare bitmap logic to avoid some logic error for duplicate verify
bool irm_transport_image::_duplicate_verify(__in const uint64_t& block_index, __in const std::vector<uint8_t> &bitmap, __in const uint32_t& crc, __in const uint8_t *md5, __in const int size)
{
    bool duplicate = false;
    try{
        if (_base_checksum_db && _checksum_db){
            chechsum::ptr chk = _base_checksum_db->get(block_index);
            if (chk && chk->validated){
                duplicate = chk->bitmap.length() == bitmap.size() && 0 == memcmp(chk->bitmap.c_str(), &bitmap.at(0), bitmap.size()) && chk->crc == crc && 0 == memcmp(chk->md5.c_str(), md5, size);
            }

            if (!duplicate){
                chechsum::ptr new_chk = _checksum_db->get(block_index);
                if ((!chk) && new_chk){
                    if (!(duplicate = chk->bitmap.length() == bitmap.size() && 0 == memcmp(chk->bitmap.c_str(), &bitmap.at(0), bitmap.size()) && new_chk->crc == crc && 0 == memcmp(new_chk->md5.c_str(), md5, size)))
                        new_chk->validated = false;
                }
                else{
                    new_chk = chechsum::ptr(new chechsum());
                    new_chk->index = block_index;
                    new_chk->bitmap = std::string(reinterpret_cast<char const*>(&bitmap.at(0)), bitmap.size());
                    new_chk->crc = crc;
                    new_chk->validated = true;
                    new_chk->md5 = std::string(reinterpret_cast<char const*>(md5), size);
                }
                _checksum_db->put(*new_chk.get());
            }
        }
    }
    catch (macho::exception_base& ex){
        LOG(LOG_LEVEL_ERROR, macho::get_diagnostic_information(ex).c_str());
    }
    catch (const boost::filesystem::filesystem_error& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
    }
    catch (const boost::exception &ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(boost::exception_detail::get_diagnostic_information(ex, "error:")).c_str());
    }
    catch (const std::exception& ex){
        LOG(LOG_LEVEL_ERROR, macho::stringutils::convert_utf8_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }

    if (!checksum_verify)
        duplicate = false;

    return duplicate;
}

bool irm_transport_image::_flush_metafiles()
{
    bool result = true;

    if (_op)
    {
        std::vector<boost::filesystem::path> files;
        std::wstring lck_file = base_name + L".lck";
        files.push_back(IRM_IMAGE_LOCAL_DB_DIR / base_name / name);
        files.push_back(boost::filesystem::path(IRM_IMAGE_LOCAL_DB_DIR / base_name / name).wstring() + (IRM_IMAGE_JOURNAL_EXT));

        result = _op->flush_metafiles(files);
    }

    return result;
}

void irm_transport_image::_terminate()
{
    if (!_flush_workers.size())
        return;

    if (!_terminated)
    {
        LOG(LOG_LEVEL_RECORD, L"Ending thread pools...");
        {
            boost::unique_lock<boost::mutex> buf_lock(_access_buffer_queue);
            _terminated = true;
            _flush_cond.notify_all();
        }
        _flush_workers.join_all();
        LOG(LOG_LEVEL_RECORD, L"Thread pools release completed.");
    }
}

bool irm_transport_image_blks_chg_journal::load(std::istream& inbuf)
{
    uint32_t len = 0;
    using namespace macho;
    uint32_t buff_size = TEMP_BUFFER_SIZE;
    try
    {
        std::auto_ptr<wchar_t> buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        inbuf.exceptions(std::istream::failbit | std::istream::badbit);
        inbuf.seekg(0, std::ios::beg);
        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        name = buff.get();

        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        comment = buff.get();

        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        parent_path = buff.get();

        inbuf.read((char *)&len, sizeof(len));
        if (len > buff_size){
            buff_size = len + sizeof(wchar_t);
            buff = std::auto_ptr<wchar_t>((wchar_t*)new char[buff_size]);
        }
        memset(buff.get(), 0, buff_size);
        inbuf.read((char *)buff.get(), len);
        extension = buff.get();
        if (is_version_2()){
            inbuf.read((char *)&next_block_index, sizeof(next_block_index));
        }
        else{
            inbuf.read((char *)&len, sizeof(len));
            if (len){
                dirty_blocks_list.resize(len);
                inbuf.read((char *)&dirty_blocks_list[0], len * sizeof(uint32_t));
            }
        }
        inbuf.seekg(0, std::ios::beg);
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

bool irm_transport_image_blks_chg_journal::save(std::ostream& outbuf, bool forcefull)
{
    uint32_t len = 0;
    using namespace macho;

    try
    {
        outbuf.exceptions(std::ostream::failbit | std::ostream::badbit);
        //outbuf.swap(std::stringstream());
        outbuf.seekp(0, std::ios::beg);

        len = name.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)name.c_str(), len);

        len = comment.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)comment.c_str(), len);

        len = parent_path.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)parent_path.c_str(), len);

        len = extension.length() * sizeof(wchar_t);
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)extension.c_str(), len);
        if (is_version_2()){
            outbuf.write((char *)&next_block_index, sizeof(next_block_index));
        }
        else{
            len = dirty_blocks_list.size();
            outbuf.write((char *)&len, sizeof(len));
            if (len){
                outbuf.write((char *)&dirty_blocks_list[0], len * sizeof(uint32_t));
            }
        }
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

irm_transport_image_block::irm_transport_image_block(uint64_t _start, uint32_t _length, uint32_t _mode)
    : index(0), flags(BLOCK_READY), mode(_mode), crc(0), start(_start), length(_length), duplicated(false), compressed(false), dirty(false), p_data(NULL)
{
    if (length){
        if (mode == IRM_IMAGE_TRANSPORT_MODE){
            bitmap.resize(1);
            bitmap[0] = 0x0;
        }
        else{
            uint32_t len = ((length - 1) / 4096) + 1;
            bitmap.resize(len);
            memset(&bitmap[0], 0, len);
        }
        data.resize(length);
        memset(md5, 0, sizeof(md5));
        memset(&data[0], 0, length);
    }
    else if (mode == IRM_IMAGE_TRANSPORT_MODE){
        bitmap.resize(1);
        bitmap[0] = 0x0;
    }
    clock = GetTickCount64();
}

bool irm_transport_image_block::save(std::ostream& outbuf, bool forcefull)
{
    uint32_t len = 0;
    using namespace macho;
    try
    {
        outbuf.exceptions(std::ostream::failbit | std::ostream::badbit);
        outbuf.seekp(0, std::ios::beg);
        outbuf.write((char *)&mode, sizeof(mode));
        outbuf.write((char *)md5, sizeof(md5));
        outbuf.write((char *)&crc, sizeof(crc));
        outbuf.write((char *)&start, sizeof(start));
        outbuf.write((char *)&length, sizeof(length));
        outbuf.write((char *)&duplicated, sizeof(duplicated));
        outbuf.write((char *)&compressed, sizeof(compressed));
        outbuf.write((char *)&dirty, sizeof(dirty));
        len = bitmap.size();
        outbuf.write((char *)&len, sizeof(len));
        outbuf.write((char *)&bitmap[0], bitmap.size());
        if (!duplicated || forcefull){
            if (p_data){
                len = p_data->length();
                outbuf.write((char *)&len, sizeof(len));
                outbuf.write(p_data->c_str(), len);
            }
            else{
                len = data.size();
                outbuf.write((char *)&len, sizeof(len));
                outbuf.write((char *)&data[0], data.size());
            }
        }
        else{
            len = 0;
            outbuf.write((char *)&len, sizeof(len));
        }
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

bool irm_transport_image_block::load_header(std::istream& inbuf) // exclude data field
{
    uint32_t len = 0;
    using namespace macho;
    try
    {
        inbuf.exceptions(std::istream::failbit | std::istream::badbit);
        inbuf.seekg(0, std::ios::beg);
        inbuf.read((char *)&mode, sizeof(mode));
        inbuf.read((char *)md5, sizeof(md5));
        inbuf.read((char *)&crc, sizeof(crc));
        inbuf.read((char *)&start, sizeof(start));
        inbuf.read((char *)&length, sizeof(length));
        inbuf.read((char *)&duplicated, sizeof(duplicated));
        inbuf.read((char *)&compressed, sizeof(compressed));
        inbuf.read((char *)&dirty, sizeof(dirty));
        inbuf.read((char *)&len, sizeof(len));
        bitmap.resize(len);
        inbuf.read((char *)&bitmap[0], len);

        inbuf.seekg(0, std::ios::beg);
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

bool irm_transport_image_block::load(std::istream& inbuf)
{
    uint32_t len = 0;
    using namespace macho;
    try
    {
        inbuf.exceptions(std::istream::failbit | std::istream::badbit);
        inbuf.seekg(0, std::ios::beg);

        inbuf.read((char *)&mode, sizeof(mode));
        inbuf.read((char *)md5, sizeof(md5));
        inbuf.read((char *)&crc, sizeof(crc));
        inbuf.read((char *)&start, sizeof(start));
        inbuf.read((char *)&length, sizeof(length));
        inbuf.read((char *)&duplicated, sizeof(duplicated));
        inbuf.read((char *)&compressed, sizeof(compressed));
        inbuf.read((char *)&dirty, sizeof(dirty));
        inbuf.read((char *)&len, sizeof(len));
        bitmap.resize(len);
        inbuf.read((char *)&bitmap[0], len);
        inbuf.read((char *)&len, sizeof(len));
        if (len){
            data.resize(len);
            inbuf.read((char *)&data[0], len);
        }
        inbuf.seekg(0, std::ios::beg);
        return true;
    }
    catch (std::exception &ex){
        LOG(LOG_LEVEL_ERROR, L"std::exception: %s", macho::stringutils::convert_ansi_to_unicode(ex.what()).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown Exceptioin");
    }
    return false;
}

void irm_transport_image_block::calc_md5_val(const std::string& src, std::string& digest)
{
    MD5 md5(src);
    md5.finalize();
    digest = md5.checksum();
}

void irm_transport_image_block::calc_crc32_val(const std::string& src, uint32_t& crc32)
{
    crc32 = macho::crc32::calculate(src.c_str(), src.length());
}

void irm_transport_image_blks_chg_journal::copy(const irm_transport_image_blks_chg_journal& journal){
    name = journal.name;
    comment = journal.comment;
    parent_path = journal.parent_path;
    extension = journal.extension;
    dirty_blocks_list = journal.dirty_blocks_list;
    next_block_index = journal.next_block_index;
}

const irm_transport_image_blks_chg_journal& irm_transport_image_blks_chg_journal::operator =(const irm_transport_image_blks_chg_journal& journal){
    if (this != &journal)
        copy(journal);
    return(*this);
}
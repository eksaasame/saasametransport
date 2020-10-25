#pragma once
#ifndef __MACHO_ARCHIVE__
#define __MACHO_ARCHIVE__

#include "boost\filesystem.hpp"
#include "boost\shared_ptr.hpp"
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>

namespace macho{

namespace archive{

class zip{
public:
    typedef boost::shared_ptr<zip> ptr;
    static zip::ptr open(boost::filesystem::path _file);
    static zip::ptr open(std::iostream& data_stream);
    virtual bool add(boost::filesystem::path _path) = 0;
	virtual bool add_dir(boost::filesystem::path _path) = 0;
    virtual bool add(std::string name_in_arcive, const std::string& data) = 0;
    virtual bool close() = 0;
};

class unzip{
public:
    typedef boost::shared_ptr<unzip> ptr;
    bool close();
    static unzip::ptr open(boost::filesystem::path _file, size_t offset = 0);
    static unzip::ptr open(std::iostream& data_stream, size_t offset = 0);
    virtual bool file_exists(const std::string& filename) = 0;
    virtual bool decompress_signal_file(const std::string& filename, const std::string& output) = 0;
    virtual bool decompress_archive(const boost::filesystem::path& directory, std::vector<boost::filesystem::path>& extracted_files = std::vector<boost::filesystem::path>()) = 0;
};

#ifndef MACHO_HEADER_ONLY

#include "miniz.c"
#include "tracelog.hpp"
#include "exception_base.hpp"

#define MZ_MIN(a,b) (((a)<(b))?(a):(b))
class zip_base : virtual public zip{
public:

    virtual ~zip_base(){
        cleanup();
        if (_file.get() != NULL)
            _file->close();
    };

    virtual bool add(boost::filesystem::path _path){
        return add(_path, "");
    }
	virtual bool add_dir(boost::filesystem::path _path){
		bool result = false;
		if (boost::filesystem::is_directory(_path)){
			boost::filesystem::directory_iterator end_iter;
			for (boost::filesystem::directory_iterator dir_iter(_path); dir_iter != end_iter; ++dir_iter){
				if (!(result = add(dir_iter->path())))
					break;
			}
		}
		return result;
	}
    virtual bool add(std::string name_in_arcive, const std::string& data);
    virtual bool close();
private:
    friend class zip;
    struct  exception : virtual public macho::exception_base {};
    bool init();
    bool _add(boost::filesystem::path file_add_to_zip, boost::filesystem::path name_in_archive, bool is_dir = false);
    bool add(boost::filesystem::path _path, boost::filesystem::path name_in_archive);
    zip_base(std::iostream &data_stream);
    static size_t _zip_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n);
    static size_t _zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n);
    std::iostream	&_data_stream;
    mz_zip_archive  _archive;
    void cleanup();
    std::shared_ptr<std::fstream> _file;
    bool _is_open;
};
class unzip_base : virtual public unzip{
public:
    virtual ~unzip_base(){
        cleanup();
        if (_file.get() != NULL)
            _file->close();
    };
    virtual bool close();
    static unzip::ptr open(boost::filesystem::path _file);
    static unzip::ptr open(std::iostream& data_stream);
    virtual bool file_exists(const std::string& filename);
    virtual bool decompress_signal_file(const std::string& filename, const std::string& output);
    virtual bool decompress_archive(const boost::filesystem::path& directory, std::vector<boost::filesystem::path>& extracted_files = std::vector<boost::filesystem::path>());
private:
    friend class unzip;
    struct  exception : virtual public macho::exception_base {};
    bool init();
    unzip_base(std::iostream &data_stream, size_t offset);
    static size_t _zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n);
    std::iostream	&_data_stream;
    mz_zip_archive  _archive;
    void cleanup();
    std::shared_ptr<std::fstream> _file;
    bool _is_open;
    size_t  _offset;
};

#endif

};

#ifndef MACHO_HEADER_ONLY

using namespace macho;

archive::zip_base::zip_base(std::iostream &data_stream) : _data_stream(data_stream){
    if (!(_is_open = init())){
        BOOST_THROW_EXCEPTION_BASE_STRING(zip_base::exception, boost::str(boost::wformat(L"Failed to initialize archive.")));
    }
}

bool archive::zip_base::close(){
    try{
        cleanup();
        return true;
    }
    catch (archive::zip_base::exception &e){
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return false;
}

bool archive::zip_base::init(){
    MZ_CLEAR_OBJ(_archive);
    _archive.m_pRead = _zip_file_read_func;
    _archive.m_pWrite = _zip_file_write_func;
    _archive.m_pIO_opaque = this;
    _data_stream.seekg((size_t)0, std::iostream::end);
    _archive.m_archive_size = _data_stream.tellg();
    _data_stream.seekg((size_t)0, std::iostream::beg);
    if (_archive.m_archive_size){
        if (!mz_zip_reader_init(&_archive, _archive.m_archive_size, 0)){
            return false;
        }
        if (_archive.m_zip_mode != MZ_ZIP_MODE_READING)
            return false;
        if ((_archive.m_total_files == 0xFFFF) || ((_archive.m_archive_size + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_ZIP_LOCAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
            return false;
        _archive.m_archive_size = _archive.m_central_directory_file_ofs;
        _archive.m_zip_mode = MZ_ZIP_MODE_WRITING;
        _archive.m_central_directory_file_ofs = 0;
    }
    else{
        if (!mz_zip_writer_init(&_archive, 0))
            return false;
    }
    return true;
}

void archive::zip_base::cleanup(){
    bool status = true;
    if (_is_open){
        if (!mz_zip_writer_finalize_archive(&_archive))
            status = false;
        if (!mz_zip_writer_end(&_archive))
            status = false;
        MZ_CLEAR_OBJ(_archive);
        _is_open = false;
        if (!status)
            BOOST_THROW_EXCEPTION_BASE_STRING(zip_base::exception, boost::str(boost::wformat(L"Failed to finalize archive .")));
    }
}

archive::zip::ptr archive::zip::open(boost::filesystem::path _file){
    zip::ptr zip_ptr;
    try{
        if (!boost::filesystem::exists(_file)){
            std::ofstream{ _file.string() };
        }
        std::shared_ptr<std::fstream> _file(new std::fstream(_file.string(), std::ios::out | std::ios::in | std::ios::binary));
        if (_file->is_open()){
            zip_ptr = zip::ptr(new zip_base(*_file.get()));
            dynamic_cast<zip_base*>(zip_ptr.get())->_file = _file;
        }
    }
    catch (archive::zip_base::exception &e){
        zip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        zip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return zip_ptr;

}

archive::zip::ptr archive::zip::open(std::iostream& data_stream){
    zip::ptr zip_ptr;
    try{
        zip_ptr = zip::ptr(new zip_base(data_stream));
    }
    catch (archive::zip_base::exception &e){
        zip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        zip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return zip_ptr;
}

size_t archive::zip_base::_zip_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n){
    archive::zip_base* archive = (archive::zip_base*)pOpaque;
    size_t size = 0;
    if (((mz_int64)file_ofs < 0))
        return 0;
    try{
        archive->_data_stream.seekp((size_t)file_ofs, std::iostream::beg);
        archive->_data_stream.write((char*)pBuf, n);
        size = (size_t)archive->_data_stream.tellp() - file_ofs;
    }
    catch (...){
        size = 0;
    }
    return size;
}

size_t archive::zip_base::_zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n){
    archive::zip_base* archive = (archive::zip_base*)pOpaque;
    size_t size = 0;
    if (((mz_int64)file_ofs < 0))
        return 0;
    try{
        archive->_data_stream.seekg((size_t)file_ofs, std::iostream::beg);
        archive->_data_stream.read((char *)pBuf, n);
        size = (size_t)archive->_data_stream.tellg() - file_ofs;
    }
    catch (...){
        size = 0;
    }
    return size;
}

bool archive::zip_base::_add(boost::filesystem::path path_add_to_zip, boost::filesystem::path name_in_archive, bool is_dir){
    bool status = false;
    if (!boost::filesystem::exists(path_add_to_zip))
        return status;
    if (is_dir)
		status = MZ_TRUE == mz_zip_writer_add_mem_ex(&_archive, name_in_archive.string().c_str(), NULL, 0, NULL, 0, MZ_DEFAULT_COMPRESSION, 0, 0);
    else if (boost::filesystem::is_regular_file(path_add_to_zip))
		status = MZ_TRUE == mz_zip_writer_add_file(&_archive, name_in_archive.string().c_str(), path_add_to_zip.string().c_str(), NULL, NULL, MZ_DEFAULT_COMPRESSION);
    else{
        LOG(LOG_LEVEL_ERROR, _T("only support adding folder and file"));
        status = false;
    }
    return status;
}

bool archive::zip_base::add(std::string name_in_arcive, const std::string& data){
    bool status = true;
    if (!mz_zip_writer_validate_archive_name(name_in_arcive.c_str()) || !_is_open)
        return false;
    if (!mz_zip_writer_add_mem(&_archive, name_in_arcive.c_str(), data.c_str(), data.length(), MZ_DEFAULT_COMPRESSION))
        status = false;
    return status;
}

bool archive::zip_base::add(boost::filesystem::path _path, boost::filesystem::path name_in_archive){
    boost::filesystem::directory_iterator end_iter;
    bool status = false;
    if (!boost::filesystem::exists(_path) || !_is_open)
        return status;
    else if (boost::filesystem::is_directory(_path)){
        boost::filesystem::path temp_path;
        temp_path = (name_in_archive.empty() ? _T("") : name_in_archive) / _path.filename();
        if (_add(_path, (temp_path.string() + '/'), true))
            status = true;
        for (boost::filesystem::directory_iterator dir_iter(_path); dir_iter != end_iter; ++dir_iter){
            if (add(dir_iter->path(), (temp_path.string() + '/')))
                status = true;
        }
    }
    else if (boost::filesystem::is_regular_file(_path)){
        if (name_in_archive.empty())
            name_in_archive = _path.filename();
        else
            name_in_archive /= _path.filename();
        if (_add(_path, name_in_archive))
            status = true;
    }
    return status;
}

archive::unzip::ptr archive::unzip::open(boost::filesystem::path _file, size_t offset){
    unzip::ptr unzip_ptr;
    try{
        if (boost::filesystem::exists(_file)){
            std::shared_ptr<std::fstream> _file(new std::fstream(_file.string(), std::ios::in | std::ios::binary));
            if (_file->is_open()){
                unzip_ptr = unzip::ptr(new unzip_base(*_file.get(), offset));
                dynamic_cast<unzip_base*>(unzip_ptr.get())->_file = _file;
            }
        }
    }
    catch (archive::unzip_base::exception &e){
        unzip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        unzip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return unzip_ptr;

}

archive::unzip::ptr archive::unzip::open(std::iostream& data_stream, size_t offset){
    unzip::ptr unzip_ptr;
    try{
        unzip_ptr = unzip::ptr(new unzip_base(data_stream, offset));
    }
    catch (archive::unzip_base::exception &e){
        unzip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        unzip_ptr = NULL;
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return unzip_ptr;
}

size_t archive::unzip_base::_zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n){
    archive::unzip_base* archive = (archive::unzip_base*)pOpaque;
    size_t size = 0;
    if (((mz_int64)file_ofs < 0))
        return 0;
    try{
        archive->_data_stream.seekg((size_t)(archive->_offset + file_ofs), std::iostream::beg);
        archive->_data_stream.read((char *)pBuf, n);
        size = (size_t)archive->_data_stream.tellg() - (file_ofs + archive->_offset);
    }
    catch (...){
        size = 0;
    }
    return size;
}

bool archive::unzip_base::file_exists(const std::string& filename){
    int file_index = mz_zip_reader_locate_file(&_archive, filename.c_str(), NULL, 0);
    return (file_index >= 0);
}

bool archive::unzip_base::decompress_signal_file(const std::string& filename, const std::string& output){
    boost::filesystem::path filepath = boost::filesystem::path(output);
    boost::filesystem::path parent_path = filepath.parent_path();
    if (!boost::filesystem::exists(parent_path)){
        if (!boost::filesystem::create_directories(filepath.parent_path())) {
            LOG(LOG_LEVEL_ERROR, _T("DecompressArchive could not create directory."));
            return false;
        }
    }
    boost::filesystem::remove(output);
    if (!mz_zip_reader_extract_file_to_file(&_archive, filename.c_str(), output.c_str(), 0)) {
        LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_extract_file_to_file() failed!"));
        return false;
    }
    return true;
}

bool archive::unzip_base::decompress_archive(const boost::filesystem::path& directory, std::vector<boost::filesystem::path>& extracted_files){
    for (unsigned int i = 0; i < mz_zip_reader_get_num_files(&_archive); i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&_archive, i, &file_stat)) {
            std::cout << "mz_zip_reader_file_stat() failed!\n";
            LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_file_stat() failed!"));
            return false;
        }

        if (!mz_zip_reader_is_file_a_directory(&_archive, i)) {
            auto filename = directory.string() + (directory.empty() ? "" : "\\") + file_stat.m_filename;
            boost::filesystem::path filepath = boost::filesystem::path(filename);
            boost::filesystem::path parent_path = filepath.parent_path();
            if (!boost::filesystem::exists(parent_path)){
                if (!boost::filesystem::create_directories(filepath.parent_path())) {
                    LOG(LOG_LEVEL_ERROR, _T("DecompressArchive could not create directory."));
                    return false;
                }
            }
            extracted_files.push_back(filename);
            boost::filesystem::remove(filename);
            if (!mz_zip_reader_extract_to_file(&_archive, i, filename.c_str(), 0)) {
                LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_extract_to_file() failed!"));
                return false;
            }
        }
    }
    return true;
}

archive::unzip_base::unzip_base(std::iostream &data_stream, size_t offset) : _data_stream(data_stream), _offset(offset){
    if (!(_is_open = init())){
        BOOST_THROW_EXCEPTION_BASE_STRING(unzip_base::exception, boost::str(boost::wformat(L"Failed to initialize archive.")));
    }
}

bool archive::unzip_base::close(){
    try{
        cleanup();
        return true;
    }
    catch (archive::unzip_base::exception &e){
        LOG(LOG_LEVEL_ERROR, L"%s", get_diagnostic_information(e).c_str());
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
    }
    return false;
}

bool archive::unzip_base::init(){
    MZ_CLEAR_OBJ(_archive);
    _archive.m_pRead = _zip_file_read_func;
    _archive.m_pIO_opaque = this;
    _data_stream.seekg((size_t)0, std::iostream::end);
    _archive.m_archive_size = _data_stream.tellg();
    _archive.m_archive_size -= _offset;
    _data_stream.seekg((size_t)_offset, std::iostream::beg);
    if (_archive.m_archive_size){
        if (!mz_zip_reader_init(&_archive, _archive.m_archive_size, 0)){
            return false;
        }
        if (_archive.m_zip_mode != MZ_ZIP_MODE_READING)
            return false;
        if ((_archive.m_total_files == 0xFFFF) || ((_archive.m_archive_size + MZ_ZIP_CENTRAL_DIR_HEADER_SIZE + MZ_ZIP_LOCAL_DIR_HEADER_SIZE) > 0xFFFFFFFF))
            return false;
    }
    return true;
}

void archive::unzip_base::cleanup(){
    bool status = true;
    if (_is_open){
        if (!mz_zip_reader_end(&_archive)) {
            LOG(LOG_LEVEL_ERROR, _T("mz_zip_reader_end() failed!"));
            status = false;
        }
        MZ_CLEAR_OBJ(_archive);
        _is_open = false;
        if (!status)
            BOOST_THROW_EXCEPTION_BASE_STRING(unzip_base::exception, boost::str(boost::wformat(L"Failed to finalize archive .")));
    }
}
#endif
};

#endif
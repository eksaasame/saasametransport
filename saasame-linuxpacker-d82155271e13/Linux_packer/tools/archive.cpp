#include "archive.h"
namespace linux_tools
{
    archive::zip:: ~zip() {
        cleanup();
        if (_file.get() != NULL)
            _file->close();
    };
    bool archive::zip::add(boost::filesystem::path _path, mz_uint level_and_flags) {
        return add(_path, "", level_and_flags);
    }
    archive::zip::zip(std::iostream &data_stream) : _data_stream(data_stream) {
        if (!(_is_open = init())) {
            BOOST_THROW_EXCEPTION_BASE_STRING(zip::exception, boost::str(boost::format("Failed to initialize archive.")));
        }
    }

    bool archive::zip::close() {
        try {
            cleanup();
            return true;
        }
        catch (archive::zip::exception &e) {
            LOG_ERROR("%s", boost::exception_detail::get_diagnostic_information(e, "error:"));
        }
        catch (...) {
            LOG_ERROR("Unknown exception");
        }
        return false;
    }

    bool archive::zip::init() {
        MZ_CLEAR_OBJ(_archive);
        _archive.m_pRead = _zip_file_read_func;
        _archive.m_pWrite = _zip_file_write_func;
        _archive.m_pIO_opaque = this;
        _data_stream.seekg((size_t)0, std::iostream::end);
        _archive.m_archive_size = _data_stream.tellg();
        _data_stream.seekg((size_t)0, std::iostream::beg);
        if (_archive.m_archive_size) {
            if (!mz_zip_reader_init(&_archive, _archive.m_archive_size, 0)) {
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
        else {
            if (!mz_zip_writer_init(&_archive, 0))
                return false;
        }
        return true;
    }

    void archive::zip::cleanup() {
        bool status = true;
        if (_is_open) {
            if (!mz_zip_writer_finalize_archive(&_archive))
                status = false;
            if (!mz_zip_writer_end(&_archive))
                status = false;
            MZ_CLEAR_OBJ(_archive);
            _is_open = false;
            if (!status)
                BOOST_THROW_EXCEPTION_BASE_STRING(zip::exception, boost::str(boost::format("Failed to finalize archive .")));
        }
    }

    archive::zip::ptr archive::zip::open(boost::filesystem::path _file) {
        zip::ptr zip_ptr;
        try {
            if (!boost::filesystem::exists(_file)) {
                std::ofstream{ _file.string() };
            }
            std::shared_ptr<std::fstream> file(new std::fstream(_file.string(), std::ios::out | std::ios::in | std::ios::binary));
            if (file->is_open()) {
                zip_ptr = zip::ptr(new zip(*file.get()));
                zip_ptr->_file = file;
            }
        }
        catch (archive::zip::exception &e) {
            zip_ptr.reset();
            LOG_ERROR("%s", boost::exception_detail::get_diagnostic_information(e, "error:"));
        }
        catch (...) {
            zip_ptr.reset();
            LOG_ERROR("Unknown exception");
        }
        return zip_ptr;

    }

    archive::zip::ptr archive::zip::open(std::iostream& data_stream) {
        zip::ptr zip_ptr;
        try {
            zip_ptr = zip::ptr(new zip(data_stream));
        }
        catch (archive::zip::exception &e) {
            zip_ptr.reset();
            LOG_ERROR("%s", boost::exception_detail::get_diagnostic_information(e, "error:"));
        }
        catch (...) {
            zip_ptr.reset();
            LOG_ERROR("Unknown exception");
        }
        return zip_ptr;
    }

    size_t archive::zip::_zip_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n) {
        archive::zip* archive = (archive::zip*)pOpaque;
        size_t size = 0;
        if (((mz_int64)file_ofs < 0))
            return 0;
        try {
            archive->_data_stream.seekp((size_t)file_ofs, std::iostream::beg);
            archive->_data_stream.write((char*)pBuf, n);
            size = (size_t)archive->_data_stream.tellp() - file_ofs;
        }
        catch (...) {
            size = 0;
        }
        return size;
    }

    size_t archive::zip::_zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n) {
        archive::zip* archive = (archive::zip*)pOpaque;
        size_t size = 0;
        if (((mz_int64)file_ofs < 0))
            return 0;
        try {
            archive->_data_stream.seekg((size_t)file_ofs, std::iostream::beg);
            archive->_data_stream.read((char *)pBuf, n);
            size = (size_t)archive->_data_stream.tellg() - file_ofs;
        }
        catch (...) {
            size = 0;
        }
        return size;
    }

    bool archive::zip::_add(boost::filesystem::path path_add_to_zip, boost::filesystem::path name_in_archive, bool is_dir, mz_uint level_and_flags) {
        bool status = false;
        if (!boost::filesystem::exists(path_add_to_zip))
            return status;
        if (is_dir)
            status = MZ_TRUE == mz_zip_writer_add_mem_ex(&_archive, name_in_archive.string().c_str(), NULL, 0, NULL, 0, level_and_flags, 0, 0);
        else if (boost::filesystem::is_regular_file(path_add_to_zip))
            status = MZ_TRUE == mz_zip_writer_add_file(&_archive, name_in_archive.string().c_str(), path_add_to_zip.string().c_str(), NULL, 0, level_and_flags);
        else {
            LOG_ERROR("only support adding folder and file");
            status = false;
        }
        return status;
    }

    bool archive::zip::add(std::string name_in_arcive, const std::string& data, mz_uint level_and_flags) {
        bool status = true;
        if (!mz_zip_writer_validate_archive_name(name_in_arcive.c_str()) || !_is_open)
            return false;
        if (!mz_zip_writer_add_mem(&_archive, name_in_arcive.c_str(), data.c_str(), data.length(), level_and_flags))
            status = false;
        return status;
    }

    bool archive::zip::add(boost::filesystem::path _path, boost::filesystem::path name_in_archive, mz_uint level_and_flags) {
        boost::filesystem::directory_iterator end_iter;
        bool status = false;
        if (!boost::filesystem::exists(_path) || !_is_open)
            return status;
        else if (boost::filesystem::is_directory(_path)) {
            boost::filesystem::path temp_path;
            temp_path = (name_in_archive.empty() ? "" : name_in_archive) / _path.filename();
            if (_add(_path, (temp_path.string() + '/'), true, level_and_flags))
                status = true;
            for (boost::filesystem::directory_iterator dir_iter(_path); dir_iter != end_iter; ++dir_iter) {
                if (add(dir_iter->path(), (temp_path.string() + '/')))
                    status = true;
            }
        }
        else if (boost::filesystem::is_regular_file(_path)) {
            if (name_in_archive.empty())
                name_in_archive = _path.filename();
            else
                name_in_archive /= _path.filename();
            if (_add(_path, name_in_archive, false, level_and_flags))
                status = true;
        }
        return status;
    }
}



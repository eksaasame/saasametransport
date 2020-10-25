#pragma once
#include <boost/filesystem.hpp>
#include <boost/exception/all.hpp>
#include <boost/format.hpp>
#include <boost/utility/string_view.hpp>
#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include "exception_base.h"
#include "log.h"
#include "miniz.h"
#ifndef _ARCHIVE_HPP_
#define _ARCHIVE_HPP_
#define MZ_MIN(a,b) (((a)<(b))?(a):(b))
namespace linux_tools
{
    class archive {
    public:
        class zip {
        public:
            typedef boost::shared_ptr<zip> ptr;
            struct  exception : virtual public boost::exception, std::exception {};
            virtual ~zip();
            static zip::ptr open(boost::filesystem::path _file);
            static zip::ptr open(std::iostream& data_stream);
            bool add(boost::filesystem::path _path, mz_uint level_and_flags = MZ_DEFAULT_LEVEL);
            bool add(std::string name_in_arcive, const std::string& data, mz_uint level_and_flags = MZ_DEFAULT_LEVEL);
            bool close();
        private:
            bool init();
            bool _add(boost::filesystem::path file_add_to_zip, boost::filesystem::path name_in_archive, bool is_dir = false, mz_uint level_and_flags = MZ_DEFAULT_LEVEL);
            bool add(boost::filesystem::path _path, boost::filesystem::path name_in_archive, mz_uint level_and_flags = MZ_DEFAULT_LEVEL);
            zip(std::iostream &data_stream);
            static size_t _zip_file_write_func(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n);
            static inline size_t _zip_file_read_func(void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n);
            std::iostream	&_data_stream;
            mz_zip_archive  _archive;
            inline void cleanup();
            std::shared_ptr<std::fstream> _file;
            bool _is_open;
        };
    };
}
#endif



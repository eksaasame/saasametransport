#include "irm_imagex_op.h"

using namespace macho;

bool irm_imagex_local_op::read_file(__in boost::filesystem::path file, __inout std::ifstream& ifs){
    return _read_file(_output_path, file, ifs);
}

bool irm_imagex_local_op::_read_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __inout std::ifstream& ifs){
    boost::filesystem::path input_file_name = path / filename;
    try{
        if (boost::filesystem::exists(input_file_name)){
            std::ifstream().swap(ifs);
            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
            ifs.open(input_file_name.wstring(), std::ios::in | std::ios::binary, _SH_DENYRW);
            return ifs.is_open();
        }
    }
    catch (...){
    }
    return false;
}

bool irm_imagex_local_op::_read_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __inout saasame::ironman::imagex::irm_imagex_base& data){
    FUN_TRACE;
    bool result = false;
    boost::filesystem::path input_file_name = path / filename;
    try{
        std::ifstream ifs;
        if (boost::filesystem::exists(input_file_name)){
            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
            ifs.open(input_file_name.wstring(), std::ios::in | std::ios::binary, _SH_DENYRW);
            if (ifs.is_open()){
                result = data.load(ifs);
                ifs.close();
            }
        }
    }
    catch (...){
        result = false;
    }
    return result;
}

bool irm_imagex_local_op::_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in saasame::ironman::imagex::irm_imagex_base& data, bool forcefull){
    FUN_TRACE;
    bool result = false;
    boost::filesystem::path output_file_name = path / filename;
    bool is_overwrite = (boost::filesystem::exists(output_file_name));
    boost::filesystem::path temp = output_file_name.string() + (is_overwrite ? ".tmp" : "");
    try{
        std::ofstream ofs;
        ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);
        ofs.open(temp.wstring(), std::ios::trunc | std::ios::out | std::ios::binary, _SH_DENYRW);
        if (ofs.is_open()){
            if (result = data.save(ofs, forcefull)){
                ofs.close();
                if (is_overwrite && !MoveFileEx(temp.wstring().c_str(), output_file_name.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                    LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), output_file_name.wstring().c_str(), GetLastError());
                    result = false;
                }
            }
        }
    }
    catch (...){
        LOG(LOG_LEVEL_ERROR, L"Unknown exception");
        result = false;
    }
    if (!result){
        try{
            boost::filesystem::remove(temp);
        }
        catch (boost::filesystem::filesystem_error &err){
            LOG(LOG_LEVEL_ERROR, L"Cannot remove '%s' : %s", temp.wstring().c_str(), macho::stringutils::convert_utf8_to_unicode(err.what()).c_str());
        }
        catch (...){
        }
    }
    return result;
}

bool irm_imagex_local_op::_read_file(__in  boost::filesystem::path& path, __in  boost::filesystem::path& filename, __inout std::ostream& data)
{
    FUN_TRACE;
    bool result = false;
    boost::filesystem::path input_file_name = path / filename;
    try{
        std::ifstream ifs;
        if (boost::filesystem::exists(input_file_name)){
            ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit | std::ifstream::eofbit);
            ifs.open(input_file_name.wstring(), std::ios::in | std::ios::binary, _SH_DENYRW);
            if (ifs.is_open()){
                //std::ostream().swap(data);
                data.seekp(0, std::ios::beg);
                data << ifs.rdbuf();
                result = true;
                ifs.close();
            }
        }
    }
    catch (...){
        result = false;
    }
    return result;
}

bool irm_imagex_local_op::_write_file(__in boost::filesystem::path& path, boost::filesystem::path& filename, __in std::istream& data)
{
    FUN_TRACE;
    bool result = false;
    boost::filesystem::path output_file_name = path / filename;
    bool is_overwrite = (boost::filesystem::exists(output_file_name));
    boost::filesystem::path temp = output_file_name.string() + (is_overwrite ? ".tmp" : "");
    try{
        std::ofstream ofs;
        ofs.exceptions(std::ofstream::failbit | std::ofstream::badbit | std::ofstream::eofbit);
        ofs.open(temp.wstring(), std::ios::trunc | std::ios::out | std::ios::binary, _SH_DENYRW);
        if (ofs.is_open()){
            data.seekg(0, std::ios::end);
            if (data.tellg() > 0){
                data.seekg(0, std::ios::beg);
                ofs << data.rdbuf();
            }
            result = true;
            ofs.close();
            if (is_overwrite && !MoveFileEx(temp.wstring().c_str(), output_file_name.wstring().c_str(), MOVEFILE_REPLACE_EXISTING)){
                LOG(LOG_LEVEL_ERROR, L"MoveFileEx('%s', '%s') with error(%d)", temp.wstring().c_str(), output_file_name.wstring().c_str(), GetLastError());
                result = false;
            }
        }
    }
    catch (...){
        result = false;
    }
    if (!result){
        try{
            boost::filesystem::remove(temp);
        }
        catch (boost::filesystem::filesystem_error &err){
            LOG(LOG_LEVEL_ERROR, L"Cannot remove '%s' : %s", temp.wstring().c_str(), macho::stringutils::convert_utf8_to_unicode(err.what()).c_str());
        }
        catch (...){
        }
    }
    return result;
}

bool irm_imagex_local_op::is_file_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;
    return boost::filesystem::exists(_output_path / file);
}

bool irm_imagex_local_op::read_file(__in boost::filesystem::path file, __inout std::ostream& data)
{
    FUN_TRACE;
    return _read_file(_output_path, file, data);
}

bool irm_imagex_local_op::write_file(__in boost::filesystem::path filename, __in std::istream& data)
{
    FUN_TRACE;
    boost::filesystem::space_info _space_info = boost::filesystem::space(_output_path);
    if ((uint64_t)_space_info.available > (uint64_t)_reserved_space){
        return _write_file(_output_path, filename, data);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't write the file(%s), Available space %I64u bytes < %I64u bytes", filename.wstring().c_str(), _space_info.available, _reserved_space);
    }
    return false;
}

bool irm_imagex_local_op::_remove_file(__in const boost::filesystem::path& path, __in boost::filesystem::path& filename)
{
    FUN_TRACE;

    bool result = true;
    boost::filesystem::path file_path_name = path / filename;

    try
    {
        if (boost::filesystem::exists(file_path_name)){
            if (boost::filesystem::is_directory(file_path_name))
                boost::filesystem::remove_all(file_path_name);
            else
                boost::filesystem::remove(file_path_name);
        }
    }
    catch (...)
    {
        result = false;
    }

    return result;
}

bool irm_imagex_local_op::remove_file(__in boost::filesystem::path file)
{
    FUN_TRACE;
    return _remove_file(_output_path, file);
}

bool irm_imagex_local_op::is_temp_file_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;
    return boost::filesystem::exists(_data_path / file);
}

bool irm_imagex_local_op::read_temp_file(__in boost::filesystem::path file, __inout std::ostream& data)
{
    FUN_TRACE;
    return _read_file(_data_path, file, data);
}

bool irm_imagex_local_op::write_temp_file(__in boost::filesystem::path file, __in std::istream& data)
{
    FUN_TRACE;
    boost::filesystem::space_info _space_info = boost::filesystem::space(_data_path);
    boost::uintmax_t  reserved_space = ((uint64_t)_reserved_space > 1024 * 1024 * 512) ? 1024 * 1024 * 512 : _reserved_space;
    if ((uint64_t)_space_info.available > (uint64_t)reserved_space){
        return _write_file(_data_path, file, data);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't write the temp file(%s), Available space %I64u bytes < %I64u bytes", file.wstring().c_str(), _space_info.available, reserved_space);
    }
    return false;
}

bool irm_imagex_local_op::remove_temp_file(__in boost::filesystem::path file)
{
    FUN_TRACE;
    return _remove_file(_data_path ,file);
}

bool irm_imagex_local_op::is_metafile_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;

    boost::filesystem::path file_path_name = _working_path / file;

    if (boost::filesystem::exists(file_path_name))
        return true;
    else
        return false;
}

macho::windows::lock_able::vtr irm_imagex_local_op::get_lock_objects(std::wstring& lock_filename)
{
    FUN_TRACE;

    macho::windows::lock_able::vtr locks;
    boost::filesystem::path lock_path_name = _output_path / lock_filename;
    boost::filesystem::path p(lock_path_name);
    macho::windows::file_lock_ex* l = new macho::windows::file_lock_ex(p, _flag);
    l->register_is_cancelled_function(boost::bind(&irm_imagex_local_op::is_canceled, this));
    macho::windows::lock_able::ptr lock(l);
    locks.push_back(lock);

    return locks;
}

macho::windows::lock_able::ptr irm_imagex_local_op::get_lock_object(std::wstring& lock_filename, std::string flag){
    FUN_TRACE;
    boost::filesystem::path lock_path_name = _output_path / lock_filename;
    boost::filesystem::path p(lock_path_name);
    macho::windows::file_lock_ex* l = new macho::windows::file_lock_ex(p, flag);
    l->register_is_cancelled_function(boost::bind(&irm_imagex_local_op::is_canceled, this));
    return macho::windows::lock_able::ptr(l);
}

//read meta file from local cache
bool irm_imagex_local_op::read_metafile(__in boost::filesystem::path file, __inout std::stringstream& data)
{
    FUN_TRACE;
    return _read_file(_working_path, file, data);
}

bool irm_imagex_local_op::read_metafile(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data)
{
    FUN_TRACE;
    return _read_file(_working_path, file, data);
}

//update local meta file and then flush to the destination
bool irm_imagex_local_op::write_metafile(__in boost::filesystem::path file, __inout std::stringstream& data, bool local_only)
{
    FUN_TRACE;

    bool result = true;
    boost::filesystem::path p = _working_path / file;

    try
    {
        if (!boost::filesystem::exists(p.parent_path()))
            result = boost::filesystem::create_directories(p.parent_path());

        if (result)
        {
            if ((result = _write_file(p.parent_path(), p.filename(), data)))
            {
                if (!local_only){
                    //write out to the destination
                    if (_magic > 0)
                        result = _delay_write_file(_output_path, p.filename(), data);
                    else{
                        std::wstring lock = p.filename().wstring();
                        try
                        {
                            macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
                            macho::windows::auto_locks block_file_locks(locks_ptr);
                            result = _write_file(_output_path, p.filename(), data);
                        }
                        catch (macho::windows::lock_able::exception &ex){
                            result = false;
                            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        result = false;
    }

    return result;
}

//update local meta file and then flush to the destination
bool irm_imagex_local_op::write_metafile(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data, bool local_only)
{
    FUN_TRACE;

    bool result = true;
    boost::filesystem::path p = _working_path / file;

    try
    {
        if (!boost::filesystem::exists(p.parent_path()))
            result = boost::filesystem::create_directories(p.parent_path());

        if (result)
        {
            if ((result = _write_file(p.parent_path(), p.filename(), data, false)))
            {
                if (!local_only){
                    //write out to the destination
                    if (_magic > 0)
                        result = _delay_write_file(_output_path, p.filename(), data);
                    else{
                        std::wstring lock = p.filename().wstring();
                        try
                        {
                            macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
                            macho::windows::auto_locks block_file_locks(locks_ptr);
                            result = _write_file(_output_path, p.filename(), data, false);
                        }
                        catch (macho::windows::lock_able::exception &ex){
                            result = false;
                            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
        result = false;
    }

    return result;
}

bool irm_imagex_local_op::remove_metafile(__in boost::filesystem::path file)
{
    return  _remove_file(_working_path, file);
}

bool irm_imagex_local_op::remove_metafiles(__in std::vector<boost::filesystem::path>& files)
{
    FUN_TRACE;

    bool result = true;

    foreach(boost::filesystem::path& file, files)
    {
        result = _remove_file(_working_path, file);
        if (!result)
            break;;
    }

    return result;
}

bool irm_imagex_local_op::_delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data)
{
    bool result = true;
    if (0 == _wrt_count.count(filename) || _wrt_count[filename] % _magic == 0)
    {
        try
        {
            std::wstring lock = filename.wstring();
            macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
            macho::windows::auto_locks block_file_locks(locks_ptr);
            result = _write_file(path, filename, data);
        }
        catch (macho::windows::lock_able::exception &ex){
            result = false;
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        _wrt_count[filename] = 1;
    }
    else
    {
        _wrt_count[filename]++;
    }
    return result;
}

bool irm_imagex_local_op::_delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in saasame::ironman::imagex::irm_imagex_base& data)
{
    bool result = true;
    if (0 == _wrt_count.count(filename) || _wrt_count[filename] % _magic == 0)
    {
        try
        {
            std::wstring lock = filename.wstring();
            macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
            macho::windows::auto_locks block_file_locks(locks_ptr);
            result = _write_file(path, filename, data, false);
        }
        catch (macho::windows::lock_able::exception &ex){
            result = false;
            LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
        }
        _wrt_count[filename] = 1;
    }
    else
    {
        _wrt_count[filename]++;
    }
    return result;
}

bool irm_imagex_local_op::flush_metafiles(__in std::vector<boost::filesystem::path>& files)
{
    FUN_TRACE;
    bool result = true;
    foreach(boost::filesystem::path& file, files)
    {
        std::ifstream ifs;
        result = _read_file(_working_path, file, ifs);
        if (result)
        {
            try
            {
                std::wstring lock = file.filename().wstring();
                macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
                macho::windows::auto_locks block_file_locks(locks_ptr);
                result = _write_file(_output_path, file.filename(), ifs);
                ifs.close();
                if (!result)
                    break;
            }
            catch (macho::windows::lock_able::exception &ex){
                result = false;
                LOG(LOG_LEVEL_ERROR, _T("%s"), get_diagnostic_information(ex).c_str());
                break;
            }
        }
        else
            break;
    }
    return result;
}

bool irm_imagex_local_op::_remove_image(__in const boost::filesystem::path &dir, std::wstring& image_name){
    boost::filesystem::directory_iterator end_itr;
    boost::regex file_filter(macho::stringutils::convert_unicode_to_utf8(image_name) + "\..*");
    for (boost::filesystem::directory_iterator i(dir); i != end_itr; ++i)
    {
        // Skip if no match
        if (!boost::regex_match(i->path().filename().string(), file_filter))
        {
            if (boost::filesystem::is_directory(i->path())) // look for the image in sub-directories.
            {
                if (!_remove_image(i->path(), image_name))
                    return false;
            }
            continue;
        }
        // File matches, remove it
        if (!_remove_file(dir, i->path().filename()))
            return false;
    }

    if (!_remove_file(dir, boost::filesystem::path(image_name)))
        return false;
    
    return true;
}

bool irm_imagex_local_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;

    std::vector<boost::filesystem::path> target_paths;
    if (_output_path.string().length())
        target_paths.push_back(_output_path);
    target_paths.push_back(_working_path);
    if (_working_path != _data_path)
        target_paths.push_back(_data_path);

    foreach(boost::filesystem::path& target_path, target_paths)
    {
        if (!_remove_image(target_path, image_name))
            return false;
    }

    return true;
}

bool irm_imagex_local_op::release_image_lock(__in std::wstring& image_name)
{
    FUN_TRACE;

    std::vector<boost::filesystem::path> target_paths;
    boost::filesystem::directory_iterator end_itr;
    boost::regex file_filter(macho::stringutils::convert_unicode_to_utf8(image_name) + "*\.lck");

    target_paths.push_back(_working_path);

    foreach(boost::filesystem::path& target_path, target_paths)
    {
        for (boost::filesystem::directory_iterator i(target_path); i != end_itr; ++i)
        {
            // Skip if no match
            if (!boost::regex_match(i->path().filename().string(), file_filter))
                continue;

            // File matches, remove it
            if (!_remove_file(target_path, i->path().filename()))
                return false;
        }
    }
    return true;
}

std::wstring irm_imagex_local_op::get_path_name() const
{
    return _output_path.wstring();
}

bool irm_imagex_local_op::read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data){
    FUN_TRACE;
    return _read_file(_output_path, file, data);
}

bool irm_imagex_local_op::write_file(__in boost::filesystem::path filename, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull){
    FUN_TRACE;
    boost::filesystem::space_info _space_info = boost::filesystem::space(_output_path);
    if ((uint64_t)_space_info.available > (uint64_t)_reserved_space){
        return _write_file(_output_path, filename, data, forcefull);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't write the file(%s), Available space %I64u bytes < %I64u bytes", filename.wstring().c_str(), _space_info.available, _reserved_space);
    }
    return false;
}

bool irm_imagex_local_op::read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data){
    FUN_TRACE;
    return _read_file(_output_path, file, data);
}

bool irm_imagex_local_op::write_file(__in boost::filesystem::path filename, __in saasame::ironman::imagex::irm_imagex_base& data){
    FUN_TRACE;
    boost::filesystem::space_info _space_info = boost::filesystem::space(_output_path);
    boost::uintmax_t  reserved_space = ((uint64_t)_reserved_space > 1024 * 1024 * 512) ? 1024 * 1024 * 512 : _reserved_space;
    if ((uint64_t)_space_info.available > (uint64_t)reserved_space){
        return _write_file(_output_path, filename, data, false);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't write the file(%s), Available space %I64u bytes < %I64u bytes", filename.wstring().c_str(), _space_info.available, reserved_space);
    }
    return false;
}

bool irm_imagex_local_op::read_temp_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data){
    FUN_TRACE;
    return _read_file(_data_path, file, data);
}

bool irm_imagex_local_op::write_temp_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data){
    FUN_TRACE;
    boost::filesystem::space_info _space_info = boost::filesystem::space(_data_path);
    boost::uintmax_t  reserved_space = ((uint64_t)_reserved_space > 1024 * 1024 * 512) ? 1024 * 1024 * 512 : _reserved_space;
    if ((uint64_t)_space_info.available > (uint64_t)reserved_space){
        return _write_file(_data_path, file, data, true);
    }
    else{
        LOG(LOG_LEVEL_ERROR, L"Can't write the temp file(%s), Available space %I64u bytes < %I64u bytes", file.wstring().c_str(), _space_info.available, reserved_space);
    }
    return false;
}

irm_imagex_local_ex_op::irm_imagex_local_ex_op(boost::filesystem::path remote_path, boost::filesystem::path local_path, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, boost::uintmax_t reserved_space)
    : irm_imagex_local_op(local_path, working_path, data_path, magic, flag, reserved_space)
{
    _remote_path = remote_path;
    _magic = magic;
}

irm_imagex_local_ex_op::~irm_imagex_local_ex_op()
{

}

bool irm_imagex_local_ex_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;
    bool result = false;
    std::vector<boost::filesystem::path> target_paths;
    if (_remote_path.string().length())
        target_paths.push_back(_remote_path);
    if (_output_path.string().length())
        target_paths.push_back(_output_path);
    target_paths.push_back(_working_path);
    if (_working_path != _data_path)
        target_paths.push_back(_data_path);

    foreach(boost::filesystem::path& target_path, target_paths)
    {
        if (!_remove_image(target_path, image_name))
            return false;
    }
    return true;
}

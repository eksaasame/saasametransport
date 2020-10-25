#include "irm_imagex_op.h"

using namespace macho;

irm_imagex_webdav_op::irm_imagex_webdav_op(webdav::ptr dav, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, uint32_t queue_size)
: irm_imagex_local_op("", working_path, data_path),
_dav(dav)
{
    _magic = magic;
    _flag = flag;
    for (uint32_t i = 0; i < queue_size; i++)
        _queue.push_back(dav->clone());
}

std::wstring irm_imagex_webdav_op::get_path_name() const 
{
    if (_dav)
        return macho::stringutils::convert_ansi_to_unicode(_dav->uri());
    return L"";
}

irm_imagex_webdav_op::~irm_imagex_webdav_op()
{

}

webdav::ptr irm_imagex_webdav_op::_pop_front(){
    macho::windows::auto_lock lock(_cs);
    webdav::ptr dav = _queue.front();
    if (dav){
        _queue.pop_front();
    }
    else{
        dav = _dav->clone();
    }
    return dav;
}

void irm_imagex_webdav_op::_push_front(webdav::ptr dav){
    macho::windows::auto_lock lock(_cs);
    _queue.push_front(dav);
}

bool irm_imagex_webdav_op::read_file(__in boost::filesystem::path file, __inout std::ostream& data)
{
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        result = dav->get_file(file.filename().string(), data);
        _push_front(dav);
    }
    return result;
}

bool irm_imagex_webdav_op::write_file(__in boost::filesystem::path file, __in std::istream& data)
{
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        result = dav->put_file(file.filename().string(), data);
        _push_front(dav);
    }
    return result;
}

bool irm_imagex_webdav_op::remove_file(__in boost::filesystem::path file)
{
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        result = dav->remove(file.filename().string());
        _push_front(dav);
    }
    return result;
}

bool irm_imagex_webdav_op::is_file_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        result = dav->is_existing(file.filename().string());
        _push_front(dav);
    }
    return result;
}

bool irm_imagex_webdav_op::write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool local_only)
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
                        try{ 
                            std::wstring lock = p.filename().wstring();
                            macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
                            macho::windows::auto_locks block_file_locks(locks_ptr);
                            bool result = false;
                            webdav::ptr dav = _pop_front();
                            if (dav){
                                result = dav->put_file(file.filename().string(),data);
                                _push_front(dav);
                            }
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

bool irm_imagex_webdav_op::_delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data)
{
    FUN_TRACE;
    bool result = true;
    if (0 == _wrt_count.count(filename) || _wrt_count[filename] % _magic == 0)
    {
        try
        { 
            std::wstring lock = filename.wstring();
            macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
            macho::windows::auto_locks block_file_locks(locks_ptr);
            webdav::ptr dav = _pop_front();
            if (dav){
                result = dav->put_file(filename.string(), data);
                _push_front(dav);
            }
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

bool irm_imagex_webdav_op::flush_metafiles(__in std::vector<boost::filesystem::path>& files)
{
    FUN_TRACE;
    bool result = true;
    foreach(boost::filesystem::path& file, files)
    {
        std::ifstream ifs;
        if (result = _read_file(_working_path, file, ifs)){
            try{ 
                std::wstring lock = file.filename().wstring();
                macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
                macho::windows::auto_locks block_file_locks(locks_ptr);
                webdav::ptr dav = _pop_front();
                if (dav){
                    result = dav->put_file(file.filename().string(), ifs);
                    _push_front(dav);
                }
                else{
                    result = false;
                }
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

bool irm_imagex_webdav_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        webdav::item::vtr items = dav->enumerate_sub_items("");
        std::string _name = macho::stringutils::convert_unicode_to_ansi(image_name);
        result = true;
        foreach(webdav::item::ptr &it, items){
            if (0 == _strnicmp(_name.c_str(), it->name.c_str(), _name.length())){
                if (it->is_dir){
                    result = dav->remove_dir(it->name);
                }
                else{
                    result = dav->remove(it->name);
                }
                if (!result)
                    break;
            }
        }
        _push_front(dav);
        return result ? irm_imagex_local_op::remove_image(image_name) : false;
    }
    return result;
}

macho::windows::lock_able::vtr irm_imagex_webdav_op::get_lock_objects(std::wstring& lock_filename)
{
    macho::windows::lock_able::vtr locks;
    webdav::lock_ex *l = new webdav::lock_ex(lock_filename, _dav, _flag);
    l->register_is_cancelled_function(boost::bind(&irm_imagex_webdav_op::is_canceled, this));
    locks.push_back(macho::windows::lock_able::ptr(l));

    return locks;
}


bool irm_imagex_webdav_op::read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data){
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
        if (result = dav->get_file(file.filename().string(), buffer))
            result = data.load(buffer);
        _push_front(dav);
    }
    return result;
}

bool irm_imagex_webdav_op::write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull){
    FUN_TRACE;
    bool result = false;
    webdav::ptr dav = _pop_front();
    if (dav){
        std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
        if (data.save(buffer, forcefull))
            result = dav->put_file(file.filename().string(), buffer);
        _push_front(dav);
    }
    return result;
}

bool irm_imagex_webdav_op::write_metafile(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data, bool local_only)
{
    FUN_TRACE;
    std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    bool result = false;
    if (result = data.save(buffer))
        result = write_metafile(file, buffer, local_only);
    return result;
}
#include "irm_imagex_op.h"

using namespace macho;

irm_imagex_s3_op::irm_imagex_s3_op(aws_s3::ptr s3, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, uint32_t queue_size)
    : irm_imagex_local_op("", working_path, data_path),
    _s3(s3)
{
    _magic = magic;
    _flag = flag;
    for (int i = 0; i < queue_size; i++)
        _queue.push_back(s3->clone());
}

std::wstring irm_imagex_s3_op::get_path_name() const
{
    if (_s3){       
        return macho::stringutils::convert_ansi_to_unicode(boost::str(boost::format("%s\\%s") %_s3->key_id() %_s3->bucket()));
    }
    return L"";
}

irm_imagex_s3_op::~irm_imagex_s3_op()
{

}

aws_s3::ptr irm_imagex_s3_op::_pop_front(){
    macho::windows::auto_lock lock(_cs);
    aws_s3::ptr s3 = _queue.front();
    if (s3){
        _queue.pop_front();
    }
    else{
        s3 = _s3->clone();
    }
    return s3;
}

void irm_imagex_s3_op::_push_front(aws_s3::ptr dav){
    macho::windows::auto_lock lock(_cs);
    _queue.push_front(dav);
}

bool irm_imagex_s3_op::read_file(__in boost::filesystem::path file, __inout std::ostream& data)
{
    FUN_TRACE;
    bool result = false;
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        result = s3->get_file(file.filename().string(), data);
        _push_front(s3);
    }
    return result;
}

bool irm_imagex_s3_op::write_file(__in boost::filesystem::path file, __in std::istream& data)
{
    FUN_TRACE;
    bool result = false;
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        result = s3->put_file(file.filename().string(), data);
        _push_front(s3);
    }
    return result;
}

bool irm_imagex_s3_op::remove_file(__in boost::filesystem::path file)
{
    FUN_TRACE;
    bool result = false;
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        result = s3->remove(file.filename().string());
        _push_front(s3);
    }
    return result;
}

bool irm_imagex_s3_op::is_file_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;
    bool result = false;
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        result = s3->is_existing(file.filename().string());
        _push_front(s3);
    }
    return result;
}

bool irm_imagex_s3_op::write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool local_only)
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
                            aws_s3::ptr s3 = _pop_front();
                            if (s3){
                                result = s3->put_file(file.filename().string(), data);
                                _push_front(s3);
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

bool irm_imagex_s3_op::write_metafile(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data, bool local_only)
{
    FUN_TRACE;
    std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    bool result = false;
    if (result = data.save(buffer))
        result = write_metafile(file, buffer, local_only);
    return result;
}

bool irm_imagex_s3_op::_delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data)
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
            aws_s3::ptr s3 = _pop_front();
            if (s3){
                result = s3->put_file(filename.string(), data);
                _push_front(s3);
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

bool irm_imagex_s3_op::flush_metafiles(__in std::vector<boost::filesystem::path>& files)
{
    FUN_TRACE;
    bool result = true;
    foreach(boost::filesystem::path& file, files)
    {
        std::ifstream ifs;
        if (result = _read_file(_working_path, file, ifs)){
            try
            {
                std::wstring lock = file.filename().wstring();
                macho::windows::lock_able::vtr locks_ptr = get_lock_objects(lock);
                macho::windows::auto_locks block_file_locks(locks_ptr);
                aws_s3::ptr s3 = _pop_front();
                if (s3){
                    result = s3->put_file(file.filename().string(), ifs);
                    _push_front(s3);
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

bool irm_imagex_s3_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;
    bool result = false;
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        aws_s3::item::vtr items = s3->enumerate_sub_items("");
        std::string _name = macho::stringutils::convert_unicode_to_ansi(image_name);
        result = true;
        foreach(aws_s3::item::ptr &it, items){
            if (0 == _strnicmp(_name.c_str(), it->name.c_str(), _name.length())){
                if (it->is_dir){
                    result = s3->remove_dir(it->name);
                }
                else{
                    result = s3->remove(it->name);
                }
                if (!result)
                    break;
            }
        }
        _push_front(s3);
        return result ? irm_imagex_local_op::remove_image(image_name) : false;
    }
    return result;
}

macho::windows::lock_able::vtr irm_imagex_s3_op::get_lock_objects(std::wstring& lock_filename)
{
    macho::windows::lock_able::vtr locks;
    aws_s3::lock_ex *l = new aws_s3::lock_ex(lock_filename, _s3, _flag);
    l->register_is_cancelled_function(boost::bind(&irm_imagex_s3_op::is_canceled, this));
    locks.push_back(macho::windows::lock_able::ptr(l));
    return locks;
}

bool irm_imagex_s3_op::read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data){
    FUN_TRACE;
    bool result = false;
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
        if (result = s3->get_file(file.filename().string(), buffer))
            result = data.load(buffer);
        _push_front(s3);
    }
    return result;
}

bool irm_imagex_s3_op::write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull){
    FUN_TRACE;
    bool result = false;
    std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
    aws_s3::ptr s3 = _pop_front();
    if (s3){
        std::stringstream buffer(std::ios_base::binary | std::ios_base::out | std::ios_base::in);
        if (data.save(buffer, forcefull))
            result = s3->put_file(file.filename().string(), buffer);
        _push_front(s3);
    }
    return result;
}

#include "irm_imagex_op.h"

using namespace macho;

irm_imagex_webdav_ex_op::irm_imagex_webdav_ex_op(webdav::ptr dav, boost::filesystem::path local_path, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, boost::uintmax_t reserved_space)
    : irm_imagex_local_op(local_path, working_path, data_path, magic, flag, reserved_space)
{
    _dav = dav;
    _magic = magic;
}

irm_imagex_webdav_ex_op::~irm_imagex_webdav_ex_op()
{

}

bool irm_imagex_webdav_ex_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;
    bool result = false;
    if (_dav){
        webdav::item::vtr items = _dav->enumerate_sub_items("");
        std::string _name = macho::stringutils::convert_unicode_to_ansi(image_name);
        result = true;
        foreach(webdav::item::ptr &it, items){
            if (0 == _strnicmp(_name.c_str(), it->name.c_str(), _name.length())){
                if (it->is_dir){
                    result = _dav->remove_dir(it->name);
                }
                else{
                    result = _dav->remove(it->name);
                }
                if (!result)
                    break;
            }
        }
        return result ? irm_imagex_local_op::remove_image(image_name) : false;
    }
    return result;
}

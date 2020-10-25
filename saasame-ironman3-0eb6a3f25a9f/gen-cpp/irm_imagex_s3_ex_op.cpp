#include "irm_imagex_op.h"

using namespace macho;

irm_imagex_s3_ex_op::irm_imagex_s3_ex_op(aws_s3::ptr s3, boost::filesystem::path local_path, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, boost::uintmax_t reserved_space)
    : irm_imagex_local_op(local_path, working_path, data_path, magic, flag, reserved_space)
{
    _s3 = s3;
    _magic = magic;
}

irm_imagex_s3_ex_op::~irm_imagex_s3_ex_op()
{

}

bool irm_imagex_s3_ex_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;
    bool result = false;
    if (_s3){
        aws_s3::item::vtr items = _s3->enumerate_sub_items("");
        std::string _name = macho::stringutils::convert_unicode_to_ansi(image_name);
        result = true;
        foreach(aws_s3::item::ptr &it, items){
            if (0 == _strnicmp(_name.c_str(), it->name.c_str(), _name.length())){
                if (it->is_dir){
                    result = _s3->remove_dir(it->name);
                }
                else{
                    result = _s3->remove(it->name);
                }
                if (!result)
                    break;
            }
        }
        return result ? irm_imagex_local_op::remove_image(image_name) : false;
    }
    return result;
}

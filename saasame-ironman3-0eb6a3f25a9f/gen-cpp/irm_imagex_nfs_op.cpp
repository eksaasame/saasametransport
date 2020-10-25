#include "irm_imagex_op.h"

using namespace macho;

irm_imagex_nfs_op::irm_imagex_nfs_op(macho::windows::wnet_connection::ptr wnet, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag) :
irm_imagex_local_op(wnet->path(), working_path, data_path, magic, flag),
_wnet(wnet)
{
    _magic = magic;
}

irm_imagex_nfs_op::~irm_imagex_nfs_op()
{

}

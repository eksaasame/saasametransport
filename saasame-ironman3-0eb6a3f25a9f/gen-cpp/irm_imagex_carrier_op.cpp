#include "irm_imagex_op.h"
#include <boost/lexical_cast.hpp>
using namespace macho;

irm_imagex_carrier_op::ptr irm_imagex_carrier_op::get(std::set<connection>& conns, boost::filesystem::path working_path, boost::filesystem::path data_path)
{
    boost::uintmax_t reserved_space = (((boost::uintmax_t)2) * 1024 * 1024 * 1024);
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"ReservedSpace"].exists() && reg[L"ReservedSpace"].is_dword())
            reserved_space = ((boost::uintmax_t)(DWORD)reg[L"ReservedSpace"] ) * 1024 * 1024;
    }
    irm_imagex_carrier_op::ptr cop = irm_imagex_carrier_op::ptr(new irm_imagex_carrier_op(working_path, data_path));
    if (cop)
    {
        std::map<std::wstring, bool> paths;
        foreach(connection conn, conns)
        {
            uint16_t _magic = MAGIC;
            if (conn.options.count("magic")){
                try {
                    _magic = boost::lexical_cast<uint16_t>(conn.options["magic"]);
                }
                catch (boost::bad_lexical_cast const&) {
                }
            }
            switch (conn.type)
            {
                case  saasame::transport::connection_type::LOCAL_FOLDER_EX:
                {                 
                    irm_imagex_op::ptr op(new irm_imagex_local_ex_op(conn.detail.remote.path, conn.detail.local.path, working_path, data_path, _magic, "c", reserved_space));
                    if (op)
                    {
                        if (!paths.count(op->get_path_name()))
                        {
                            paths[op->get_path_name()] = true;
                            cop->_ops.insert(op);
                        }
                        cop->_all_ops.insert(op);
                    }              
                }
                break;
                case  saasame::transport::connection_type::LOCAL_FOLDER:
                {
                    if (conn.options.count("folder"))
                    {
                        irm_imagex_op::ptr op(new irm_imagex_local_op(conn.options["folder"], working_path, data_path, _magic, "c", reserved_space));
                        if (op)
                        {
                            if (!paths.count(op->get_path_name()))
                            {
                                paths[op->get_path_name()] = true;
                                cop->_ops.insert(op);
                            }
                            cop->_all_ops.insert(op);
                        }
                    }
                    else
                    {
                        irm_imagex_op::ptr op(new irm_imagex_local_op(conn.detail.local.path, working_path, data_path, _magic, "c", reserved_space));
                        if (op)
                        {
                            if (!paths.count(op->get_path_name()))
                            {
                                paths[op->get_path_name()] = true;
                                cop->_ops.insert(op);
                            }
                            cop->_all_ops.insert(op);
                        }
                    }
                }
                break;
                case saasame::transport::connection_type::NFS_FOLDER:
                {
                    static std::map<std::string, macho::windows::wnet_connection::ptr> g_connections;
                    static macho::windows::critical_section g_cs;
                    macho::windows::auto_lock lock(g_cs);
                    irm_imagex_op::ptr op;
                    if (g_connections.count(conn.detail.remote.path))
                        op.reset(new irm_imagex_nfs_op(g_connections[conn.detail.remote.path], working_path, data_path, _magic, "c"));
                    else
                    {
                        macho::windows::wnet_connection::ptr wnet = macho::windows::wnet_connection::connect(
                            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.path),
                            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.username),
                            macho::stringutils::convert_ansi_to_unicode(conn.detail.remote.password), false);
                        if (wnet)
                        {
                            g_connections[conn.detail.remote.path] = wnet;
                            op.reset(new irm_imagex_nfs_op(wnet, working_path, data_path, _magic, "c"));
                        }
                    }
                    if (op)
                    {
                        if (!paths.count(op->get_path_name()))
                        {
                            paths[op->get_path_name()] = true;
                            cop->_ops.insert(op);
                        }
                        cop->_all_ops.insert(op);
                    }
                }
                break;
#ifndef IRM_TRANSPORTER
                case saasame::transport::connection_type::S3_BUCKET:
                {
                    aws_s3::ptr s3 = aws_s3::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.s3_region, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
                    if (s3)
                    {
                        irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_s3_op(s3, working_path, data_path, _magic, "c"));
                        if (op)
                        {
                            if (!paths.count(op->get_path_name()))
                            {
                                paths[op->get_path_name()] = true;
                                cop->_ops.insert(op);
                            }
                            cop->_all_ops.insert(op);
                        }
                    }
                }
                break;
#endif
                case saasame::transport::connection_type::WEBDAV_WITH_SSL:
                {
                    webdav::ptr dav = webdav::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.port, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
                    if (dav)
                    {
                        irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_webdav_op(dav, working_path, data_path, _magic, "c"));
                        if (op)
                        {
                            if (!paths.count(op->get_path_name()))
                            {
                                paths[op->get_path_name()] = true;
                                cop->_ops.insert(op);
                            }
                            cop->_all_ops.insert(op);
                        }
                    }
                }
                break;
                case saasame::transport::connection_type::WEBDAV_EX:
                {
                    webdav::ptr dav = webdav::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.port, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
                    if (dav)
                    {
                        irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_webdav_ex_op(dav, conn.detail.local.path, working_path, data_path, _magic, "c", reserved_space));
                        if (op)
                        {
                            if (!paths.count(op->get_path_name()))
                            {
                                paths[op->get_path_name()] = true;
                                cop->_ops.insert(op);
                            }
                            cop->_all_ops.insert(op);
                        }
                    }
                }
                break;
#ifndef IRM_TRANSPORTER
                case saasame::transport::connection_type::S3_BUCKET_EX:
                {
                    aws_s3::ptr s3 = aws_s3::connect(conn.detail.remote.path, conn.detail.remote.username, conn.detail.remote.password, conn.detail.remote.s3_region, conn.detail.remote.proxy_host, conn.detail.remote.proxy_port, conn.detail.remote.proxy_username, conn.detail.remote.proxy_password, conn.detail.remote.timeout);
                    if (s3)
                    {
                        irm_imagex_op::ptr op = irm_imagex_op::ptr(new irm_imagex_s3_ex_op(s3, conn.detail.local.path, working_path, data_path, _magic, "c", reserved_space));
                        if (op)
                        {
                            if (!paths.count(op->get_path_name()))
                            {
                                paths[op->get_path_name()] = true;
                                cop->_ops.insert(op);
                            }
                            cop->_all_ops.insert(op);
                        }
                    }
                }
#endif
            }
        }
        if (cop->_ops.empty())
            return NULL;
    }
    return cop;
}

irm_imagex_carrier_op::irm_imagex_carrier_op(boost::filesystem::path working_path, boost::filesystem::path data_path) 
    : irm_imagex_local_op("", working_path, data_path)
{
}

irm_imagex_carrier_op::~irm_imagex_carrier_op()
{
    _ops.clear();
}

bool irm_imagex_carrier_op::read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data){
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->read_file(file, data)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->write_file(file, data, forcefull)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::read_file(__in boost::filesystem::path file, __inout std::ostream& data)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->read_file(file, data)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::write_file(__in boost::filesystem::path file, __in std::istream& data)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->write_file(file, data)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::remove_file(__in boost::filesystem::path file)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->remove_file(file)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::is_file_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->is_file_existing(file)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::is_metafile_existing(__in boost::filesystem::path file)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->is_metafile_existing(file)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool local_only)
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
            if (_ops.empty())
                result = false;

            foreach(irm_imagex_op::ptr op, _ops)
            {
                if (!(result = op->write_metafile(file, data, local_only)))
                    break;
            }
        }
    }
    catch (...)
    {
        result = false;
    }

    return result;
}


bool irm_imagex_carrier_op::write_metafile(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_imagex_base& data, bool local_only)
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
            if (_ops.empty())
                result = false;

            foreach(irm_imagex_op::ptr op, _ops)
            {
                if (!(result = op->write_metafile(file, data, local_only)))
                    break;
            }
        }
    }
    catch (...)
    {
        result = false;
    }

    return result;
}

bool irm_imagex_carrier_op::remove_metafiles(__in std::vector<boost::filesystem::path>& files)
{
    FUN_TRACE;

    bool result = true;
    
    flush_metafiles(files);

    foreach(boost::filesystem::path& file, files)
    {
        result = _remove_file(_working_path, file);
        if (!result)
            return result;
    }
    return result;
}

bool irm_imagex_carrier_op::flush_metafiles(__in std::vector<boost::filesystem::path>& files)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty() || files.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->flush_metafiles(files)))
            break;
    }

    return result;
}

macho::windows::lock_able::vtr irm_imagex_carrier_op::get_lock_objects(std::wstring& lock_filename)
{
    FUN_TRACE;

    macho::windows::lock_able::vtr locks;

    if (_ops.empty())
        return locks;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        foreach(macho::windows::lock_able::ptr lock, op->get_lock_objects(lock_filename))
        {
            locks.push_back(lock);
        }
    }

    return locks;
}

bool irm_imagex_carrier_op::remove_image(__in std::wstring& image_name)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _all_ops)
    {
        if (!(result = op->remove_image(image_name)))
            break;
    }

    return result;
}

bool irm_imagex_carrier_op::release_image_lock(__in std::wstring& image_name)
{
    FUN_TRACE;

    bool result = true;

    if (_ops.empty())
        result = false;

    foreach(irm_imagex_op::ptr op, _ops)
    {
        if (!(result = op->release_image_lock(image_name)))
            break;
    }

    return result;
}

std::wstring irm_imagex_carrier_op::get_path_name() const
{
    FUN_TRACE;

    std::wstring path_name = L"";

    if (_ops.empty())
        return L"";

    foreach(irm_imagex_op::ptr op, _ops)
    {
        path_name.append(op->get_path_name()).append(L";");
    }

    if (!path_name.empty())
        path_name.resize(path_name.size() - 1);

    return path_name;
}

void irm_imagex_carrier_op::cancel()
{
    FUN_TRACE;
    foreach(irm_imagex_op::ptr op, _all_ops)
    {
        op->cancel();
    }
    irm_imagex_op::cancel();
}
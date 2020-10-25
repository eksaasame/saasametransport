#pragma once

#ifndef __IRM_IMAGEX_OP__
#define __IRM_IMAGEX_OP__

#include "..\irm_imagex\irm_imagex.h"
#include "..\irm_carrier\carrier_service_handler.h"
#include "..\gen-cpp\webdav.h"
#ifndef IRM_TRANSPORTER
#include "..\gen-cpp\aws_s3.h"
#endif
#define MAGIC 5

class irm_imagex_local_op : virtual public saasame::ironman::imagex::irm_imagex_op
{
public:
    typedef boost::shared_ptr<irm_imagex_local_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_local_op(boost::filesystem::path output_path, boost::filesystem::path working_path = macho::windows::environment::get_working_directory(), boost::filesystem::path data_path = macho::windows::environment::get_working_directory(), uint16_t magic = MAGIC, std::string flag = "", boost::uintmax_t reserved_space = 0) : _output_path(output_path), _working_path(working_path), _data_path(data_path), _magic(magic), _flag(flag), _reserved_space(reserved_space), irm_imagex_op()
    { 
    };
    virtual ~irm_imagex_local_op(){}
    virtual bool is_file_existing(__in boost::filesystem::path file);
    bool read_file(__in boost::filesystem::path file, __inout std::ifstream& ifs);
    virtual bool read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data);
    virtual bool write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull = false);

    virtual bool read_file(__in boost::filesystem::path file, __inout std::ostream& data);
    virtual bool write_file(__in boost::filesystem::path file, __in std::istream& data);
    virtual bool remove_file(__in boost::filesystem::path file);

    virtual bool is_temp_file_existing(__in boost::filesystem::path file);
    virtual bool read_temp_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data);
    virtual bool write_temp_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data);
    virtual bool read_temp_file(__in boost::filesystem::path file, __inout std::ostream& data);
    virtual bool write_temp_file(__in boost::filesystem::path file, __in std::istream& data);
    virtual bool remove_temp_file(__in boost::filesystem::path file);

    virtual bool is_metafile_existing(__in boost::filesystem::path file);
    virtual bool read_metafile(__in boost::filesystem::path file, __inout std::stringstream& data);
    virtual bool write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool only_local);

    virtual bool read_metafile(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data);
    virtual bool write_metafile(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_imagex_base& data, bool local_only);

    virtual bool remove_metafile(__in boost::filesystem::path file);
    virtual bool remove_metafiles(__in std::vector<boost::filesystem::path>& files);
    virtual bool flush_metafiles(__in std::vector<boost::filesystem::path>& files);
    virtual bool remove_image(__in std::wstring& image_name);
    virtual bool release_image_lock(__in std::wstring& image_name);
    virtual macho::windows::lock_able::vtr get_lock_objects(std::wstring& lock_filename);
    virtual std::wstring get_path_name() const;
    
    //For repeat job usage
    virtual bool read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_imagex_base& data);
    virtual bool write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_imagex_base& data);
    virtual macho::windows::lock_able::ptr get_lock_object(std::wstring& lock_filename, std::string flag);
protected:
    bool _read_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __inout std::ifstream& ifs);

    bool _read_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __inout std::ostream& data);
    bool _write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data);

    bool _read_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __inout saasame::ironman::imagex::irm_imagex_base& data);
    bool _write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in saasame::ironman::imagex::irm_imagex_base& data, bool forcefull);

    bool _remove_file(__in const boost::filesystem::path& path, __in boost::filesystem::path& filename);
    virtual bool _delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data);
    virtual bool _delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in saasame::ironman::imagex::irm_imagex_base& data);

    bool _remove_image(__in const boost::filesystem::path &dir, std::wstring& image_name);
    boost::filesystem::path _output_path;
    boost::filesystem::path _working_path;
    boost::filesystem::path _data_path;
    std::map<boost::filesystem::path, uint16_t>  _wrt_count;
    uint16_t                _magic;
    std::string             _flag;
    boost::uintmax_t        _reserved_space;
private:
};

class irm_imagex_local_ex_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_local_ex_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_local_ex_op(boost::filesystem::path remote_path, boost::filesystem::path local_path, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, boost::uintmax_t reserved_space);
    virtual ~irm_imagex_local_ex_op();
    virtual bool remove_image(__in std::wstring& image_name);
protected:
private:
    boost::filesystem::path                      _remote_path;
};

class irm_imagex_carrier_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_carrier_op> ptr;
    typedef std::vector<ptr> vtr;
    virtual ~irm_imagex_carrier_op();
    virtual bool read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data);
    virtual bool write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull);
    virtual bool read_file(__in boost::filesystem::path file, __inout std::ostream& data);
    virtual bool write_file(__in boost::filesystem::path file, __in std::istream& data);
    virtual bool remove_file(__in boost::filesystem::path file);
    virtual bool is_file_existing(__in boost::filesystem::path file);
    virtual bool is_metafile_existing(__in boost::filesystem::path file);
    virtual bool write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool only_local);
    virtual bool write_metafile(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_imagex_base& data, bool local_only);
    virtual bool remove_metafiles(__in std::vector<boost::filesystem::path>& files);
    virtual bool flush_metafiles(__in std::vector<boost::filesystem::path>& files);
    virtual bool remove_image(__in std::wstring& image_name);
    virtual bool release_image_lock(__in std::wstring& image_name);
    virtual macho::windows::lock_able::vtr get_lock_objects(std::wstring& lock_filename);
    virtual std::wstring get_path_name() const;
    virtual void cancel();
    static irm_imagex_carrier_op::ptr get(std::set<connection>& conns, boost::filesystem::path working_path = macho::windows::environment::get_working_directory(), boost::filesystem::path data_path = macho::windows::environment::get_working_directory());
private:
    //Hiding in Inheritance
    using irm_imagex_local_op::read_file;
    using irm_imagex_local_op::write_file;
    using irm_imagex_local_op::get_lock_object;
    irm_imagex_carrier_op(boost::filesystem::path working_path, boost::filesystem::path data_path); 
    std::set<irm_imagex_op::ptr>    _ops;
    std::set<irm_imagex_op::ptr>    _all_ops;
};

class irm_imagex_nfs_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_nfs_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_nfs_op(macho::windows::wnet_connection::ptr wnet, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag );
    virtual ~irm_imagex_nfs_op();

private:
    macho::windows::wnet_connection::ptr    _wnet;
};

class irm_imagex_webdav_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_webdav_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_webdav_op(webdav::ptr dav, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, uint32_t queue_size = 2);
    virtual ~irm_imagex_webdav_op();
    virtual bool read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data);
    virtual bool write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull);
    virtual bool read_file(__in boost::filesystem::path file, __inout std::ostream& data);
    virtual bool write_file(__in boost::filesystem::path file, __in std::istream& data);
    virtual bool remove_file(__in boost::filesystem::path file);
    virtual bool is_file_existing(__in boost::filesystem::path file);
    virtual bool write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool only_local);
    virtual bool write_metafile(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_imagex_base& data, bool local_only);
    virtual bool flush_metafiles(__in std::vector<boost::filesystem::path>& files);
    virtual bool remove_image(__in std::wstring& image_name);
    virtual macho::windows::lock_able::vtr get_lock_objects(std::wstring& lock_filename);
    virtual std::wstring get_path_name() const;
protected:
private:
    using irm_imagex_local_op::read_file;
    using irm_imagex_local_op::write_file;
    using irm_imagex_local_op::get_lock_object;
    virtual bool _delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data);
    webdav::ptr                      _pop_front();
    void                             _push_front(webdav::ptr dav);
    webdav::ptr                      _dav;
    webdav::queue                    _queue;
    macho::windows::critical_section _cs;
};

class irm_imagex_webdav_ex_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_webdav_ex_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_webdav_ex_op(webdav::ptr dav, boost::filesystem::path local_path, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, boost::uintmax_t reserved_space);
    virtual ~irm_imagex_webdav_ex_op();
    virtual bool remove_image(__in std::wstring& image_name);
protected:
private:
    webdav::ptr                      _dav;
};

#ifndef IRM_TRANSPORTER
class irm_imagex_s3_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_s3_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_s3_op(aws_s3::ptr s3, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, uint32_t queue_size = 2);
    virtual ~irm_imagex_s3_op();
    virtual bool read_file(__in boost::filesystem::path file, __inout saasame::ironman::imagex::irm_transport_image_block& data);
    virtual bool write_file(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_transport_image_block& data, bool forcefull);
    virtual bool read_file(__in boost::filesystem::path file, __inout std::ostream& data);
    virtual bool write_file(__in boost::filesystem::path file, __in std::istream& data);
    virtual bool remove_file(__in boost::filesystem::path file);
    virtual bool is_file_existing(__in boost::filesystem::path file);
    virtual bool write_metafile(__in boost::filesystem::path file, __in std::stringstream& data, bool only_local);
    virtual bool write_metafile(__in boost::filesystem::path file, __in saasame::ironman::imagex::irm_imagex_base& data, bool local_only);
    virtual bool flush_metafiles(__in std::vector<boost::filesystem::path>& files);
    virtual bool remove_image(__in std::wstring& image_name);
    virtual macho::windows::lock_able::vtr get_lock_objects(std::wstring& lock_filename);
    virtual std::wstring get_path_name() const;
protected:
private:
    using irm_imagex_local_op::read_file;
    using irm_imagex_local_op::write_file;
    using irm_imagex_local_op::get_lock_object;
    virtual bool _delay_write_file(__in boost::filesystem::path& path, __in boost::filesystem::path& filename, __in std::istream& data);
    aws_s3::ptr                      _pop_front();
    void                             _push_front(aws_s3::ptr dav);
    aws_s3::ptr                      _s3;
    aws_s3::queue                    _queue;
    macho::windows::critical_section _cs;
};

class irm_imagex_s3_ex_op : virtual public irm_imagex_local_op
{
public:
    typedef boost::shared_ptr<irm_imagex_s3_ex_op> ptr;
    typedef std::vector<ptr> vtr;

    irm_imagex_s3_ex_op(aws_s3::ptr s3, boost::filesystem::path local_path, boost::filesystem::path working_path, boost::filesystem::path data_path, uint16_t magic, std::string flag, boost::uintmax_t reserved_space);
    virtual ~irm_imagex_s3_ex_op();
    virtual bool remove_image(__in std::wstring& image_name);
protected:
private:
    aws_s3::ptr                      _s3;
};
#endif

#endif
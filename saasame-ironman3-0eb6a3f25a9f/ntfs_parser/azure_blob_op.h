#pragma once

#ifndef _AZURE_UPLOAD_H_
#define _AZURE_UPLOAD_H_ 1
#ifdef _AZURE_BLOB
#include "macho.h"
#include "..\gen-cpp\universal_disk_rw.h"

class azure_page_blob_op{
public:
    typedef boost::shared_ptr<azure_page_blob_op> ptr;
    struct blob_snapshot{
        typedef std::vector<blob_snapshot> vtr;
        std::string name;
        std::string id;
        std::string date_time;
    };
    
    static azure_page_blob_op::ptr open(std::string connection_string);
    static bool                   create_vhd_page_blob(std::string connection_string, std::string container, std::string blob_name, uint64_t size, std::string comment = std::string("base"));
    static bool                   delete_vhd_page_blob(std::string connection_string, std::string container, std::string blob_name);
    static bool                   create_vhd_page_blob_snapshot(std::string connection_string, std::string container, std::string blob_name, std::string snapshot_name);
    static bool                   delete_vhd_page_blob_snapshot(std::string connection_string, std::string container, std::string blob_name, std::string snapshot_name);

    static blob_snapshot::vtr     get_vhd_page_blob_snapshots(std::string connection_string, std::string container, std::string blob_name);

    static bool                   create_vhd_page_blob_from_snapshot(std::string connection_string, std::string container, std::string source_blob_name, std::string snapshot, std::string target_blob_name);
    static bool                   change_vhd_blob_id(std::string connection_string, std::string container, std::string blob_name, macho::guid_ new_id);
    static universal_disk_rw::ptr open_vhd_page_blob_for_rw(std::string connection_string, std::string container, std::string blob_name);
    static io_range::vtr		  get_page_ranges(std::string connection_string, std::string container, std::string blob_name);
    static uint64_t               get_vhd_page_blob_size(std::string connection_string, std::string container, std::string blob_name);

    virtual bool                   create_vhd_page_blob(std::string container, std::string blob_name, uint64_t size, std::string comment = std::string()) = 0;
    virtual bool                   delete_vhd_page_blob(std::string container, std::string blob_name) = 0;
    virtual bool                   create_vhd_page_blob_snapshot(std::string container, std::string blob_name, std::string snapshot_name) = 0;
    virtual bool                   delete_vhd_page_blob_snapshot(std::string container, std::string blob_name, std::string snapshot_name) = 0;
    virtual blob_snapshot::vtr     get_vhd_page_blob_snapshots(std::string container, std::string blob_name) = 0;
    virtual bool                   create_vhd_page_blob_from_snapshot(std::string container, std::string source_blob_name, std::string snapshot, std::string target_blob_name) = 0;
    virtual bool                   change_vhd_blob_id(std::string container, std::string blob_name, macho::guid_ new_id) = 0;
    virtual universal_disk_rw::ptr open_vhd_page_blob_for_rw(std::string container, std::string blob_name) = 0;
    virtual io_range::vtr		   get_page_ranges(std::string container, std::string blob_name) = 0;
    virtual uint64_t               get_vhd_page_blob_size(std::string container, std::string blob_name) = 0;
};

#endif
#endif

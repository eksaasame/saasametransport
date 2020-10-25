#pragma once

#ifndef __LICENSE_DB_SQLITE
#define __LICENSE_DB_SQLITE

#include "..\sqlite\sqlite3.h"
#include "windows.h"
#include <string>
#include <vector>
#include <map>
#include <macho.h>
#include <boost\shared_ptr.hpp>

#pragma comment(lib, "sqlite.lib")

namespace saasame { namespace transport {
struct license{
    typedef boost::shared_ptr<license> ptr;
    typedef std::vector<ptr>            vtr;
    license() : index(0){
    }
    uint64_t                index;
    std::string             key;
    std::string             lic;
    std::string             active;
    std::string             name;
    std::string             email;
    std::string             status;
    std::string             comment;
};

struct private_key{
    typedef boost::shared_ptr<private_key> ptr;
    private_key() : index(0){
    }
    uint64_t                index;
    std::string             key;
    std::string             comment;
};

struct workload{
    typedef boost::shared_ptr<workload> ptr;
    typedef std::vector<ptr>            vtr;
    typedef std::map<std::string, workload::vtr > map;
    workload() : index(0), created(0), deleted(0), updated(0){
    }
    uint64_t                index;
    std::string             host;
    std::time_t             created;
    std::time_t             deleted;
    std::time_t             updated;
    std::string             type;
    std::string             name;
    std::string             comment;
};

class license_db{
public:
    typedef boost::shared_ptr<license_db> ptr;
    license_db() : _db(NULL),
        _select_private_key_stmt(NULL),
        _insert_private_key_stmt(NULL),
        _select_licenses_stmt(NULL),
        _select_license_stmt(NULL),
        _insert_license_stmt(NULL),
        _select_workloads_stmt(NULL),
        _select_workload_stmt(NULL),
        _select_workload_stmt2(NULL),
        _insert_workload_stmt(NULL)
    {
    }
    static license_db::ptr open(std::string path, std::string password = "");
    bool save(std::string path, std::string password = "");
    private_key::ptr get_private_key();
    bool put_private_key(const private_key& key);
    license::vtr get_licenses();
    license::ptr get_license(const std::string key);

    bool insert_license(license &lic);
    bool update_license(const license &lic);

    workload::vtr get_workloads();
    workload::map get_workloads_map();
    bool insert_workload(workload &host);
    bool update_workload(const workload &host);

    workload::vtr get_workload(const std::string host);
    workload::ptr get_workload(const std::string host, const std::time_t created);

    void close();
    virtual ~license_db(){ close(); }
private:
    bool _open(std::string path, std::string password = "");
    bool initial();
    macho::windows::critical_section   _cs;
    sqlite3*                           _db;
    sqlite3_stmt*                      _select_private_key_stmt;
    sqlite3_stmt*                      _insert_private_key_stmt;
    sqlite3_stmt*                      _select_licenses_stmt;
    sqlite3_stmt*                      _select_license_stmt;
    sqlite3_stmt*                      _insert_license_stmt;
    sqlite3_stmt*                      _update_license_stmt;
    sqlite3_stmt*                      _select_workloads_stmt;
    sqlite3_stmt*                      _select_workload_stmt;
    sqlite3_stmt*                      _select_workload_stmt2;
    sqlite3_stmt*                      _insert_workload_stmt;
    sqlite3_stmt*                      _update_workload_stmt;
};

};};

#endif
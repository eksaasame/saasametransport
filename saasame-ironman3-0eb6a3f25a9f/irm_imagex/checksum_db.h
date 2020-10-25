#pragma once

#ifndef __CHECKSUM_DB_SQLITE
#define __CHECKSUM_DB_SQLITE

#include "..\sqlite\sqlite3.h"
#include "windows.h"
#include <string>
#include <vector>
#include <map>
#include <macho.h>
#include <boost\shared_ptr.hpp>

#pragma comment(lib, "sqlite.lib")

struct chechsum{
    typedef boost::shared_ptr<chechsum> ptr;
    typedef std::vector<ptr>            vtr;
    chechsum() : index(0), crc(0), validated(true){
    }
    int                     index;
    std::string             bitmap;
    std::string             md5;
    uint32_t                crc;
    bool                    validated;
};

class checksum_db{
public:
    typedef boost::shared_ptr<checksum_db> ptr;
    checksum_db() : _db(NULL), _select_stmt(NULL), _insert_stmt(NULL){
    }
    bool open(std::string path);
    bool load(std::string path);
    bool merge(std::string path);
    bool save(std::string path);
    bool put(const chechsum &blk);
    chechsum::ptr get(int index);
    chechsum::vtr get_all();
    void close();
    virtual ~checksum_db(){ close(); }
private:
    bool initial();
    macho::windows::critical_section   _cs;
    sqlite3*                           _db;
    sqlite3_stmt*                      _select_stmt;
    sqlite3_stmt*                      _insert_stmt;
};


#endif
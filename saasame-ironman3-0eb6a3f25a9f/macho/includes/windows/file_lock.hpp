// file_lock.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_FILE_LOCK_HEADER__
#define __MACHO_WINDOWS_FILE_LOCK_HEADER__
#include <Windows.h>
#include "boost\filesystem.hpp"
#include "lock.hpp"
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>

namespace macho{
namespace windows{

class file_lock : virtual public lock_able {
public:
    file_lock(boost::filesystem::path file): _lock(false){
        _lock_file = file.wstring() + L".lock";
    }
    ~file_lock(){
        unlock();
    }
    bool lock(){
        if (!_lock){
            while (!(_lock = (0 == _wmkdir(_lock_file.wstring().c_str()))))
                Sleep(100);
        }
        return _lock;
    }
    bool unlock(){
        if (_lock)
            _lock = !(0 == _wrmdir(_lock_file.wstring().c_str()));
        return !_lock;
    }
    bool trylock(){
        _lock = ( 0 == _wmkdir(_lock_file.wstring().c_str()));
        return _lock;
    }
private:
    boost::filesystem::path _lock_file;
    bool                    _lock;
};

class file_lock_ex : virtual public lock_able_ex {
public:
    file_lock_ex(boost::filesystem::path file, std::string flag, int wait_count = 10000) : _lock(false), _flag(false), _count(wait_count){
        _lock_file = file.wstring() + L".lock";
        if (flag.length())
            _flag_file = _lock_file / flag;
    }
    ~file_lock_ex(){
        unlock();
    }
    bool lock(){
        if (!_lock){
            bool _check_flag = false;
            int  count = _count;
            while (!(_lock = (0 == _wmkdir(_lock_file.wstring().c_str())))){
                if (!_check_flag && !_flag_file.empty()){
                    if (_flag = (0 == _waccess(_flag_file.wstring().c_str(), 0))){
                        _lock = true;
                        break;
                    }
                    _check_flag = true;
                }
                if (is_canceled()){
                    BOOST_THROW_EXCEPTION_BASE_STRING(lock_able::exception, boost::str(boost::wformat(L"Cancelled lock file (%s).") % _lock_file.wstring()));
                }
                else if (count){
                    Sleep(100);
                    count--;
                }
                else{
                    BOOST_THROW_EXCEPTION_BASE_STRING(lock_able::exception, boost::str(boost::wformat(L"Lock file (%s) time out.") % _lock_file.wstring()));
                }
            }
            if (!_flag && !_flag_file.empty())
                _flag = (0 == _wmkdir(_flag_file.wstring().c_str()));
        }
        return _lock;
    }
    bool unlock(){
        if (_lock){
            if (_flag && !_flag_file.empty())
                _flag = !(0 == _wrmdir(_flag_file.wstring().c_str()));
            _lock = !(0 == _wrmdir(_lock_file.wstring().c_str()));
        }
        return !_lock;
    }
    bool trylock(){
        _lock = (0 == _wmkdir(_lock_file.wstring().c_str()));
        if (!_flag_file.empty()){
            if (_lock)
                _flag = (0 == _wmkdir(_flag_file.wstring().c_str()));
            else if (_flag = (0 == _waccess(_flag_file.wstring().c_str(), 0)))
                _lock = true;
        }
        return _lock;
    }
private:
    boost::filesystem::path _lock_file;
    bool                    _lock;
    boost::filesystem::path _flag_file;
    bool                    _flag;
    int                     _count;
};

};//namespace windows
};//namespace macho

#endif
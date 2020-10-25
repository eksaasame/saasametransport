// lock.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_LOCK_HEADER__
#define __MACHO_WINDOWS_LOCK_HEADER__
#include <Windows.h>
#include "common\exception_base.hpp"
#include <boost\signals2.hpp>

namespace macho{

namespace windows{

class lock_able{
public:
    typedef boost::shared_ptr<lock_able> ptr;
    typedef std::vector<ptr> vtr;
    struct  exception : virtual public macho::exception_base {};
    virtual bool lock()      = 0;
    virtual bool unlock()    = 0;
    virtual bool trylock()   = 0;
};

class lock_able_ex : virtual public lock_able{
public:
    typedef boost::shared_ptr<lock_able> ptr;
    typedef std::vector<ptr> vtr;
    typedef boost::signals2::signal<bool(), _or<bool>> be_canceled;
    inline void register_is_cancelled_function(be_canceled::slot_type slot){
        is_canceled.connect(slot);
    }
protected:
    be_canceled              is_canceled;
};

class critical_section : virtual public lock_able {
public:
    critical_section(){ InitializeCriticalSectionAndSpinCount( &_section, 0x40 ) ; }
    bool lock(){ EnterCriticalSection(&_section); return true; }
    bool unlock(){ LeaveCriticalSection(&_section); return true; }
    bool trylock(){  return ( TRUE == TryEnterCriticalSection(&_section) ) ;  }
    ~critical_section(){ DeleteCriticalSection(&_section); }
private:
    CRITICAL_SECTION _section;
};

class auto_lock{
public:
    auto_lock(lock_able& lock) : _lock(lock) { _lock.lock(); }
    ~auto_lock() { _lock.unlock(); }
private:
    lock_able& _lock;
};

class auto_locks{
public:
    auto_locks(lock_able::vtr& locks) : _locks(locks) {
        foreach(lock_able::ptr &_lock, _locks)
            _lock->lock(); 
    }
    ~auto_locks() {
        foreach(lock_able::ptr &_lock, _locks)
            _lock->unlock();
    }
private:
    lock_able::vtr& _locks;
};

};//namespace windows
};//namespace macho

#endif


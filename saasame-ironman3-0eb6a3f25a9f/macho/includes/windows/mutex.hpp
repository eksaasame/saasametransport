// mutex.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_MUTEX_HEADER__
#define __MACHO_WINDOWS_MUTEX_HEADER__
#include <Windows.h>
#include "..\common\exception_base.hpp"
#include "..\common\stringutils.hpp"
#include "..\common\tracelog.hpp"
#include "lock.hpp"

namespace macho{

namespace windows{

 // By design, we limit mutex and semaphore cannot be used as a global object between each threads. 
 // For this purpose, you need to use a naming mutex or semaphore to control them.

struct    mutex_exception : virtual public exception_base {};
#define BOOST_THROW_MUTEX_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( mutex_exception, no, message )

class mutex : virtual public lock_able {
protected:
    HANDLE        _mutex;
    stdstring     _name;
    bool          _lock;
public:
    mutex(stdstring name = stdstring(), bool open_only = false) :_mutex(NULL), _lock(false){
        _name = name;
        if ( open_only )
            _mutex = OpenMutex( NULL, FALSE, _name.c_str() );
        else if ( name.length() )
            _mutex = CreateMutex( NULL, FALSE, _name.c_str() );
        else 
            _mutex = CreateMutex( NULL, FALSE, NULL );
        if ( ( NULL == _mutex ) || ( INVALID_HANDLE_VALUE == _mutex ) ){
            BOOST_THROW_MUTEX_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), stringutils::format( _T("Can't %s mutex (%s)."), ( open_only ? L"open" : L"create"), name.c_str()) ) ;
        }
    }

    ~mutex(void){
        remove();
    }

    bool trylock(void){
        return lock( 0 ); 
    }

    bool lock(void){
        return lock( INFINITE ); 
    }
    bool lock( DWORD timeout ){
        bool bResult = _lock;
        if ((!_lock) && (_mutex) && (_mutex != INVALID_HANDLE_VALUE)){
            DWORD dwWait = WaitForSingleObject( _mutex, timeout );
            switch( dwWait ){
            case WAIT_OBJECT_0:
                LOG( LOG_LEVEL_TRACE,_T("Locked mutex (%s)."), _name.c_str() );
                _lock = bResult = true;
                break;
            case WAIT_ABANDONED:
                LOG( LOG_LEVEL_TRACE,_T("Mutex (%s) was not released properly by a terminating thread."), _name.c_str() );
                break;
            case WAIT_TIMEOUT:
                LOG( LOG_LEVEL_TRACE,_T("The time-out interval (%d millisec) elapsed for mutex (%s)."), timeout, _name.c_str());
                break;
            case WAIT_FAILED:
                BOOST_THROW_MUTEX_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), stringutils::format( _T("Cannot wait for single object on mutex (%s)." ),_name.c_str() ) ) ;
                break;
            }
        }
        return bResult;
    }

    bool unlock(void){
        if (_lock && (_mutex) && (_mutex != INVALID_HANDLE_VALUE)){
            if (ReleaseMutex(_mutex)){
                _lock = false;
                return true;
            }
            else
                BOOST_THROW_MUTEX_EXCEPTION( HRESULT_FROM_WIN32(GetLastError()), stringutils::format( _T("Cannot release mutex (%s)."), _name.c_str() ) ) ;
        }
        return false;
    }
private:
    bool remove(void){
        if ( ( _mutex ) && ( _mutex != INVALID_HANDLE_VALUE ) ){
            if (_lock)
                ReleaseMutex( _mutex );
            CloseHandle( _mutex );
            _mutex = NULL;
        }
        return true;
    }
};

class semaphore : virtual public lock_able {

protected:
    HANDLE        _semaphore;
    stdstring     _name;
    bool          _lock;
public:
    semaphore(stdstring name = stdstring(), uint32_t maximum_count = 1, bool open_only = false) :_semaphore(NULL), _lock(false){
        _name = name;
        if (!maximum_count)
            maximum_count = 1;
        if (open_only)
            _semaphore = OpenSemaphore(NULL, FALSE, _name.c_str());
        else if (name.length())
            _semaphore = CreateSemaphore(NULL, maximum_count, maximum_count, _name.c_str());
        else
            _semaphore = CreateSemaphore(NULL, maximum_count, maximum_count, NULL);
        if ((NULL == _semaphore) || (INVALID_HANDLE_VALUE == _semaphore)){
            BOOST_THROW_MUTEX_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()), stringutils::format(_T("Cannot %s semaphore (%s)."), (open_only ? L"open" : L"create"), name.c_str()));
        }
    }
    
    bool trylock(void){
        return lock(0);
    }

    bool lock(void){
        return lock(INFINITE);
    }

    bool lock(DWORD timeout){
        bool bResult = _lock;
        if ((!_lock) && (_semaphore) && (_semaphore != INVALID_HANDLE_VALUE)){
            DWORD dwWait = WaitForSingleObject(_semaphore, timeout);
            switch (dwWait){
            case WAIT_OBJECT_0:
                LOG(LOG_LEVEL_TRACE, _T("Locked semaphore (%s)."), _name.c_str());
                _lock = bResult = true;
                break;
            case WAIT_ABANDONED:
                LOG(LOG_LEVEL_TRACE, _T("Semaphore (%s) was not released properly by a terminating thread."), _name.c_str());
                break;
            case WAIT_TIMEOUT:
                LOG(LOG_LEVEL_TRACE, _T("The time-out interval (%d millisec) elapsed for semaphore (%s)."), timeout, _name.c_str());
                break;
            case WAIT_FAILED:
                BOOST_THROW_MUTEX_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()), stringutils::format(_T("Cannot wait for single object on semaphore (%s)."), _name.c_str()));
                break;
            }
        }
        return bResult;
    }

    bool unlock(void){
        if (_lock && (_semaphore) && (_semaphore != INVALID_HANDLE_VALUE)){
            if (ReleaseSemaphore(_semaphore, 1, NULL)){
                _lock = false;
                return true;
            }
            else
                BOOST_THROW_MUTEX_EXCEPTION(HRESULT_FROM_WIN32(GetLastError()), stringutils::format(_T("Cannot release semaphore (%s)."), _name.c_str()));
        }
        return false;
    }

    ~semaphore(void){
        remove();
    }

private:
    bool remove(void){
        if ((_semaphore) && (_semaphore != INVALID_HANDLE_VALUE)){
            if (_lock)
                ReleaseSemaphore(_semaphore, 1, NULL);
            CloseHandle(_semaphore);
            _semaphore = NULL;
        }
        return true;
    }
};
};//namespace windows
};//namespace macho

#endif
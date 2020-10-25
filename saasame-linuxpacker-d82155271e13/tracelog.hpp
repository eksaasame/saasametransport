// tracelog.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_TRACELOG__
#define __MACHO_TRACELOG__

#include "..\config\config.hpp"
#include "stringutils.hpp"
#include "..\windows\lock.hpp"
#include "boost\format.hpp"
#include <assert.h>
#include <stdio.h>
#include <tchar.h>
#include <sys/stat.h>
#include <time.h>
#include "boost\filesystem.hpp"

namespace macho{

typedef enum TRACE_LOG_LEVEL {
    LOG_LEVEL_NONE          = 0L,
    LOG_LEVEL_ERROR         = 1L,
    LOG_LEVEL_WARNING       = 2L,
    LOG_LEVEL_NOTICE        = 3L,
    LOG_LEVEL_INFO          = 4L,
    LOG_LEVEL_DEBUG         = 5L,
    LOG_LEVEL_INTERNAL      = 6L,
    LOG_LEVEL_TRACE         = 10L,
    LOG_LEVEL_RECORD        = -1L //a special log level, it means something must be recorded
};

#ifdef MACHO_GLOBAL_TRACE
 
void log( TRACE_LOG_LEVEL level, LPCTSTR fmt, ... );
bool is_log( TRACE_LOG_LEVEL level );
void set_log_file( stdstring logfile );
void set_log_level( TRACE_LOG_LEVEL level );
stdstring get_log_file();
TRACE_LOG_LEVEL get_log_level();

#ifdef _UNICODE

#ifdef _DEBUG
#define LOG( level, fmt, ... )                                                  \
   do{                                                                          \
       if ( ( level == LOG_LEVEL_RECORD ) || is_log(level) ) {                  \
           boost::wformat wformat1(_T("%s(%d) : %s - %s "));                    \
           wformat1 %_T(__FILE__) %__LINE__ %_T(__FUNCTION__) %fmt ;            \
           log( level, boost::str( wformat1 ).c_str(), ##__VA_ARGS__ );         \
       }                                                                        \
   }while(0)

#else
#define LOG( level, fmt, ... )                                                  \
   do{                                                                          \
       if ( ( level == LOG_LEVEL_RECORD ) || is_log(level) ) {                  \
           boost::wformat wformat1(_T("%s - %s "));                             \
           wformat1 %_T(__FUNCTION__) %fmt ;                                    \
           log( level, boost::str( wformat1 ).c_str(), ##__VA_ARGS__ );         \
              }                                                                 \
      }while(0)

#endif

#else

#ifdef _DEBUG

#define LOG( level, fmt, ... )                                                  \
   do{                                                                          \
       if ( ( level == LOG_LEVEL_RECORD ) || is_log(level) ) {                  \
           boost::format format1(_T("%s(%d) : %s - %s "));                      \
           format1 %_T(__FILE__) %__LINE__ %_T(__FUNCTION__) %fmt ;             \
           log( level, boost::str( format1 ).c_str(), ##__VA_ARGS__ );          \
       }                                                                        \
    }while(0)

#else

#define LOG( level, fmt, ... )                                                  \
   do{                                                                          \
       if ( ( level == LOG_LEVEL_RECORD ) || is_log(level) ) {                  \
           boost::format format1(_T("%s - %s "));                               \
           format1 %_T(__FUNCTION__) %fmt ;                                     \
           log( level, boost::str( format1 ).c_str(), ##__VA_ARGS__ );          \
       }                                                                        \
   }while(0)

#endif

#endif

#define LOG2( level, fmt, ... )                                                 \
   do{                                                                          \
       if ( ( level == LOG_LEVEL_RECORD ) || is_log(level) ) {                  \
           log( level, fmt, ##__VA_ARGS__ );                                    \
       }                                                                        \
   }while(0)

#else

stdstring& trace_log_header( TRACE_LOG_LEVEL level ){                            
    SYSTEMTIME ltm;                                            
    TCHAR ch;                                                
    switch( level ){                                        
    case LOG_LEVEL_ERROR:     ch = _T('E');    break;
    case LOG_LEVEL_WARNING:   ch = _T('W');    break;
    case LOG_LEVEL_NOTICE:    ch = _T('N');    break;
    case LOG_LEVEL_INFO:      ch = _T('I');    break;
    case LOG_LEVEL_DEBUG:     ch = _T('D');    break;
    case LOG_LEVEL_INTERNAL:  ch = _T('L');    break;
    case LOG_LEVEL_TRACE:     ch = _T('T');    break;
    case LOG_LEVEL_RECORD:    ch = _T('R');    break;
    default:                  ch = _T('U');
    };                                                        
    GetLocalTime(&ltm);                                        
    return stringutils::format(                    
    _T("%4d-%02d-%02d %02d:%02d:%02d.%03d (T%04u) |%c|"),    
                        ltm.wYear,                            
                        ltm.wMonth,                            
                        ltm.wDay,                            
                        ltm.wHour,                            
                        ltm.wMinute,                        
                        ltm.wSecond,                        
                        ltm.wMilliseconds,                    
                        GetCurrentThreadId(),                
                        ch                                    
                        );                            
}

#define LOG( level, fmt, ... )                                                               \
    do{                                                                                      \
        if ( ( level == LOG_LEVEL_RECORD ) ||                                                \
            ( level == LOG_LEVEL_ERROR ) || ( level == LOG_LEVEL_WARNING ) ){                \
            stdstring log = trace_log_header( level );                                       \
            log.append( stringutils::format( _T("%s(%d) : %s - "),                           \
            _T(__FILE__), __LINE__ ,_T( __FUNCTION__ ) ) );                                  \
            log.append( macho::stringutils::format( fmt, ##__VA_ARGS__ ) );                  \
            if ( log.length() && log[log.length() - 1] != _T('\n') )                         \
                log.append(_T("\n"));                                                        \
            OutputDebugString( log.c_str() );                                                \
        }                                                                                    \
    }while(0)    

#define LOG2( level, fmt, ... )                                                              \
    do{                                                                                      \
        if ( ( level == LOG_LEVEL_RECORD ) ||                                                \
            ( level == LOG_LEVEL_ERROR ) || ( level == LOG_LEVEL_WARNING ) ){                \
            stdstring log = trace_log_header( level );                                       \
            log.append( macho::stringutils::format( fmt, ##__VA_ARGS__ ) );                  \
            if ( log.length() && log[log.length() - 1] != _T('\n') )                         \
                log.append(_T("\n"));                                                        \
            OutputDebugString( log.c_str() );                                                \
        }                                                                                    \
    }while(0)    
#endif 

class tracelog{
public:
    tracelog( stdstring identity = stdstring() ) : _implementation(identity) {}
    void            inline set_file( stdstring logfile ){ _implementation.set_file(logfile); }
    void            inline set_level( TRACE_LOG_LEVEL newlevel ){ _implementation.set_level(newlevel); }
    TRACE_LOG_LEVEL inline get_level() const { return _implementation.get_level(); }
    stdstring       inline get_file() const { return _implementation.get_file(); }
    void            inline log( TRACE_LOG_LEVEL level, LPCTSTR fmt, va_list args){
        if ( get_level() >= level ){
            _implementation.log( level, fmt, args );
        }    
    }
    void            inline log( TRACE_LOG_LEVEL level, LPCTSTR fmt, ... ){
        if ( get_level() >= level ){
            va_list args;
            va_start ( args, fmt );
            _implementation.log( level, fmt, args );
            va_end (args);
        }
    }
protected:
    class tracelog_implementation {
    public:
        tracelog_implementation( stdstring id ): level( LOG_LEVEL_WARNING ), maxfilesize(2 * 1024 * 1024), maxfilesnum(10), count(0){ identity = id; } 
        void            inline set_level( TRACE_LOG_LEVEL newlevel ){ level = newlevel; }
        void            inline set_file( stdstring logfile ){ file = logfile; }
        TRACE_LOG_LEVEL inline get_level() const { return level; }
        stdstring       inline get_file()  const { return file;  }
        void            inline log( TRACE_LOG_LEVEL level, LPCTSTR fmt, va_list args ){
            macho::windows::auto_lock lock( cs );
            log_header( level );
            log( fmt, args );
        }        
        void            log( LPCTSTR fmt, va_list args);
        void            log_header( TRACE_LOG_LEVEL level );
    private :
        void            shift_file(boost::filesystem::path old_file, boost::filesystem::path new_file);
        void            archive_log_file(boost::filesystem::path fname);
    private:
        macho::windows::critical_section        cs;
        TRACE_LOG_LEVEL                         level;
        stdstring                               trace_header;
        stdstring                               file;
        stdstring                               lastmsg;
        stdstring                               identity;
        UINT                                    maxfilesize;
        UINT                                    maxfilesnum;
        UINT                                    count;
    };
    tracelog_implementation       _implementation;
};

class auto_log{
public :
    auto_log(stdstring message){
        __message = message;
        LOG2( LOG_LEVEL_INTERNAL, _T("%s - Entered."), __message.c_str() ) ;
    }
    ~auto_log(){
        LOG2( LOG_LEVEL_INTERNAL, _T("%s - Exit."), __message.c_str() ) ;
    }
private:
    stdstring __message;
};

class auto_log_ex{
public :
    auto_log_ex(stdstring message, HRESULT& hr) : __hr(hr) {
        __message = message;
        LOG2(LOG_LEVEL_INTERNAL, _T("%s - Entered (0x%08X)."), __message.c_str(), __hr);
    }
    ~auto_log_ex(){
        LOG2(LOG_LEVEL_INTERNAL, _T("%s - Exit (0x%08X)."), __message.c_str(), __hr);
    }
private:
    stdstring __message;
    HRESULT&   __hr;
};

#if 0
#define FUN_TRACE_ENTER                   LOG( LOG_LEVEL_INTERNAL, _T("Entered.") )
#define FUN_TRACE_LEAVE                   LOG( LOG_LEVEL_INTERNAL, _T("Exit.") )
#define FUN_TRACE_LEAVE_HRESULT(x)        LOG( x ? LOG_LEVEL_ERROR : LOG_LEVEL_INTERNAL , _T("Exit (0x%08X)."), x )
#else
#ifdef _UNICODE

#ifdef _DEBUG
#define FUN_TRACE                                                                         \
        auto_log logFun(boost::str( boost::wformat(_T("%s(%d) : %s") )%_T(__FILE__)% __LINE__ %_T(__FUNCTION__) ) );

#define FUN_TRACE_HRESULT(hr)                                                                          \
        auto_log_ex logFunEx(boost::str( boost::wformat(_T("%s(%d) : %s") )%_T(__FILE__)% __LINE__ %_T(__FUNCTION__) ), hr );

#else

#define FUN_TRACE                                                                                       \
        auto_log logFun(boost::str( boost::wformat(_T("%s") )%_T(__FUNCTION__) ) );

#define FUN_TRACE_HRESULT(hr)                                                                           \
        auto_log_ex logFunEx(boost::str( boost::wformat(_T("%s") )%_T(__FUNCTION__) ), hr );

#endif

#else

#ifdef _DEBUG
#define FUN_TRACE                                                                                         \
        auto_log logFun(boost::str( boost::format(_T("%s(%d) : %s") )%_T(__FILE__)% __LINE__ %_T(__FUNCTION__) ) );

#define FUN_TRACE_HRESULT(hr)                                                                             \
        auto_log_ex logFunEx(boost::str( boost::format(_T("%s(%d) : %s") )%_T(__FILE__)% __LINE__ %_T(__FUNCTION__) ), hr );

#else
#define FUN_TRACE                                                                                         \
        auto_log logFun(boost::str( boost::format(_T("%s") )%_T(__FUNCTION__) ) );

#define FUN_TRACE_HRESULT(hr)                                                                             \
        auto_log_ex logFunEx(boost::str( boost::format(_T("%s") )%_T(__FUNCTION__) ), hr );
#endif

#endif

#endif

#ifndef MACHO_HEADER_ONLY

#ifndef MAX_DEDUCTED_MSG_NUM
#define MAX_DEDUCTED_MSG_NUM 100
#endif

void tracelog::tracelog_implementation::log_header( TRACE_LOG_LEVEL level ){
    SYSTEMTIME ltm;
    TCHAR ch;
    switch( level ){
    case LOG_LEVEL_ERROR:     ch = _T('E');    break;
    case LOG_LEVEL_WARNING:   ch = _T('W');    break;
    case LOG_LEVEL_NOTICE:    ch = _T('N');    break;
    case LOG_LEVEL_INFO:      ch = _T('I');    break;
    case LOG_LEVEL_DEBUG:     ch = _T('D');    break;
    case LOG_LEVEL_INTERNAL:  ch = _T('L');    break;
    case LOG_LEVEL_TRACE:     ch = _T('T');    break;
    case LOG_LEVEL_RECORD:    ch = _T('R');    break;
    default:                  ch = _T('U');
    };

    GetLocalTime(&ltm);
    // the caller should require lock to do it, so don't worry about the content in it
    if ( identity.length() ){
        trace_header = stringutils::format(TEXT("%4d-%02d-%02d %02d:%02d:%02d.%03d (T%04u) (%s) |%c|"), 
                        ltm.wYear,
                        ltm.wMonth,
                        ltm.wDay,
                        ltm.wHour,
                        ltm.wMinute,
                        ltm.wSecond,
                        ltm.wMilliseconds,
                        GetCurrentThreadId(),
                        identity.c_str(),
                        ch
                        );
    }
    else{
        trace_header = stringutils::format(TEXT("%4d-%02d-%02d %02d:%02d:%02d.%03d (T%04u) |%c|"), 
                        ltm.wYear,
                        ltm.wMonth,
                        ltm.wDay,
                        ltm.wHour,
                        ltm.wMinute,
                        ltm.wSecond,
                        ltm.wMilliseconds,
                        GetCurrentThreadId(),
                        ch);
    }
}

void tracelog::tracelog_implementation::shift_file(boost::filesystem::path old_file, boost::filesystem::path new_file){
    if (boost::filesystem::exists(new_file)){
        boost::filesystem::remove(new_file);
    }
    if (boost::filesystem::exists(old_file)){       
        boost::filesystem::rename(old_file, new_file);
    }
}

void tracelog::tracelog_implementation::archive_log_file(boost::filesystem::path fname){
    boost::filesystem::path oldname, newname;
    for (int i = maxfilesnum - 2; i > 0; i--){
        oldname = fname.string() + boost::str(boost::format(".%d") % i);
        newname = fname.string() + boost::str(boost::format(".%d") % (i + 1));  
        shift_file( oldname, newname );
    }
    newname = fname.string() + ".1";
    shift_file(fname, newname);
}

void tracelog::tracelog_implementation::log( LPCTSTR fmt, va_list args ){

    stdstring    msg;
    int          repeat = 0;
    bool        is_limit_reached = false;
    msg = stringutils::format( fmt, args );
    
    if( 0 == file.length() ){
        OutputDebugString( stringutils::format( _T("%s %s\n"), trace_header, msg.c_str()).c_str());
        return;
    }

    if ((_tcscmp(msg.c_str(), lastmsg.c_str()) == 0) && (count < MAX_DEDUCTED_MSG_NUM)) {
        count ++;
        return;
    }
    else{
        repeat = count;
        count = 0;
        lastmsg = msg;
    }
#ifdef _UNICODE
    std::wofstream fout(file, std::ios::out | std::ios::app);
#else
    std::ofstream fout(file, std::ios::out | std::ios::app);
#endif
    if (fout.is_open()){
        if (repeat){
            size_t found = trace_header.find_first_of(_T('|'));
            if (found != stdstring::npos){
                trace_header[found + 1] = _T('R');
            }
#ifdef _UNICODE
            fout << boost::str(boost::wformat(TEXT("%s Last message repeated %d times")) %trace_header %repeat ) << std::endl;
#else
            fout << boost::str(boost::format(TEXT("%s Last message repeated %d times")) % trace_header %repeat) << std::endl;
#endif
        }
        fout << trace_header << msg;
        if ((msg.length() == 0) || (msg[msg.length() - 1] != _T('\n'))){
            fout << std::endl;
        }
        is_limit_reached = fout.tellp() >= maxfilesize;
        fout.close();
    }
    if (is_limit_reached){
        archive_log_file( file );    
    }
}

#endif

};

#endif
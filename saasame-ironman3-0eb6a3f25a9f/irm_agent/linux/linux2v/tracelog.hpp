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

#include <boost/format.hpp>
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/date_time.hpp>
#include <string>
#include <time.h>
#include <stdarg.h>

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
#define _T 
#define TEXT

void log(TRACE_LOG_LEVEL level, const char* fmt, ...);
bool is_log(TRACE_LOG_LEVEL level);

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

class tracelog{
public:
    tracelog( std::string identity = std::string() ) : _implementation(identity) {}
    void            inline set_file( std::string logfile ){ _implementation.set_file(logfile); }
    void            inline set_level( TRACE_LOG_LEVEL newlevel ){ _implementation.set_level(newlevel); }
    TRACE_LOG_LEVEL inline get_level() const { return _implementation.get_level(); }
    std::string       inline get_file() const { return _implementation.get_file(); }
    void            inline log( TRACE_LOG_LEVEL level, const char* fmt, va_list args){
        if ( get_level() >= level ){
            _implementation.log( level, fmt, args );
        }    
    }
    void            inline log( TRACE_LOG_LEVEL level, const char* fmt, ... ){
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
        tracelog_implementation( std::string id ): level( LOG_LEVEL_WARNING ), maxfilesize(2 * 1024 * 1024), maxfilesnum(10), count(0){ identity = id; } 
        void            inline set_level( TRACE_LOG_LEVEL newlevel ){ level = newlevel; }
        void            inline set_file( std::string logfile ){ file = logfile; }
        TRACE_LOG_LEVEL inline get_level() const { return level; }
        std::string       inline get_file()  const { return file;  }
        void            inline log( TRACE_LOG_LEVEL level, const char* fmt, va_list args ){
            boost::unique_lock<boost::mutex> lock(cs);
            log_header( level );
            log( fmt, args );
        }        
        void            log( const char* fmt, va_list args);
        void            log_header( TRACE_LOG_LEVEL level );
    private :
        void            shift_file(boost::filesystem::path old_file, boost::filesystem::path new_file);
        void            archive_log_file(boost::filesystem::path fname);
    private:
        std::string& format(std::string& str, const char* fmt, va_list args) {
            int len = 0;
            std::vector<char> buf;
            if (fmt) {
                len = _vscprintf(fmt, args) + 1;
                buf.resize(len, 0);
                vsnprintf(&buf.at(0), len, fmt, args);
                str = &buf.at(0);
            }
            return str;
        }
        int _vscprintf(const char * format, va_list pargs) {
            int retval;
            va_list argcopy;
            va_copy(argcopy, pargs);
            retval = vsnprintf(NULL, 0, format, argcopy);
            va_end(argcopy);
            return retval;
        }

        boost::mutex                              cs;
        TRACE_LOG_LEVEL                           level;
        std::string                               trace_header;
        std::string                               file;
        std::string                               lastmsg;
        std::string                               identity;
        uint32_t                                  maxfilesize;
        uint32_t                                  maxfilesnum;
        uint32_t                                  count;
    };
    tracelog_implementation                       _implementation;
};



#ifndef MACHO_HEADER_ONLY

#ifndef MAX_DEDUCTED_MSG_NUM
#define MAX_DEDUCTED_MSG_NUM 100
#endif

void tracelog::tracelog_implementation::log_header( TRACE_LOG_LEVEL level ){
    char ch;
    switch (level) {
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
    // the caller should require lock to do it, so don't worry about the content in it
    if ( identity.length() ){
        trace_header = boost::str((boost::format("%s (T%04u) (%s) |%c|") % boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) % boost::this_thread::get_id() % identity % ch));
    }
    else{
        trace_header = boost::str((boost::format("%s (T%04u)|%c|") % boost::posix_time::to_simple_string(boost::posix_time::microsec_clock::local_time()) % boost::this_thread::get_id() % ch));
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

void tracelog::tracelog_implementation::log( const char* fmt, va_list args ){

    std::string    msg;
    int          repeat = 0;
    bool        is_limit_reached = false;
    format(msg, fmt, args);
    if( 0 == file.length() ){
        file = "/dev/kmsg";
    }

    if ((strcmp(msg.c_str(), lastmsg.c_str()) == 0) && (count < MAX_DEDUCTED_MSG_NUM)) {
        count ++;
        return;
    }
    else{
        repeat = count;
        count = 0;
        lastmsg = msg;
    }
    std::ofstream fout(file, std::ios::out | std::ios::app);
    if (fout.is_open()){
        if (repeat){
            size_t found = trace_header.find_first_of(_T('|'));
            if (found != std::string::npos){
                trace_header[found + 1] = _T('R');
            }
            fout << boost::str(boost::format(TEXT("%s Last message repeated %d times")) % trace_header %repeat) << std::endl;
        }
        fout << trace_header << msg;
        if ((msg.length() == 0) || (msg[msg.length() - 1] != _T('\n'))){
            fout << std::endl;
        }
        is_limit_reached = file != "/dev/kmsg" && fout.tellp() >= maxfilesize;
        fout.close();
    }
    if (is_limit_reached){
        archive_log_file( file );    
    }
}

#endif

};

#endif
// lib_main.cpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#include "..\includes\macho.h"

using namespace macho;
static tracelog g_log( _T("global") );

void macho::log( TRACE_LOG_LEVEL level, LPCTSTR fmt, ... ){
    va_list arg_list;
    va_start ( arg_list, fmt );
    g_log.log( level, fmt, arg_list );
    va_end ( arg_list );
}

bool macho::is_log( TRACE_LOG_LEVEL level ){
    return level <= g_log.get_level();
}

void macho::set_log_file( stdstring logfile ){
    g_log.set_file( logfile );
}

void macho::set_log_level( TRACE_LOG_LEVEL level ){
    g_log.set_level( level );
}

stdstring macho::get_log_file(){
    return g_log.get_file();
}

TRACE_LOG_LEVEL macho::get_log_level(){
    return g_log.get_level();
}


// exception_base.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_EXCEPTION_BASE__
#define __MACHO_EXCEPTION_BASE__

#include <boost/format.hpp>
#include <boost/exception/all.hpp>
#include <boost/exception/get_error_info.hpp>

namespace macho
{

struct  exception_base : virtual public boost::exception, virtual public std::exception {};

typedef boost::error_info<struct throw_errorno_, unsigned long> throw_errorno; 

#ifdef _UNICODE

typedef boost::error_info<struct throw_wmessage_, std::wstring> throw_wmessage; 
typedef boost::error_info<struct throw_wfile_, wchar_t const *> throw_wfile; 
typedef boost::error_info<struct throw_wfunction_, wchar_t const *> throw_wfunction; 

#if defined( BOOST_CURRENT_FUNCTION )
#undef  BOOST_CURRENT_FUNCTION
#endif

#define BOOST_CURRENT_FUNCTION __FUNCTION__

#define BOOST_THROW_EXCEPTION_BASE( type, no, message )\
    do{\
        BOOST_THROW_EXCEPTION( type() << macho::throw_wfile(_T(__FILE__)) << macho::throw_wfunction(_T(__FUNCTION__)) << macho::throw_errorno((unsigned long)no) <<macho::throw_wmessage(message) );\
        \
    }while(0)

#define BOOST_THROW_EXCEPTION_BASE_STRING( type, message)\
    do{\
        BOOST_THROW_EXCEPTION( type() << macho::throw_wfile(_T(__FILE__)) << macho::throw_wfunction(_T(__FUNCTION__)) << macho::throw_wmessage(message) );\
        \
    }while(0)

std::wstring inline get_diagnostic_information( boost::exception &e ){

    std::wstring msg;
    if ( wchar_t const **pfile = boost::get_error_info<throw_wfile>(e) )
        msg = boost::str( boost::wformat(L"%s")  %*pfile );
    if ( int const *line = boost::get_error_info<boost::throw_line>(e) )
        msg = boost::str( boost::wformat(L"%s(%d)") %msg %*line );
    if ( wchar_t const **pfun  = boost::get_error_info<throw_wfunction>(e) )
        msg = boost::str( boost::wformat(L"%s : %s") %msg %*pfun );
    if ( std::wstring *pmsg  = boost::get_error_info<throw_wmessage>(e) )
        msg = boost::str( boost::wformat(L"%s - %s") %msg %*pmsg );
    if ( unsigned long const *error = boost::get_error_info<throw_errorno>(e) )
        msg = boost::str( boost::wformat(L"%s (0x%08X)") %msg %*error );
    
    return msg;
}

#else

typedef boost::error_info<struct throw_message_, std::string> throw_message; 

#define BOOST_THROW_EXCEPTION_BASE( no, message )\
    do{\
        BOOST_THROW_EXCEPTION( wmi_exception() << throw_errorno((unsigned long)no) << throw_message(message) );\
        \
    }while(0)

#define BOOST_THROW_EXCEPTION_BASE_STRING(message)\
    do{\
        BOOST_THROW_EXCEPTION( wmi_exception() << throw_message(message) );\
        \
    }while(0)

std::string inline get_diagnostic_information( boost::exception &e ){
    return boost::exception_detail::get_diagnostic_information( e , __FUNCTION__);
}
#endif

};

#endif

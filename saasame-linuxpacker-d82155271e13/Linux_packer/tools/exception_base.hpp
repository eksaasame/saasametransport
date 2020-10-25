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

namespace mumi
{
    struct  exception_base : virtual public boost::exception, virtual public std::exception {};

    typedef boost::error_info<struct throw_errorno_, unsigned long> throw_errorno;

    typedef boost::error_info<struct throw_message_, std::string> throw_message;
    typedef boost::error_info<struct throw_file_, char const *> throw_file;
    typedef boost::error_info<struct throw_function_, char const *> throw_function;

#if defined( BOOST_CURRENT_FUNCTION )
#undef  BOOST_CURRENT_FUNCTION
#endif

#define BOOST_CURRENT_FUNCTION __FUNCTION__

#define BOOST_THROW_EXCEPTION_BASE( type, no, message )\
    do{\
        BOOST_THROW_EXCEPTION( type() << mumi::throw_file(__FILE__) << mumi::throw_function(__FUNCTION__) << mumi::throw_errorno((unsigned long)no) <<mumi::throw_message(message) );\
        \
    }while(0)

#define BOOST_THROW_EXCEPTION_BASE_STRING( type, message)\
    do{\
        BOOST_THROW_EXCEPTION( type() << mumi::throw_file(__FILE__) << mumi::throw_function(__FUNCTION__) << mumi::throw_message(message) );\
        \
    }while(0)

    std::string inline get_diagnostic_information(boost::exception &e) {

        std::string msg;
        if (char const **pfile = boost::get_error_info<throw_file>(e))
            msg = boost::str(boost::format("%s") % *pfile);
        if (int const *line = boost::get_error_info<boost::throw_line>(e))
            msg = boost::str(boost::format("%s(%d)") % msg %*line);
        if (char const **pfun = boost::get_error_info<throw_function>(e))
            msg = boost::str(boost::format("%s : %s") % msg %*pfun);
        if (std::string *pmsg = boost::get_error_info<throw_message>(e))
            msg = boost::str(boost::format("%s - %s") % msg %*pmsg);
        if (unsigned long const *error = boost::get_error_info<throw_errorno>(e))
            msg = boost::str(boost::format("%s (0x%08X)") % msg %*error);
        return msg;
    }
}
#endif

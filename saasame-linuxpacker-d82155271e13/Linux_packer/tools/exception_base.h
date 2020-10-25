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
#include <string>

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

    std::string inline get_diagnostic_information(boost::exception &e);
}
#endif

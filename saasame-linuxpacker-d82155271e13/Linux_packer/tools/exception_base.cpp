// exception_base.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------
#include "exception_base.h"

std::string mumi::get_diagnostic_information(boost::exception &e) {
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
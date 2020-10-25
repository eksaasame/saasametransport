// config.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_CONFIG__
#define __MACHO_CONFIG__

#include <Windows.h>
#include <tchar.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

namespace macho{

#if _UNICODE
#define stdstring std::wstring 
#else
#define stdstring std::string
#endif

typedef std::vector<std::string>      string_array_a;
typedef std::vector<std::wstring>     string_array_w;
typedef std::vector<std::string>      string_table_a;
typedef std::vector<std::wstring>     string_table_w;

#if _UNICODE
typedef string_array_w       string_array;
typedef string_array_w       string_table;
#else
typedef string_array_a       string_array;
typedef string_array_a       string_table;
#endif
typedef std::vector< unsigned char >  uint8_table;
typedef std::vector< unsigned short > uint16_table;
typedef std::vector< unsigned long >  uint32_table;

template<typename T>
class _or
{
public:
    typedef T result_type;
    template<typename InputIterator>
    T operator()(InputIterator first, InputIterator last) const
    {
        // If there are no slots to call, just return the
        // default-constructed value
        if (first == last) return T(false);
        T value = *first++;
        while (first != last) {
            value = value || *first;
            ++first;
        }
        return value;
    }
};

};

#include <boost\foreach.hpp>
#ifndef foreach
#define foreach         BOOST_FOREACH
#define reverse_foreach BOOST_REVERSE_FOREACH
#endif

#ifndef MACHO_LINK_LIB
#define MACHO_HEADER_ONLY
#define MACHO_GLOBAL_TRACE
#pragma comment(lib, "macho.lib")
#endif

#endif
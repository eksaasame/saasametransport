// base32.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_GUID_INCLUDE__
#define __MACHO_GUID_INCLUDE__

#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include "boost\shared_ptr.hpp"
#include <cguid.h>
#pragma comment(lib, "rpcrt4.lib")

namespace macho{

class guid_{
public:
    typedef boost::shared_ptr<guid_> ptr;
    typedef std::vector<ptr> vtr;
    struct  exception : virtual public exception_base {};

    guid_(void) :_guid(GUID_NULL){}
    guid_(const guid_ &g) : _guid(GUID_NULL) {
         copy(g);
    }
    guid_(const GUID &g) : _guid(GUID_NULL) {
         copy(g);
    }
    guid_(const std::string &g) : _guid(GUID_NULL) {
         (*this) = g;
    }
    guid_(const std::wstring &g) : _guid(GUID_NULL) {
         (*this) = g;
    }
    guid_(LPCSTR &g) : _guid(GUID_NULL) {
         (*this) = g;
    }
    guid_(LPCWSTR &g) : _guid(GUID_NULL) {
         (*this) = g;
    }

    std::string    string(){ return operator std::string(); }
    std::wstring   wstring(){ return operator std::wstring(); }

    const guid_ &operator =(const guid_& g);
    const guid_ &operator =(const GUID& g);
    const guid_ &operator =(const std::wstring& g);
    const guid_ &operator =(const std::string& g);
    const guid_ &operator =(LPCWSTR& g);
    const guid_ &operator =(LPCSTR& g);

    bool operator==(const GUID& g);
    bool operator==(const guid_& g);
    bool operator!=(const GUID& g);
    bool operator!=(const guid_& g);
    operator GUID&();
    operator GUID*();
    operator std::string();
    operator std::wstring();
    static       guid_ create();
private:
    void copy(const guid_& g);
    void copy(const GUID& g);
    GUID _guid;
};

#ifndef MACHO_HEADER_ONLY

#define BOOST_THROW_GUID_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( guid_::exception, no, message )
#define BOOST_THROW_GUID_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING( guid_::exception, message)       

#define GUID_STRING_LEN 40

void guid_::copy(const guid_& g) {
    _guid = g._guid;
}

void guid_::copy(const GUID& g) {
    _guid = g;
}

const guid_ &guid_::operator = (const guid_& g){
    if (this != &g)
        copy(g);
    return(*this);
}

const guid_ &guid_::operator = (const GUID& g){
    copy(g);
    return(*this);
}

const guid_ &guid_::operator = (const std::wstring& g){
    if (g.length() < GUID_STRING_LEN -4) BOOST_THROW_GUID_EXCEPTION_STRING(_T("the parameter is short"));
    RPC_STATUS rs;
    if (g.at(0) == L'{'){
        if (g.at(g.length() - 1) != L'}')
            BOOST_THROW_GUID_EXCEPTION_STRING(_T("the parameter is invalid"));
        rs = UuidFromStringW((RPC_WSTR)g.substr(1, g.length() -2 ).c_str(), &(*this)._guid);
    }
    else{
        rs = UuidFromStringW((RPC_WSTR)g.c_str(), &(*this)._guid);
    }
    if (rs != RPC_S_OK) BOOST_THROW_GUID_EXCEPTION(rs, _T("UuidFromStringW Failed"));
    return(*this);
}

const guid_ &guid_::operator = (const std::string& g){
    if (g.length() < GUID_STRING_LEN -4) BOOST_THROW_GUID_EXCEPTION_STRING(_T("the parameter is short"));
    RPC_STATUS rs;
    if (g.at(0) == '{'){
        if (g.at(g.length() - 1) != '}')
            BOOST_THROW_GUID_EXCEPTION_STRING(_T("the parameter is invalid"));
        rs = UuidFromStringA((RPC_CSTR)g.substr(1, g.length() - 2).c_str(), &(*this)._guid);
    }
    else{
        rs = UuidFromStringA((RPC_CSTR)g.c_str(), &(*this)._guid);
    }
    if (rs != RPC_S_OK) BOOST_THROW_GUID_EXCEPTION(rs, _T("UuidFromStringA Failed"));
    return(*this);
}

const guid_ &guid_::operator = (LPCWSTR& g){
    if (!g) BOOST_THROW_GUID_EXCEPTION_STRING(_T("the parameter is null"));
    (*this) = std::wstring(g);
    return(*this);
}

const guid_ &guid_::operator = (LPCSTR& g){
    if (!g) BOOST_THROW_GUID_EXCEPTION_STRING(_T("the parameter is null"));
    (*this) = std::string(g);
    return(*this);
}

guid_ guid_::create(){
    guid_ g;
    RPC_STATUS rs = ::UuidCreate(&g._guid);
    if (rs!= RPC_S_OK) BOOST_THROW_GUID_EXCEPTION(rs, _T("UuidCreate Failed"));
    return g;
}

bool guid_::operator == (const GUID& g){
    return !::memcmp(&_guid, &g, sizeof(GUID));
}

bool guid_::operator == (const guid_& g){
    return operator==(g._guid);
}

bool guid_::operator != (const GUID& g){
    return ::memcmp(&_guid, &g, sizeof(GUID)) != 0;
}

bool guid_::operator != (const guid_& g){
    return operator!=(g._guid);
}

guid_::operator GUID&(){
    return _guid;
}

guid_::operator GUID*(){
    return &_guid;
}

guid_::operator std::string(){
    RPC_STATUS rs;
    unsigned char* s;
    std::string r;
    //
    // Why does this function take an unsigned char* instead of a char*?
    //
    rs = ::UuidToStringA(&_guid, &s);
    if (rs == RPC_S_OK) {
        r = reinterpret_cast<char*> (s);
        ::RpcStringFreeA(&s);
    }
    return r;
}

guid_::operator std::wstring(){
    RPC_STATUS rs;
    wchar_t* s;
    std::wstring r;

    rs = ::UuidToStringW(&_guid, (RPC_WSTR*)&s);
    if (rs == RPC_S_OK) {
        r = s;
        ::RpcStringFreeW((RPC_WSTR*)&s);
    }
    return r;
}

#endif

};

#endif
// bytes.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_BYTES_INCLUDE__
#define __MACHO_BYTES_INCLUDE__

#include "..\config\config.hpp"
#include <vector>
#include <memory>
#include <algorithm>
#include <assert.h>

namespace macho{

class bytes{
public:
    bytes( void ): lpbytes(NULL) {
    }
    
    bytes( const bytes &_bytes ): lpbytes(NULL) {
        copy( _bytes );
    }
    
    bytes( LPBYTE lpBuf, size_t nLen): lpbytes(NULL) {
        set( lpBuf, nLen ) ;
    }
    
    bytes( stdstring _bytes ): lpbytes(NULL) {
        set( _bytes );
    }

    bytes( stdstring szbytes, stdstring delimiters ): lpbytes(NULL) {
        set( szbytes, delimiters );
    }     
    
    ~bytes( void ){ }
    
    void copy ( const bytes& _bytes ) { 
        vbytes = _bytes.vbytes;  
    }

    const bytes &operator =( const bytes& _bytes  ) {
        if ( this != &_bytes )
            copy( _bytes );
        return( *this );
    }
    
    bool operator==(const bytes& _bytes){
        return vbytes == _bytes.vbytes;
    }

    bool operator!=(const bytes& _bytes){
        return vbytes != _bytes.vbytes;
    }
    
    BYTE&  operator[]( size_t index ) { return vbytes[index]; }
    void inline resize( size_t nLen ){ vbytes.resize( nLen, 0 ); }

    stdstring const get( stdstring delimiters = _T(",") );
    void set( stdstring szbytes, stdstring delimiters = _T(",") );
    size_t const inline length(){ return vbytes.size(); }
    void set( LPBYTE lpBuf, size_t nLen );
    void get( LPBYTE lpbDest, size_t nMaxLen );
    LPBYTE const ptr();
    static int hex_to_dec( const stdstring& csHex );
private:
    std::vector<stdstring> tokenize( const stdstring& str, const stdstring& delimiters );
    std::vector<BYTE>      vbytes;
    std::auto_ptr<BYTE>    lpbytes;
};

#ifndef MACHO_HEADER_ONLY

stdstring const bytes::get( stdstring delimiters ){
    TCHAR szHex[4];
    stdstring szString;
    if ( delimiters.length() == 0 )    delimiters = _T(",");
    for ( size_t n = 0; n < vbytes.size(); n++ ){
        _stprintf_s( szHex, 4, _T("%02x"), vbytes[n] );
        szString.append(szHex);    
        szString.append( delimiters.substr( 0, 1 ) );
    }
    return szString.substr( 0, szString.length() - 1 );
}

void bytes::set( stdstring szbytes, stdstring delimiters ){
    std::vector<stdstring> tokens;
    if ( delimiters.length() == 0 ) delimiters = _T(",");
    if (!szbytes.length()) { vbytes.clear(); return; }
    tokens = tokenize( szbytes, delimiters );
    if ( vbytes.size() < tokens.size() ) vbytes.reserve( tokens.size() );
    vbytes.clear();
    for ( size_t n = 0; n < tokens.size(); n++ ){
        vbytes.push_back( hex_to_dec(tokens[n] ) );
    }    
}

void bytes::set( LPBYTE lpBuf, size_t nLen ){
    if (!nLen) { vbytes.clear(); return; }
    if (vbytes.size() < nLen) vbytes.reserve(nLen);
    vbytes.clear();
    do { vbytes.push_back(lpBuf[vbytes.size()]); }
    while (vbytes.size() < nLen);
}

void bytes::get( LPBYTE lpbDest, size_t nMaxLen ) {    
    if ((size_t)(&vbytes.back() - &vbytes.at(0)+1) == vbytes.size()*sizeof(BYTE))
        memcpy(lpbDest, (LPBYTE)&vbytes.at(0), vbytes.size() > nMaxLen ? nMaxLen : vbytes.size());
    else
        for (size_t n=0; n < vbytes.size() && n < nMaxLen; n++)
            lpbDest[n] = vbytes[n];        
}

LPBYTE const bytes::ptr() {
    if (vbytes.size()){
        if ((size_t)(&vbytes.back() - &vbytes.at(0) + 1) == vbytes.size()*sizeof(BYTE))
            return (LPBYTE)&vbytes.at(0);
        else{
            lpbytes = std::auto_ptr<BYTE>(new BYTE[vbytes.size() + 2]);
            if (lpbytes.get()){
                memset(lpbytes.get(), 0, vbytes.size() + 2);
                get(lpbytes.get(), vbytes.size());
                return lpbytes.get();
            }
        }
    }
    return NULL;
}

int bytes::hex_to_dec( const stdstring& csHex ){
    int tot = 0;
    TCHAR hex[17] = {_T('0'),_T('1'),_T('2'),_T('3'),_T('4'),_T('5'),_T('6'),_T('7'),_T('8'),_T('9'),_T('a'),_T('b'),_T('c'),_T('d'),_T('e'),_T('f'),_T('\0')};
    int num[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    int base = 1;
    int nCtr = ( int ) csHex.length() - 1 ;
    stdstring csHexLocal = csHex;
#if    _UNICODE
    std::transform(csHexLocal.begin(), csHexLocal.end(), csHexLocal.begin(), towlower);
#else
    std::transform(csHexLocal.begin(), csHexLocal.end(), csHexLocal.begin(), std::tolower);
#endif
    for (int n = nCtr; n >= 0; n-- ){
        TCHAR lc = csHexLocal[n];
        int vl = 0;
        for (int n = 0; n < 16; n++ ){
            TCHAR cc = hex[n];
            if ( lc == cc ){
                vl = num[n];
                break;
            }
        }
        tot = tot + ( base * vl );
        base = base * 16;
    }
    return tot;    
}

std::vector<stdstring> bytes::tokenize( const stdstring& str, const stdstring& delimiters ){
    
    std::vector<stdstring> tokens;
    stdstring::size_type delimPos = 0, tokenPos = 0, pos = 0;
    
    if( str.length() < 1 ) return tokens;

    while( 1 ){
        delimPos = str.find_first_of( delimiters, pos );
        tokenPos = str.find_first_not_of( delimiters, pos );

        if( stdstring::npos != delimPos ){
            if( stdstring::npos != tokenPos ){
                if( tokenPos < delimPos ){
                    tokens.push_back(str.substr( pos, delimPos-pos ) );
                }
                else{
                    tokens.push_back(_T(""));
                }
            }
            else {
                tokens.push_back(_T(""));
            }
            pos = delimPos + 1;           
        }
        else {
            if( stdstring::npos != tokenPos ) {
                tokens.push_back( str.substr( pos ) );
            }
            else{
                tokens.push_back(_T(""));
            }
            break;
        }
    }
    return tokens;
}

#endif

};//namespace macho

#endif
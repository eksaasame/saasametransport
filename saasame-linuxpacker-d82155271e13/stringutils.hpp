// stringutils.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_STRING_UTILS__
#define __MACHO_STRING_UTILS__

#include "..\config\config.hpp"
#include "boost\format.hpp"
#include "..\windows\lock.hpp"
#include "bytes.hpp"
#include <cctype> 
#include <algorithm>

namespace macho{

class stringutils{
public:
    static std::string& format( LPCSTR fmt, ...); 
    static std::string& format( LPCSTR fmt, va_list args);
    static std::string& format(std::string& str, LPCSTR fmt, ...);
    static std::string& format(std::string& str, LPCSTR fmt, va_list args);
    static std::wstring& format( LPCWSTR fmt, ...); 
    static std::wstring& format( LPCWSTR fmt, va_list args);
    static std::wstring& format(std::wstring& str, LPCWSTR fmt, ...);
    static std::wstring& format(std::wstring& str, LPCWSTR fmt, va_list args);
    static std::wstring convert_multibytes_to_widechar( bytes& src, UINT codePage = CP_ACP );
    static bytes        convert_widechar_to_multibytes( const std::wstring& src, UINT codePage = CP_ACP );
    static std::string  convert_unicode_to_ansi( const std::wstring& src );
    static std::string  convert_unicode_to_utf8( const std::wstring& src );
    static std::wstring convert_ansi_to_unicode( const std::string& src );
    static std::wstring convert_utf8_to_unicode( const std::string& src );
    static std::string erase_trailing_whitespaces( const std::string& str );
    static std::string remove_begining_whitespaces( const std::string& str );
    static std::string parser_double_quotation_mark( const std::string& str );
    static std::vector<std::string> tokenize2( const std::string& str, const std::string& delimiters, size_t num = 0, bool empty = true );
    static std::vector<std::string> tokenize( const std::string& str, const std::string& delimiters, size_t num = 0, bool empty = true );
    static std::string replace_double_slash_with_one( const std::string& str );
    static std::string replace_one_slash_with_two( const std::string& str );

    static std::wstring erase_trailing_whitespaces(const std::wstring& str);
    static std::wstring remove_begining_whitespaces(const std::wstring& str);
    static std::wstring parser_double_quotation_mark(const std::wstring& str);
    static std::vector<std::wstring> tokenize2(const std::wstring& str, const std::wstring& delimiters, size_t num = 0, bool empty = true);
    static std::vector<std::wstring> tokenize(const std::wstring& str, const std::wstring& delimiters, size_t num = 0, bool empty = true);
    static std::wstring replace_double_slash_with_one(const std::wstring& str);
    static std::wstring replace_one_slash_with_two(const std::wstring& str);

    static std::string& tolower(std::string& str);
    static std::string& toupper(std::string& str);
    static std::wstring& tolower(std::wstring& str);
    static std::wstring& toupper(std::wstring& str);
    static std::vector<std::string> split( std::string& str, std::string subStr );
    static std::vector<std::wstring> split(std::wstring& str, std::wstring subStr);
    struct no_case_string_less{
        bool operator()(const stdstring& _Left, const stdstring& _Right) const{
            return ( _tcsicmp(_Left.c_str(), _Right.c_str()) < 0);
        }
    };
    struct no_case_string_less_a{
        bool operator()(const std::string& _Left, const std::string& _Right) const{
            return (_stricmp(_Left.c_str(), _Right.c_str()) < 0);
        }
    };
    struct no_case_string_less_w{
        bool operator()(const std::wstring& _Left, const std::wstring& _Right) const{
            return (_wcsicmp(_Left.c_str(), _Right.c_str()) < 0);
        }
    };
};

#ifndef MACHO_HEADER_ONLY

static macho::windows::critical_section _cs;

std::string stringutils::convert_unicode_to_ansi( const std::wstring& src ){
    bytes _ansi = convert_widechar_to_multibytes( src );
    std::string ansi;
    if ( _ansi.length() ) ansi.assign((LPCSTR)_ansi.ptr(), _ansi.length());
    return ansi;
}

std::string stringutils::convert_unicode_to_utf8(const std::wstring& src){
    bytes _utf8 = convert_widechar_to_multibytes(src, CP_UTF8);
    std::string utf8;
    if (_utf8.length()) utf8.assign((LPCSTR)_utf8.ptr(), _utf8.length());
    return utf8;
}

std::wstring stringutils::convert_ansi_to_unicode( const std::string& src ){
    if ( src.length() ){
        bytes _ansi( (LPBYTE)src.c_str(), src.length() );
        return convert_multibytes_to_widechar( _ansi );
    }
    return L"";
}

std::wstring stringutils::convert_utf8_to_unicode(const std::string& src){
    if (src.length()){
        bytes _utf8((LPBYTE)src.c_str(), src.length());
        return convert_multibytes_to_widechar(_utf8, CP_UTF8);
    }
    return L"";
}

std::string& stringutils::format( LPCSTR fmt, ...) {
    macho::windows::auto_lock _lock(_cs);
    static std::string str;
    va_list               list = NULL;
    va_start(list, fmt);
    format(str, fmt, list);
    va_end(list);
    return str;    
}

std::string& stringutils::format( LPCSTR fmt, va_list args){
    macho::windows::auto_lock _lock(_cs);
    static std::string str;
    format(str, fmt, args);
    return str;    
}

std::string& stringutils::format(std::string& str, LPCSTR fmt, ...){
    va_list list    = NULL;
    va_start(list, fmt);
    format(str, fmt, list);
    va_end(list);
    return str;
}

std::string& stringutils::format(std::string& str, LPCSTR fmt, va_list args){
    int len = 0;
    std::vector<char> buf;
    if(fmt)    {         
        len = _vscprintf(fmt, args) + 1;
        buf.resize(len, 0);
        vsprintf_s(&buf.at(0), len, fmt, args);
        str = &buf.at(0);
    }
    return str;
}

std::wstring& stringutils::format( LPCWSTR fmt, ...) {
    macho::windows::auto_lock _lock(_cs);
    static std::wstring str;
    va_list                list = NULL;
    va_start(list, fmt);
    format(str, fmt, list);
    va_end(list);
    return str;    
}

std::wstring& stringutils::format( LPCWSTR fmt, va_list args){
    macho::windows::auto_lock _lock(_cs);
    static std::wstring str;
    format(str, fmt, args);
    return str;    
}

std::wstring& stringutils::format(std::wstring& str, LPCWSTR fmt, ...){
    va_list list    = NULL;
    va_start(list, fmt);
    format(str, fmt, list);
    va_end(list);
    return str;
}

std::wstring& stringutils::format(std::wstring& str, LPCWSTR fmt, va_list args){
    int len = 0;
    std::vector<wchar_t> buf;
    if(fmt)    {         
        len = _vscwprintf(fmt, args) + 1;
        buf.resize(len, 0);
        vswprintf_s(&buf.at(0), len, fmt, args);
        str = &buf.at(0);
    }
    return str;
}

std::wstring stringutils::convert_multibytes_to_widechar( bytes& src, UINT codePage ){  
    std::wstring  rt;  
    int    len = (int)src.length();
    if ( len ){
        int unicodeLen = ::MultiByteToWideChar( codePage, 0, (LPCSTR)src.ptr(), len, NULL, 0 );  
        wchar_t * pUnicode = new  wchar_t[unicodeLen+1];  
        if ( pUnicode ){
            memset(pUnicode,0,(unicodeLen+1)*sizeof(wchar_t));  
            ::MultiByteToWideChar( codePage, 0, (LPCSTR)src.ptr(), len, (LPWSTR)pUnicode, unicodeLen ); 
            rt.append( ( wchar_t* )pUnicode, unicodeLen );
            delete[]  pUnicode; 
        }
    }
    return  rt;  
}

bytes stringutils::convert_widechar_to_multibytes( const std::wstring& src, UINT codePage ){    
    bytes  rt;
    if ( src.length() ){
        int       len = WideCharToMultiByte( codePage, 0, src.c_str(), (int)src.length(), NULL, 0, NULL, NULL );
        char*  pElementText = new char[len + 1];
        if ( pElementText ){
            memset( ( void* )pElementText, 0, sizeof( char ) * ( len + 1 ) );
            ::WideCharToMultiByte( codePage, 0, src.c_str(), (int)src.length(), pElementText, len, NULL, NULL );
            rt.set((LPBYTE)pElementText, len ) ;
            delete[] pElementText;
        }
    }
    return rt;
}

std::wstring stringutils::replace_double_slash_with_one( const std::wstring& str ){
    std::wstring strTemp;
    int                nIndex = 0;
    int                nNewIndex = 0;
    strTemp.resize( str.length() + 1 );
    while ( nIndex < (int)str.length() ) {
        if ( str[nIndex] == _T('\\') && ( nIndex < (int)str.length() - 1 ) && str[nIndex+1] == _T('\\') ){
            strTemp[nNewIndex++] = _T('\\');
            nIndex+=2;
        }
        else
            strTemp[nNewIndex++] = str[nIndex++];
    }
    strTemp[nNewIndex++] =  _T('\0');
    return strTemp;
}
        
std::wstring stringutils::replace_one_slash_with_two( const std::wstring& str ){
    std::wstring        strTemp;
    int                nIndex = 0;
    int                nNewIndex = 0;
    strTemp.resize( str.length() + 1 );
    while ( nIndex < (int)str.length() )
    {
        if ( str[nIndex] == _T('\\') && ( nIndex < (int)str.length() - 1 ) && str[nIndex+1] == _T('\\') ){
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            nIndex+=2;
            strTemp.resize( strTemp.length() + 2 );
        }
        else if ( str[nIndex] == _T('\\') && ( ( nIndex == (int)str.length() - 1 ) || str[nIndex+1] != _T('\\') ) ){
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            nIndex+=1;
            strTemp.resize( strTemp.length() + 1 );
        }
        else
            strTemp[nNewIndex++] = str[nIndex++];
    }
    strTemp[nNewIndex++] =  _T('\0');
    return strTemp;
}

std::wstring  stringutils::parser_double_quotation_mark( const std::wstring& str )
{
    std::wstring strTemp = erase_trailing_whitespaces(str);
    strTemp = remove_begining_whitespaces(strTemp);    

    if ( strTemp.length() > 1 && strTemp[0] == _T('\"') && str[strTemp.length() - 1 ] == _T('\"') )
        return strTemp.substr( 1, strTemp.length() - 2 );
    else
        return strTemp;
}

std::wstring stringutils::erase_trailing_whitespaces( const std::wstring& str ){

    std::wstring whitespaces (_T(" \t\f\v\n\r"));
    size_t found;
    std::wstring strTemp;
    strTemp = str;
    if ( !strTemp.length() ){   
        strTemp.clear();
    }
    else{      
        found = strTemp.find_last_not_of(whitespaces);
        if ( found != std::wstring::npos )
            strTemp.erase( found + 1 );
        else
            strTemp.clear();
    }
    return strTemp;
}

std::wstring stringutils::remove_begining_whitespaces( const std::wstring& str ){

    std::wstring whitespaces (_T(" \t\f\v\n\r"));
    size_t found;
    std::wstring strTemp;
    strTemp = str;
    if ( !strTemp.length() ) return strTemp;

    found = strTemp.find_first_not_of(whitespaces);
        
    if ( found != 0 ){
        if ( found != std::wstring::npos )    {
            strTemp = strTemp.substr( found, strTemp.length() - found );
        }
        else
            strTemp.clear();
    }
    return strTemp;
}

std::vector<std::wstring> stringutils::tokenize2( const std::wstring& str, const std::wstring& delimiters, size_t num, bool empty ){

    std::vector<std::wstring> tokens;
    tokens = tokenize( str, delimiters, num, empty );

    for( size_t i = 0 ; i < tokens.size(); i ++ ){
        tokens[i] = erase_trailing_whitespaces( tokens[i] );
        tokens[i] = remove_begining_whitespaces( tokens[i] );
    }

    return tokens;
}

std::vector<std::wstring> stringutils::tokenize( const std::wstring& str, const std::wstring& delimiters, size_t num, bool empty ){

    std::vector<std::wstring> tokens;
    std::wstring::size_type delimPos = 0, tokenPos = 0, pos = 0;

    if( str.length() < 1 ) return tokens;
        
    size_t count = 1;

    while( 1 ){
        delimPos = str.find_first_of( delimiters, pos );
        tokenPos = str.find_first_not_of( delimiters, pos );

        if( ( count != num ) && ( std::wstring::npos != delimPos ) ){
            if( std::wstring::npos != tokenPos ){
                if( tokenPos < delimPos ){
                    tokens.push_back(str.substr( pos, delimPos-pos ) );
                    count ++;
                }
                else if ( empty ){
                    tokens.push_back( _T("") );
                    count ++;
                }
            }
            else if ( empty ){
                tokens.push_back( _T("") );
                count ++;
            }
            pos = delimPos + 1;       
        }
        else{
            if( std::wstring::npos != tokenPos )
                tokens.push_back( str.substr( pos ) );
            else if (empty)
                tokens.push_back( _T("") );
            break;
        }
    }
    return tokens;
}

std::string& stringutils::tolower(std::string& str){
    std::transform(str.begin(), str.end(), str.begin(), std::tolower);
    return str;
}

std::string& stringutils::toupper(std::string& str){
    std::transform(str.begin(), str.end(), str.begin(), std::toupper);
    return str;
}

std::wstring& stringutils::tolower(std::wstring& str){
    std::transform(str.begin(), str.end(), str.begin(), towlower);
    return str;
}

std::wstring& stringutils::toupper(std::wstring& str){
    transform(str.begin(), str.end(), str.begin(), towupper);
    return str;
}

std::vector<std::wstring> stringutils::split( std::wstring& str, std::wstring subStr ){
    std::vector<std::wstring> splits;
    size_t pos = 0, prepos = 0;
    if ( !str.length() ){
    }
    else if ( !subStr.length() ){
        splits.push_back( str );
    }
    else{
        while( true ){
            pos = str.find( subStr, prepos ? prepos + subStr.length() : 1 );
            if ( std::wstring::npos == pos ){
                std::wstring sub = str.substr( prepos, -1 );
                splits.push_back( sub );
                break;
            }
            else{
                std::wstring sub = str.substr( prepos, pos - prepos );
                splits.push_back( sub );
                prepos = pos;
            }
        }
    }
    return splits;
}

std::string stringutils::replace_double_slash_with_one(const std::string& str){

    std::string strTemp;
    int                nIndex = 0;
    int                nNewIndex = 0;
    strTemp.resize(str.length() + 1);
    while (nIndex < (int)str.length()) {
        if (str[nIndex] == _T('\\') && (nIndex < (int)str.length() - 1) && str[nIndex + 1] == _T('\\')){
            strTemp[nNewIndex++] = _T('\\');
            nIndex += 2;
        }
        else
            strTemp[nNewIndex++] = str[nIndex++];
    }
    strTemp[nNewIndex++] = _T('\0');
    return strTemp;
}

std::string stringutils::replace_one_slash_with_two(const std::string& str){

    std::string        strTemp;
    int                nIndex = 0;
    int                nNewIndex = 0;
    strTemp.resize(str.length() + 1);
    while (nIndex < (int)str.length())
    {
        if (str[nIndex] == _T('\\') && (nIndex < (int)str.length() - 1) && str[nIndex + 1] == _T('\\')){
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            nIndex += 2;
            strTemp.resize(strTemp.length() + 2);
        }
        else if (str[nIndex] == _T('\\') && ((nIndex == (int)str.length() - 1) || str[nIndex + 1] != _T('\\'))){
            strTemp[nNewIndex++] = _T('\\');
            strTemp[nNewIndex++] = _T('\\');
            nIndex += 1;
            strTemp.resize(strTemp.length() + 1);
        }
        else
            strTemp[nNewIndex++] = str[nIndex++];
    }
    strTemp[nNewIndex++] = _T('\0');
    return strTemp;
}

std::string  stringutils::parser_double_quotation_mark(const std::string& str)
{
    std::string strTemp = erase_trailing_whitespaces(str);
    strTemp = remove_begining_whitespaces(strTemp);

    if (strTemp.length() > 1 && strTemp[0] == _T('\"') && str[strTemp.length() - 1] == _T('\"'))
        return strTemp.substr(1, strTemp.length() - 2);
    else
        return strTemp;
}

std::string stringutils::erase_trailing_whitespaces(const std::string& str){

    std::string whitespaces(" \t\f\v\n\r");
    size_t found;
    std::string strTemp;
    strTemp = str;
    if (!strTemp.length()){
        strTemp.clear();
    }
    else{
        found = strTemp.find_last_not_of(whitespaces);
        if (found != std::string::npos)
            strTemp.erase(found + 1);
        else
            strTemp.clear();
    }
    return strTemp;
}

std::string stringutils::remove_begining_whitespaces(const std::string& str){

    std::string whitespaces(" \t\f\v\n\r");
    size_t found;
    std::string strTemp;
    strTemp = str;
    if (!strTemp.length()) return strTemp;

    found = strTemp.find_first_not_of(whitespaces);

    if (found != 0){
        if (found != std::string::npos)    {
            strTemp = strTemp.substr(found, strTemp.length() - found);
        }
        else
            strTemp.clear();
    }
    return strTemp;
}

std::vector<std::string> stringutils::tokenize2(const std::string& str, const std::string& delimiters, size_t num, bool empty){

    std::vector<std::string> tokens;
    tokens = tokenize(str, delimiters, num, empty);

    for (size_t i = 0; i < tokens.size(); i++){
        tokens[i] = erase_trailing_whitespaces(tokens[i]);
        tokens[i] = remove_begining_whitespaces(tokens[i]);
    }

    return tokens;
}

std::vector<std::string> stringutils::tokenize(const std::string& str, const std::string& delimiters, size_t num, bool empty){

    std::vector<std::string> tokens;
    std::string::size_type delimPos = 0, tokenPos = 0, pos = 0;

    if (str.length() < 1) return tokens;

    size_t count = 1;

    while (1){
        delimPos = str.find_first_of(delimiters, pos);
        tokenPos = str.find_first_not_of(delimiters, pos);

        if ((count != num) && (std::string::npos != delimPos)){
            if (std::string::npos != tokenPos){
                if (tokenPos < delimPos){
                    tokens.push_back(str.substr(pos, delimPos - pos));
                    count++;
                }
                else if (empty){
                    tokens.push_back("");
                    count++;
                }
            }
            else if (empty){
                tokens.push_back("");
                count++;
            }
            pos = delimPos + 1;
        }
        else{
            if (std::string::npos != tokenPos)
                tokens.push_back(str.substr(pos));
            else if (empty)
                tokens.push_back("");
            break;
        }
    }
    return tokens;
}

std::vector<std::string> stringutils::split(std::string& str, std::string subStr){
    std::vector<std::string> splits;
    size_t pos = 0, prepos = 0;
    if (!str.length()){
    }
    else if (!subStr.length()){
        splits.push_back(str);
    }
    else{
        while (true){
            pos = str.find(subStr, prepos ? prepos + subStr.length() : 1);
            if (std::string::npos == pos){
                std::string sub = str.substr(prepos, -1);
                splits.push_back(sub);
                break;
            }
            else{
                std::string sub = str.substr(prepos, pos - prepos);
                splits.push_back(sub);
                prepos = pos;
            }
        }
    }
    return splits;
}

#endif

};

#endif
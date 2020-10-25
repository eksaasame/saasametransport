// xor_crypto.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once
#ifndef __MACHO_XOR_CRYPTO_INCLUDE__
#define __MACHO_XOR_CRYPTO_INCLUDE__

#include "..\config\config.hpp"
#include "bytes.hpp"
#include "base32.hpp"

namespace macho{

class xor_crypto{

public:
    static std::string encrypt( std::string value, std::string key ){
        bytes retvalue( (LPBYTE)value.c_str(), value.length());
        if ( retvalue.length() < minimize_size ) retvalue.resize( minimize_size );
        encrypt( retvalue, key );
        int    base32len = base32::get_encode32_length( (int)retvalue.length() );
        if ( base32len > 0 ){
            std::auto_ptr<BYTE> base32_value = std::auto_ptr<BYTE>( new BYTE[base32len] );
            if ( base32::encode32( retvalue.ptr(), (int) retvalue.length(), base32_value.get() ) ){
                if ( base32::map32( base32_value.get(), base32len ) ){
                    value.assign( (LPSTR)base32_value.get(), base32len);
                    return value;
                }
            }
        }
        return "";
    }

    static void inline encrypt( bytes& value, std::string key ){
        key.reserve();
        _xor(value, key);
        _xor(value, water_mark);
    }

    static std::string encrypt2( std::string value, std::string key1, std::string key2 ){
        bytes retvalue( (LPBYTE)value.c_str(), value.length());
        if ( retvalue.length() < minimize_size ) retvalue.resize( minimize_size );
        encrypt2( retvalue, key1, key2 );
        int    base32len = base32::get_encode32_length( (int) retvalue.length() );
        if ( base32len > 0 ){
            std::auto_ptr<BYTE> base32_value = std::auto_ptr<BYTE>( new BYTE[base32len] );
            if ( base32::encode32( retvalue.ptr(), (int) retvalue.length(), base32_value.get() ) ){
                if ( base32::map32( base32_value.get(), base32len ) ){
                    value.assign( (LPSTR)base32_value.get(), base32len );
                    return value;
                }
            }
        }
        return "";
    }

    static void inline encrypt2( bytes& value, std::string key1, std::string key2 ){
        key1.reserve();
        key2.reserve();
        _xor(value, key1);
        _xor(value, key2);
        _xor(value, water_mark);
    }

    static std::string decrypt( std::string value, std::string key ){
        int base32len = (int) value.length();
        std::auto_ptr<BYTE> base32_value = std::auto_ptr<BYTE>( new BYTE[base32len] );
        memcpy( base32_value.get(), value.c_str(), base32len);
        if ( base32::unmap32( (LPBYTE) base32_value.get(), base32len ) ){
            if ( base32::decode32( (LPBYTE) base32_value.get(), base32len, base32_value.get() ) ){
                int datalen = base32::get_decode32_length(base32len);
                bytes retvalue( base32_value.get(), datalen ); 
                decrypt( retvalue, key );
                return value.assign((LPCSTR)retvalue.ptr(), retvalue.length());
            }
        }
        return "";
    }

    static void inline decrypt( bytes& value, std::string key ){
        key.reserve();
        _xor(value, water_mark);
        _xor(value, key);
    }
    
    static std::string decrypt2( std::string value, std::string key1, std::string key2 ){
        int base32len = (int) value.length();
        std::auto_ptr<BYTE> base32_value = std::auto_ptr<BYTE>( new BYTE[base32len] );
        memcpy( base32_value.get(), value.c_str(), base32len);
        if ( base32::unmap32( (LPBYTE) base32_value.get(), base32len ) ){
            if ( base32::decode32( (LPBYTE) base32_value.get(), base32len, base32_value.get() ) ){
                int datalen = base32::get_decode32_length(base32len);
                bytes retvalue( base32_value.get(), datalen ); 
                decrypt2( retvalue, key1, key2 );
                return value.assign((LPCSTR)retvalue.ptr(), retvalue.length());
            }
        }
        return "";
    }

    static void inline decrypt2( bytes& value, std::string key1, std::string key2 ){
        key1.reserve();
        key2.reserve();
        _xor(value, water_mark);
        _xor(value, key2);
        _xor(value, key1);
    }

private:
    static const size_t      minimize_size;
    static const std::string water_mark; 
    static void inline _xor( bytes& value, const std::string& key ){
        unsigned int klen = (int) key.length();
        unsigned int vlen = (int) value.length();
        for( unsigned int v = 0; v < vlen; v++ )
            value[v] ^= key[ v % klen ];
        /*
        std::auto_ptr<BYTE> retval = std::auto_ptr<BYTE>( new BYTE[vlen]);
        for( unsigned int v = 0; v < vlen; v++ )
            retval.get()[v]=value.ptr()[v]^key[ v % klen ];
        value.set( retval.get(), vlen );
        */
    }
};

#ifndef MACHO_HEADER_ONLY

const size_t      xor_crypto::minimize_size = 20;
const std::string xor_crypto::water_mark = "Copyright (C) 2016 SaaSaMe. All Rights Reserved.";

#endif

};
#endif
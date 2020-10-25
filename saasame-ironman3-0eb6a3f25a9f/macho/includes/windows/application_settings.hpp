// application_settings.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_APPLICATION_SETTINGS__
#define __MACHO_WINDOWS_APPLICATION_SETTINGS__
#include "..\config\config.hpp"
#include "environment.hpp"
#include "registry.hpp"
#include "protected_data.hpp"

namespace macho{

namespace windows{

class application_settings{
public:
    application_settings( stdstring key_path, HKEY hkey = HKEY_LOCAL_MACHINE, bool iswow64_64key = false ){ 
        _path   = key_path; 
        _hkey   = hkey; 
        _flags  = iswow64_64key ? ( REGISTRY_FLAGS_ENUM ) ( environment::is_wow64_process() ? REGISTRY_WOW64_64KEY : REGISTRY_NONE ) : REGISTRY_NONE  ; 
    }
    virtual ~application_settings(){}
    bool set_value( stdstring name, stdstring value );
    bool set_value( stdstring name, DWORD value );
    bool set_value( stdstring name, ULONGLONG value );
    bool set_value( stdstring name, bytes value );
    bool set_value( stdstring name, string_array values );
    bool set_protected_value( stdstring name, std::string value, bool pre_machine = false );
    bool set_protected_value( stdstring name, std::wstring value, bool pre_machine = false );
    bool set_protected_value( stdstring name, bytes value, bool pre_machine = false );
    bool delete_value( stdstring name );
    bool get_value( stdstring name, stdstring &value );
    bool get_value( stdstring name, bytes &value );
    bool get_value( stdstring name, DWORD &value );
    bool get_value( stdstring name, ULONGLONG &value );
    bool get_value( stdstring name, string_array &values );
    bool get_protected_value( stdstring name, std::string &value, bool pre_machine = false );
    bool get_protected_value( stdstring name, std::wstring &value, bool pre_machine = false );
    bool get_protected_value( stdstring name, bytes &value, bool pre_machine = false );

private:
    stdstring           _path;
    HKEY                _hkey;
    REGISTRY_FLAGS_ENUM _flags;    
};

#ifndef MACHO_HEADER_ONLY

bool application_settings::delete_value( stdstring name ){

    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags ) ) ;
        if( reg.open( _hkey, _path ) ){
            if ( !( bRet = !reg[name].exists() ) )
                bRet = reg[name].delete_value();
        }
    }
    return bRet;
}

bool application_settings::set_value( stdstring name, stdstring value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_CREATE ) ) ;
        if( reg.open( _hkey, _path ) ){
            reg[name] = value;
            bRet = reg[name].exists();
        }
    }
    return bRet;
}

bool application_settings::set_value( stdstring name, DWORD value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_CREATE ) ) ;
        if( reg.open( _hkey, _path ) ){
            reg[name] = value;
            bRet = reg[name].exists();
        }
    }
    return bRet;
}

bool application_settings::set_value( stdstring name, ULONGLONG value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_CREATE ) ) ;
        if( reg.open( _hkey, _path ) ){
            reg[name] = value;
            bRet = reg[name].exists();
        }
    }
    return bRet;
}

bool application_settings::set_value( stdstring name, bytes value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_CREATE ) ) ;
        if( reg.open( _hkey, _path ) ){
            reg[name] = value;
            bRet = reg[name].exists();
        }
    }
    return bRet;
}

bool application_settings::set_value( stdstring name, string_array values ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_CREATE ) ) ;
        if( reg.open( _hkey, _path ) ){
            reg[name].clear_multi();
            foreach( stdstring value, values ){
                reg[name].add_multi( value );
            }
            bRet = reg[name].exists();
        }
    }
    return bRet;
}

bool application_settings::set_protected_value( stdstring name, std::string value, bool pre_machine ){
    std::wstring wsz_value = stringutils::convert_ansi_to_unicode(value);
    return set_protected_value(name, wsz_value, pre_machine);
}

bool application_settings::set_protected_value( stdstring name, std::wstring value, bool pre_machine ){
    
    bool bRet = false;
    bytes source;
    source.set((LPBYTE) value.c_str(), 2 * value.length());
    bRet = set_protected_value( name, source, pre_machine );
    return bRet;
}

bool application_settings::set_protected_value( stdstring name, bytes value, bool pre_machine ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_CREATE ) ) ;
        if( reg.open( _hkey, _path ) ){
            bytes data = protected_data::protect( value, pre_machine );
            reg[name] = data;
            bRet = reg[name].exists();
        }
    }
    return bRet;
}

bool application_settings::get_value( stdstring name, stdstring &value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_READONLY ) ) ;
        if( reg.open( _hkey, _path ) ){
            if ( ( bRet = reg[name].exists() ) && ( bRet = reg[name].is_string() ) ){
#if _UNICODE
                value = reg[name].wstring();
#else
                value = reg[name].string();
#endif
            }
        }
    }
    return bRet;
}

bool application_settings::get_value( stdstring name, bytes &value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_READONLY ) ) ;
        if( reg.open( _hkey, _path ) ){
            if ( ( bRet = reg[name].exists() ) && ( bRet = reg[name].is_binary() ) ){
                value = (bytes)reg[name];
            }
        }
    }
    return bRet;
}

bool application_settings::get_value( stdstring name, DWORD &value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_READONLY ) ) ;
        if( reg.open( _hkey, _path ) ){
            if (  bRet = reg[name].exists() ){
                if ( bRet = reg[name].is_dword()){
                    value = (DWORD)reg[name];
                }
            }
        }
    }
    return bRet;
}

bool application_settings::get_value( stdstring name, ULONGLONG &value ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_READONLY ) ) ;
        if( reg.open( _hkey, _path ) ){
            if ( ( bRet = reg[name].exists() ) && ( bRet = reg[name].is_qword() ) ){
                value = (ULONGLONG)reg[name];
            }
        }
    }
    return bRet;
}

bool application_settings::get_value( stdstring name, string_array &values ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_READONLY ) ) ;
        if( reg.open( _hkey, _path ) ){
            if ( ( bRet = reg[name].exists() ) && ( bRet = reg[name].is_multi_sz() ) ){
                for( int i = 0 ; i < (int)reg[name].get_multi_count(); i++ ){
                    values.push_back( reg[name].get_multi_at(i) );
                }
            }
        }
    }
    return bRet;
}

bool application_settings::get_protected_value( stdstring name, std::string &value, bool pre_machine ){
    std::wstring wsz_value;
    bool bRet = get_protected_value(name, wsz_value, pre_machine);
    if ( bRet ){
        value = stringutils::convert_unicode_to_ansi(wsz_value);
    }
    return bRet;
}

bool application_settings::get_protected_value( stdstring name, std::wstring &value, bool pre_machine ){
    
    bool bRet = false;
    bytes data;
    if ( bRet = get_protected_value( name, data, pre_machine ) ){
        value = ( LPWSTR ) data.ptr();
        size_t len = data.length() / 2; 
        if ( value.length() >  len ){
            value.erase( len );
        }
    }
    return bRet;
}

bool application_settings::get_protected_value( stdstring name, bytes &value, bool pre_machine ){
    
    bool bRet = false;
    if ( _path.length() ){
        registry reg(( REGISTRY_FLAGS_ENUM ) ( _flags | REGISTRY_READONLY ) ) ;
        if( reg.open( _hkey, _path ) ){
            if ( ( bRet = reg[name].exists() ) && ( bRet = reg[name].is_binary() ) ){
                bytes data = (bytes)reg[name];
                value = protected_data::unprotect( data, pre_machine );
            }
        }
    }
    return bRet;
}

#endif

};//namespace windows
};//namespace macho

#endif
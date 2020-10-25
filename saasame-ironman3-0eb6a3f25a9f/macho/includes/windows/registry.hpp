// registry.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_REGISTRY__
#define __MACHO_WINDOWS_REGISTRY__

#include "..\config\config.hpp"
#include "..\common\bytes.hpp"
#include "..\common\stringutils.hpp"
#include "reg_native_edit.hpp"
#include "boost\shared_ptr.hpp"

namespace macho{

namespace windows{

#ifndef QWORD
typedef ULONGLONG           QWORD;
typedef QWORD near          *PQWORD;
typedef QWORD far           *LPQWORD;
#endif

typedef enum _REGISTRY_FLAGS_ENUM{
REGISTRY_NONE                    = 0x00000000,
REGISTRY_READONLY                = 0x00000001,
REGISTRY_CREATE                  = 0x00000002,
REGISTRY_WOW64_64KEY             = 0x00000004, //Use this flag to disable registry WOW64 Redirector
REGISTRY_READONLY_WOW64_64KEY    = REGISTRY_READONLY | REGISTRY_WOW64_64KEY ,
REGISTRY_CREATE_WOW64_64KEY      = REGISTRY_CREATE   | REGISTRY_WOW64_64KEY
}REGISTRY_FLAGS_ENUM;

// Internal usage flags
#define REGISTRY_INTERNAL        0x80000000
#define REGISTRY_LOADING         0x40000000
#define REGISTRY_AUTO_OPEN       0x20000000

#define _MAX_REG_VALUE_NAME       16383
#define _MAX_REG_VALUE            1048576    // Maximum Value length, this may be increased

class registry;

class reg_value{
public:
    reg_value( registry& reg, stdstring name = _T(""), UINT type = REG_NONE ) 
        : _stored(false),
        _flags(0),
        _reg(reg){
        _name = name;
        _type = type;
    }
    virtual ~reg_value(){}
    LONG set_value( UINT type, const LPBYTE lpbuf, DWORD size );
    bool delete_value();

        /* -----------------------------------------*
     *    Operators                                *
     * -----------------------------------------*/
    reg_value&    operator =( reg_value& value );
    reg_value&    operator =( LPCTSTR lpvalue ){
        size_t    size = (_tcslen(lpvalue) + 1)*sizeof(TCHAR);
        assert(size <= _MAX_REG_VALUE);
        set_value( REG_SZ, (const LPBYTE)lpvalue, (DWORD) size );
        return *this;
    }
    reg_value&    operator =( std::string value )  { 
#if _UNICODE
        return (*this = stringutils::convert_ansi_to_unicode(value).c_str());
#else
        return (*this = value.c_str());
#endif
    }
    reg_value&    operator =( std::wstring value ) { 
#if _UNICODE
        return (*this = value.c_str());
#else
        return (*this = stringutils::convert_unicode_to_ansi(value).c_str());
#endif
    }
    reg_value&    operator =( INT   value )     { return (*this = (LPDWORD)&value); }
    reg_value&    operator =( UINT  value )     { return (*this = (LPDWORD)&value); }
    reg_value&    operator =( LONG  value )     { return (*this = (LPDWORD)&value); }
    reg_value&    operator =( DWORD value )     { return (*this = &value);          }
    reg_value&    operator =( bytes value )     {
        set_value( REG_BINARY, (const LPBYTE)value.ptr(), (DWORD)value.length() );
        return *this;   
    }
    reg_value&    operator =( LPDWORD lpvalue ) {
        set_value( REG_DWORD, (const LPBYTE)lpvalue, sizeof(DWORD) );
        return *this;
    }
    reg_value&    operator =( LONGLONG value )    { return (*this = (PULONGLONG)&value);  }
    reg_value&    operator =( ULONGLONG value )   { return (*this = &value);              }
    reg_value&    operator =( PULONGLONG lpvalue ){
        set_value( REG_QWORD, (const LPBYTE)lpvalue, sizeof(ULONGLONG) );
        return *this;
    }
    std::wstring wstring() const {
#if _UNICODE
        return _string;
#else
        return stringutils::convert_ansi_to_unicode(_string);
#endif
    }
    std::string string() const { 
#if _UNICODE
        return stringutils::convert_unicode_to_ansi(_string); 
#else
        return _string;
#endif
    }
    operator bytes() const { return _bytes;       }
    //operator LPCTSTR() { return _string.c_str();  }
    operator LPBYTE()  { return _bytes.ptr();     } 
    operator INT()     { return operator DWORD(); }
    operator UINT()    { return operator DWORD(); }
    operator LONG()    { return operator DWORD(); }
    operator DWORD()   { 
        DWORD value = 0;
        if ( is_dword() && _bytes.length() == sizeof(DWORD) )
            value = *((LPDWORD)_bytes.ptr()); 
        else if ( is_string() )
            value = _tcstoul( _string.c_str(), NULL, NULL );
        else if ( is_binary() )
            value = _tcstoul( (LPTSTR)_bytes.ptr(), NULL, NULL );
        return value;
    }
    operator LONGLONG()  { return operator ULONGLONG();}
    operator ULONGLONG() { 
        ULONGLONG value = 0;
        if ( is_qword() && _bytes.length() == sizeof( ULONGLONG ) )
            value = *((PULONGLONG)_bytes.ptr()); 
        else if ( is_string() )
            value = _ttoi64( _string.c_str() );
        else if ( is_binary() )
            value = _ttoi64( (LPTSTR)_bytes.ptr() );
        return value;
    }       
//
    void inline             set_binary(LPBYTE lpvalue, size_t nLen ){
        if (!nLen) return; 
        set_value( REG_BINARY, lpvalue, (DWORD)nLen );                                
    }    

    void inline             set_expand_sz(LPCTSTR lpszValue){
        size_t    nLen = (_tcslen(lpszValue) + 1)*sizeof(TCHAR);
        assert(nLen <= _MAX_REG_VALUE);
        set_value( REG_EXPAND_SZ, (const LPBYTE)lpszValue, (DWORD) nLen );
    }
    
    void inline             set_multi(LPCTSTR lpszValue, size_t nLen ){
        set_value( REG_MULTI_SZ, (const LPBYTE)lpszValue, (DWORD) nLen * sizeof(TCHAR) );
    }

    void                    set_multi_at( size_t index, LPCTSTR lpszVal ); 
    void inline             add_multi ( LPCTSTR lpszVal ) { set_multi_at( get_multi_count(), lpszVal ); }
    void inline             add_multi ( stdstring value ) { add_multi( value.c_str()); }

    void                    remove_multi_at( size_t index );
    void                    remove_multi ( LPCTSTR lpszVal );
    void inline             remove_multi (stdstring value) { remove_multi(value.c_str()); }

    void inline             clear_multi() { is_stored() ? set_multi(_T("\0"), 1) : __noop; }

    LPTSTR                  get_multi(LPTSTR lpszDest, size_t nMax = _MAX_REG_VALUE);
    size_t                  get_multi_length();
    size_t  inline          get_multi_count() { return _multi_sz.size(); }
    LPCTSTR inline          get_multi_at( size_t index ) {
        if ( is_multi_sz() && _multi_sz.size( ) > index )
            return _multi_sz[index].c_str();
        else
            return NULL;
    }

    void inline            get_binary(LPBYTE lpDest, size_t nMaxLen){
        _bytes.get( lpDest, nMaxLen );
    }
    size_t inline        get_binary_length() { return _bytes.length();  }

    template <class T>void set_struct(T &type) { set_binary((LPBYTE) &type, sizeof(T)); }
    template <class T>void get_struct(T &type) { get_binary((LPBYTE) &type, sizeof(T)); }

    bool inline            is_string()        const { return ( _type == REG_SZ || _type == REG_EXPAND_SZ ); }
    bool inline            is_dword()         const { return (_type == REG_DWORD);    }
    bool inline            is_qword()         const { return (_type == REG_QWORD);    }
    bool inline            is_binary()        const { return (_type == REG_BINARY);   }
    bool inline            is_none()          const { return (_type == REG_NONE);     }    
    bool inline            is_multi_sz()      const { return (_type == REG_MULTI_SZ); }
    bool inline            is_stored()        const { return _stored; }
    bool inline            exists()           const { return _stored; }
    UINT inline            type()             const { return _type;   }
    stdstring inline       name()             const { return _name;   }
private:
    stdstring       _name;
    UINT            _type;
    UINT            _flags;
    bytes           _bytes;
    stdstring       _string;
    string_array    _multi_sz;
    bool            _stored;
    registry&       _reg;
};

class registry{
public:
    static reg_native_edit _reg_native_edit;
    registry( REGISTRY_FLAGS_ENUM flags = REGISTRY_NONE, reg_edit_base& edit = _reg_native_edit ) 
        : _edit(edit),
        _key(NULL),
        _root_key(HKEY_LOCAL_MACHINE),
        _stored(false),
        _flags((UINT)flags) { }  

    registry( reg_edit_base& edit, REGISTRY_FLAGS_ENUM flags = REGISTRY_NONE ) 
        : _edit(edit),
        _key(NULL),
        _root_key(HKEY_LOCAL_MACHINE),
        _stored(false),
        _flags((UINT)flags) { }  

    virtual ~registry(){ close(); }
    
    stdstring inline key_path()    const { return _sub_key; }
    stdstring        key_name();
    // value functions
    reg_value&                    operator[](LPCTSTR name);
    reg_value&                    operator[](stdstring name) { return operator[](name.c_str()); }
    reg_value&                    operator[](size_t index);
    int     inline count()        const { return (int)_reg_values.size(); }

    // Key functions
    bool        open( LPCTSTR key_path, HKEY hkey = HKEY_LOCAL_MACHINE );
    bool inline open( HKEY hkey, LPCTSTR key_path ) { return open( key_path, hkey ); }
    bool inline open( stdstring key_path, HKEY hkey = HKEY_LOCAL_MACHINE ) { return  open( key_path.c_str(), hkey ); }
    bool inline open( HKEY hkey, stdstring key_path ) { return  open( key_path.c_str(), hkey ); }
    bool inline open()                                { return open( _root_key, _sub_key.c_str() ) ;}
    bool        refresh();
    void inline close() { if (_key != NULL) { _edit.close_key(_key); _key = NULL; } }
    bool        delete_key();

    // subkeys operations
    bool              refresh_subkeys();
    registry&         subkey    ( LPCTSTR key_name, bool is_create = false );
    registry&         subkey    ( stdstring key_name, bool is_create = false ) { return subkey(key_name.c_str(), is_create ); }
    registry&         subkey ( int index );
    int        inline subkeys_count()        const { return (int)_reg_subkeys.size() ; } 
    bool              delete_subkeys();

    // status check
    bool inline is_readonly()        const { return (_flags & REGISTRY_READONLY )? true : false; }
    bool inline is_key_valid();

    bool inline exists()        const { return _stored; }
    
private:
    friend LONG reg_value::set_value( UINT iType, const LPBYTE lpBuf, DWORD size );
    friend bool reg_value::delete_value();
    bool inline is_loading_operation()  const{ return (_flags & REGISTRY_LOADING) ? true : false; }
    bool inline is_internal_operation() const{ return (_flags & REGISTRY_INTERNAL) ? true : false; }

    reg_edit_base&                              _edit;
    UINT                                        _flags;
    HKEY                                        _key;
    HKEY                                        _root_key;
    bool                                        _stored;
    stdstring                                   _sub_key;
    std::vector< boost::shared_ptr<reg_value> > _reg_values;
    std::vector< boost::shared_ptr<registry> >  _reg_subkeys; 
};

#ifndef MACHO_HEADER_ONLY

#define REGISTRY_NOINTERNAL \
    !( _flags & REGISTRY_INTERNAL )

#define REGISTRY_NOLOADING \
    !( _flags & REGISTRY_LOADING )

#define REGISTRY_SETLOADING(op) \
    _flags op= REGISTRY_LOADING

#define REGISTRY_SETINTERNAL(op) \
    _flags op= REGISTRY_INTERNAL

reg_native_edit registry::_reg_native_edit;

/*******************************************************************************
        registry functions
*******************************************************************************/

stdstring registry::key_name(){
    stdstring::size_type  pos = 0;
    if ((pos = _sub_key.find_last_of(_T("\\"))) != stdstring::npos)
        return _sub_key.substr(pos + 1, _sub_key.length() - (pos + 1)).c_str();
    else
        return _sub_key.c_str();
}

bool inline registry::is_key_valid() {
    if (_key != NULL)
        return true;
    else if (_flags & REGISTRY_AUTO_OPEN) {
        REGISTRY_SETINTERNAL(+);
        open();
        REGISTRY_SETINTERNAL(-);
    }
    return (_key != NULL) ? true : false;
}

reg_value&    registry::operator[](size_t index){
    if ( (REGISTRY_NOLOADING) && !_reg_values.size() ) refresh();
    assert( index <= _reg_values.size() );
    return *(_reg_values[index].get());
}

reg_value&  registry::operator []( LPCTSTR name ) {
    
    boost::shared_ptr< reg_value> p_value;
    assert( ( _tcslen(name) + 1 ) <= _MAX_REG_VALUE);
    
    if ( (REGISTRY_NOLOADING) && !_reg_values.size() ) refresh();

    foreach( boost::shared_ptr< reg_value> p_regvalue,  _reg_values ){
        if( !_tcsicmp( p_regvalue.get()->name().c_str(), name) ){
            p_value = p_regvalue; 
            break;
        }
    }

    if ( p_value.get() == NULL ){
        p_value = boost::shared_ptr< reg_value>( new reg_value( *this, name, REG_NONE ) );
        _reg_values.push_back(p_value);
    }
    return *p_value.get();
}

bool registry::open( LPCTSTR key_path, HKEY hkey ){
    
    bool  bNew = true;
    DWORD access ;

    /* The key is being opened manually, if the key location differs
    from the last opened location, clear the current entries and
    store the path information for future auto opening and key
    deletion using DeleteKey() */

    if ( _sub_key.length() ){
        if (_tcsicmp( key_path, _sub_key.c_str())) {                
            /* If new key, clear any currently stored entries and SubKeys */
            _reg_values.clear();
            _reg_subkeys.clear();
        } else bNew = false;
    }

    if ( bNew ) {
        _sub_key = key_path;
        if ( _sub_key.length() && _sub_key[ _sub_key.length() -1 ] == _T('\\') )
            _sub_key.erase( _sub_key.length() );
    }
    
    _root_key = hkey;
        
    /* This is where the key is actually opened (if all goes well).
    If the key does not exist and the CREG_CREATE flag is present,
    it will be created... Any currently opened key will be closed
    before opening another one. After opening the key, Refresh() is
    called and the key's values    are stored in memory for future use. */
    close();
    if ( _flags & REGISTRY_READONLY )
        access = KEY_READ;
    else
        access = ( _flags & REGISTRY_CREATE ) ? ( KEY_ALL_ACCESS ) : ( KEY_ALL_ACCESS & (~KEY_CREATE_SUB_KEY) );

    if( _flags & REGISTRY_WOW64_64KEY )
        access |= KEY_WOW64_64KEY;

    /* Open or create the sub key, and return the result: */
    LONG lResult = (_flags & REGISTRY_CREATE ?
        _edit.create_key( _root_key, key_path, 0, NULL, REG_OPTION_NON_VOLATILE, access, NULL, &_key, NULL)
    : _edit.open_key(_root_key, key_path, 0, access, &_key));
    
    if ( lResult == ERROR_SUCCESS ) 
        _stored = true;
    SetLastError( lResult );
    return (lResult == ERROR_SUCCESS ? ( REGISTRY_NOINTERNAL ? refresh() : true ) : false);
}

bool registry::refresh(){

    DWORD    type;
    DWORD    name_len;
    DWORD    buffer_size;
    DWORD    value_count;
    std::auto_ptr<BYTE> pbuf( new BYTE[_MAX_REG_VALUE] ) ;
    std::auto_ptr<TCHAR> pvalue_name ( new TCHAR[_MAX_REG_VALUE_NAME] );

    if ( !is_key_valid() ) return false;
    
    if ( ERROR_SUCCESS != (_edit.query_info_key(_key, NULL, NULL, NULL, NULL, NULL, NULL, &value_count, NULL, NULL, NULL, NULL)))
        return false;

    /* set loading flag */
    REGISTRY_SETLOADING(+);
    
    foreach( boost::shared_ptr< reg_value> p_regvalue,  _reg_values ){
        p_regvalue.get()->delete_value();
    }

    for( DWORD index = 0; index < value_count; index++) {
        memset(pbuf.get(),0,_MAX_REG_VALUE);
        memset(pvalue_name.get(),0,_MAX_REG_VALUE_NAME * sizeof(TCHAR));
        name_len = _MAX_REG_VALUE_NAME; 
        buffer_size = _MAX_REG_VALUE;    

        if (ERROR_SUCCESS != (_edit.enumerate_value(_key, index, pvalue_name.get(), &name_len, NULL, &type, pbuf.get(), &buffer_size)))
            continue;

        switch (type) {

            case REG_QWORD:
                this[0][pvalue_name.get()] = (PULONGLONG)pbuf.get();        
                break;

            case REG_DWORD:        
                this[0][pvalue_name.get()] = (LPDWORD)pbuf.get();                
                break;
                
            case REG_SZ:
                this[0][pvalue_name.get()] = (LPCTSTR)pbuf.get();
                break;

            case REG_EXPAND_SZ:
                this[0][pvalue_name.get()].set_expand_sz((LPCTSTR)pbuf.get());
                break;    

            case REG_MULTI_SZ:
                this[0][pvalue_name.get()].set_multi((LPCTSTR)pbuf.get(), buffer_size/sizeof(TCHAR));                
                break;    

            case REG_BINARY:
                this[0][pvalue_name.get()].set_binary(pbuf.get(), (size_t)buffer_size);
                break;

            case REG_NONE:
                if ( buffer_size )
                       this[0][pvalue_name.get()].set_value( REG_NONE, pbuf.get(), (size_t)buffer_size);                
                break;
        }
    }

    for( int index = (int) _reg_values.size() - 1; index >= 0; --index ){
        if( !_reg_values[index].get()->exists() )
            _reg_values.erase(_reg_values.begin()+index);
    }

    REGISTRY_SETLOADING(-);
    return true;
}

bool registry::refresh_subkeys(){

    _TCHAR    key_name[_MAX_PATH];    
    DWORD   count=0;               // number of subkeys 

    if ( !is_key_valid() ) return false;

    if (ERROR_SUCCESS != (_edit.query_info_key( _key, NULL, NULL, NULL, &count, NULL, NULL, NULL, NULL, NULL, NULL, NULL)))
        return false;

    REGISTRY_SETLOADING(+);
    
    foreach( boost::shared_ptr<registry> p_existed_subkey, _reg_subkeys ){
        p_existed_subkey.get()->_stored = false;
    }

    for( DWORD index = 0; index < count; index++ ) {
        DWORD                       name_size         = _MAX_PATH;
        boost::shared_ptr<registry> p_subkey;
        stdstring                   full_sub_key_path = _sub_key;

        if ( ERROR_SUCCESS != ( _edit.enumerate_key( _key, index, key_name, &name_size, NULL, NULL, NULL, NULL)))
            continue;
        full_sub_key_path.append(_T("\\")).append(key_name);
        foreach( boost::shared_ptr<registry> p_existed_subkey, _reg_subkeys ){
            if ( !_tcsicmp( p_existed_subkey.get()->_sub_key.c_str(), full_sub_key_path.c_str() ) ){
                p_subkey = p_existed_subkey; 
                break;
            }
        }

        if ( !p_subkey.get() ){
            p_subkey = boost::shared_ptr<registry>( new registry( _edit, REGISTRY_NONE ) );
            p_subkey.get()->_flags =  ( _flags | REGISTRY_AUTO_OPEN ) & (~REGISTRY_LOADING );
            p_subkey.get()->_sub_key      = full_sub_key_path;
            p_subkey.get()->_root_key     = _root_key;                    
            _reg_subkeys.push_back( p_subkey );
        }
        p_subkey.get()->_stored  = true;
    }
    
    for( int index = (int) _reg_subkeys.size() - 1; index >= 0; --index ){
        if( !_reg_subkeys[index].get()->_stored )
            _reg_subkeys.erase(_reg_subkeys.begin()+index);
        else{
            _reg_subkeys[index].get()->open( _reg_subkeys[index].get()->_sub_key, _root_key );
            _reg_subkeys[index].get()->close();
        }
    }
    
    REGISTRY_SETLOADING(-);
    return true;
}

registry&   registry::subkey( LPCTSTR key_name, bool is_create ){

    boost::shared_ptr<registry> p_subkey;
    string_table arr_subkeys = stringutils::tokenize2( key_name, _T("\\/") );
    registry * p_reg = this;
    if ( !_reg_subkeys.size() ) refresh_subkeys();
    stdstring  full_sub_key_path = _sub_key;
    foreach( stdstring subkey, arr_subkeys ){
        boost::shared_ptr<registry> p_key;
        full_sub_key_path.append(_T("\\")).append(subkey);
        foreach( boost::shared_ptr<registry> p_existed_subkey, p_reg->_reg_subkeys ){
            if ( !_tcsicmp( p_existed_subkey.get()->_sub_key.c_str(), full_sub_key_path.c_str() ) ){
                p_key = p_existed_subkey; 
                break;
            }
        }
        if ( p_key.get() ){
            p_subkey = p_key;
            p_reg     = p_key.get();
            if ( !p_key.get()->exists() ){
                p_key.get()->_flags = REGISTRY_AUTO_OPEN | ( is_create ? ( _flags | REGISTRY_CREATE ) : ( _flags & ( ~REGISTRY_CREATE ) ) ) ;
                if ( !p_key.get()->open() )
                    break;
                else
                    p_key.get()->close();
            }
        }
        else{
            p_subkey = p_key = boost::shared_ptr<registry>( new registry( _edit, REGISTRY_NONE ) );
            p_key.get()->_flags        = REGISTRY_AUTO_OPEN | ( is_create ? ( _flags | REGISTRY_CREATE ) : ( _flags & ( ~REGISTRY_CREATE ) ) ) ;
            p_key.get()->_sub_key      = full_sub_key_path;
            p_key.get()->_root_key     = _root_key;  
            p_reg->_reg_subkeys.push_back( p_key );          
            if ( p_key.get()->open() ){
                p_key.get()->close();    
                p_reg = p_key.get();
            }
            else
                break;
        }
    }
    return *p_subkey.get();
}

registry&   registry::subkey  ( int index ){
    if ( !_reg_subkeys.size() ) refresh_subkeys();
    assert( index < (int)_reg_subkeys.size() );
    return *(_reg_subkeys[index].get());
}

bool        registry::delete_key(){

    if ( delete_subkeys() ){
        LONG error = ERROR_SUCCESS;
        close();
        if ( REGISTRY_NOINTERNAL ){
            if( _flags & REGISTRY_WOW64_64KEY )
                error = _edit.delete_key( _root_key, _sub_key.c_str(), KEY_WOW64_64KEY, 0 );
            else
                error = _edit.delete_key( _root_key, _sub_key.c_str() );
        }
        if ( ( ERROR_SUCCESS == error ) || ( ERROR_FILE_NOT_FOUND == error ) ){
            _stored = false;
            _reg_values.clear();
            return true;    
        }
        SetLastError(error);
    }
    return false;
}

bool        registry::delete_subkeys(){

    LONG error = ERROR_SUCCESS;
    if ( REGISTRY_NOINTERNAL ){
        if ( is_readonly() )
            error = ERROR_ACCESS_DENIED;
        else if ( !is_key_valid() )
            error = ERROR_INVALID_HANDLE;
        else{
            error = _edit.delete_tree( _key, NULL );
        }
    }
    if ( ( ERROR_SUCCESS == error ) || ( ERROR_FILE_NOT_FOUND == error ) ){
        REGISTRY_SETINTERNAL(+);      
        foreach( boost::shared_ptr<registry> p_existed_subkey, _reg_subkeys ){
            p_existed_subkey.get()->_flags += REGISTRY_INTERNAL;
            p_existed_subkey.get()->delete_key();
            p_existed_subkey.get()->_flags -= REGISTRY_INTERNAL;
        }
        REGISTRY_SETINTERNAL(-);     
        _reg_subkeys.clear();
        return true;
    }
    SetLastError(error);
    return false;
}


/*******************************************************************************
        reg_value functions
*******************************************************************************/

bool reg_value::delete_value(){
    if ( !_reg.is_loading_operation() ){
        if( _reg.is_readonly() ) return false;
        if( _reg.is_key_valid() ){
            ULONG error = _reg._edit.delete_value( _reg._key , _name.c_str() );
            if ( ( ERROR_SUCCESS == error ) || ( ERROR_FILE_NOT_FOUND == error ) ){
                _stored = false;
                return true;
            }
        }
    }
    else
        _stored = false;
    return false;
}

reg_value&    reg_value::operator =( reg_value& value ){
    
    if (this == &value)
        return *this;
    _name = value._name;
    switch( _type = value._type ){

    case REG_SZ:
        return (*this = value._string.c_str());
        break;
    case REG_EXPAND_SZ: {
        set_expand_sz( value._string.c_str());
        return *this;
        }
        break;
    case REG_MULTI_SZ: {
        size_t size = value.get_multi_length();
        std::auto_ptr<TCHAR> lpszBuf( new TCHAR[ size + 1 ] );
        set_multi( value.get_multi( lpszBuf.get(), size + 1 ), size );
        return *this;
        }
        break;
    case REG_BINARY: {
        set_binary( value._bytes.ptr(), value._bytes.length());
        return *this;
        }
        break;
    case REG_QWORD:{
            return (*this = (QWORD) value );
        }
        break;
    default:
        return (*this = (DWORD) value );
    }
    return *this;
}

LONG reg_value::set_value( UINT type, const LPBYTE lpbuf, DWORD size ){

    LONG res = ERROR_ACCESS_DENIED; 
    if ( ( !_reg.is_loading_operation() ) && _reg.is_readonly() ){
        // LOG for the access denied error; 
    }
    else{
        _type = type;
        switch ( type ){
        case REG_SZ:
        case REG_EXPAND_SZ:
            _string = (LPCTSTR)lpbuf;
            break;
        case REG_NONE: // Used in HKLM\SECURITY
        case REG_BINARY:    
        case REG_QWORD:
        case REG_DWORD:
            _bytes.set( lpbuf, size );
            break;
        case REG_MULTI_SZ:
            if ( REGISTRY_NOINTERNAL ) {
                size_t nCur = 0, nPrev = 0, nShortLen = size / sizeof(TCHAR);
                LPCTSTR lpszValue = (LPCTSTR)lpbuf;
                _multi_sz.clear();
                if (size > 2){  // if (nLen <= 2)  The string is empty : \0\0
                    if (*(lpszValue + nShortLen - 1) == _T('\0') )
                        nShortLen--;    
                    /* Populate a vector with each string part for easy and quick access */
                    while ((nCur = (int)(_tcschr(lpszValue+nPrev, _T('\0'))-lpszValue)) < nShortLen) {        
                        _multi_sz.push_back(lpszValue+nPrev);
                        nPrev = nCur+1;
                    }
                }
            }
            break;
        default:
            break;
        }
    
        if ( !_reg.is_loading_operation() ){
            _stored = false;
            if ( !_reg.is_key_valid() )
                res = ERROR_INVALID_HANDLE;
            else{
                if ( ERROR_SUCCESS == ( res = _reg._edit.set_value( _reg._key, _name.c_str(), 0, type, lpbuf, size ) ) ){
                    _stored = true;
                } 
            }
        }
        else{
            _stored = true;
        }
    }
    return res;
}

LPTSTR reg_value::get_multi(LPTSTR lpszDest, size_t nMax) {

    LPCTSTR strBuf;
    size_t nCur = 0, nLen = 0;
    
    if ( is_stored() && !is_multi_sz()) return &(lpszDest[0] = 0);
   
    for (size_t n=0; n < _multi_sz.size() && nCur < nMax; n++) {
        
        strBuf = _multi_sz[n].c_str(); 
        nLen = _multi_sz[n].length()+1;
        _tcsncpy_s( lpszDest + nCur, nMax-nCur , strBuf, (nLen >= nMax ? (nMax-nCur) : nLen) * sizeof(_TCHAR));
        nCur += nLen;
    }

    /* Add the final null termination */
    *(lpszDest + nCur) = 0;
    
    return lpszDest;
}

size_t reg_value::get_multi_length() {

    size_t nLen , nIndex;
    for (nLen = 0, nIndex = 0; nIndex < _multi_sz.size(); nIndex++)
        nLen += _multi_sz[nIndex].length() + 1;
    return nLen+1;
}

void reg_value::set_multi_at( size_t index, LPCTSTR lpszVal ){

    if ( ( is_multi_sz() || !is_stored() ) && index <= _multi_sz.size() ){
        if ( index == _multi_sz.size() )
            _multi_sz.push_back( lpszVal );
        else
            _multi_sz[index] = lpszVal;
        REGISTRY_SETINTERNAL(+);
        size_t size = get_multi_length(); 
        std::auto_ptr<TCHAR> pBuf( new TCHAR[ size ]) ;
        get_multi( pBuf.get(), size );
        REGISTRY_SETINTERNAL(-);
        set_multi(pBuf.get(), size);
    }
} 

void reg_value::remove_multi_at( size_t index ){
    
    if ( is_multi_sz() && index < _multi_sz.size() ){    
        _multi_sz.erase(_multi_sz.begin()+index);
        REGISTRY_SETINTERNAL(+);
        size_t size = get_multi_length(); 
        std::auto_ptr<TCHAR> pBuf( new TCHAR[ size ]) ;
        get_multi( pBuf.get(), size );
        REGISTRY_SETINTERNAL(-);
        set_multi( pBuf.get(), size );
    }
}

void reg_value::remove_multi ( LPCTSTR lpszVal ){

    for ( int index = (int) _multi_sz.size()-1 ; index >= 0 ; --index ){
        if (!_tcsicmp( _multi_sz[ index ].c_str(), lpszVal ) )
            _multi_sz.erase(_multi_sz.begin()+index);
    }
    REGISTRY_SETINTERNAL(+);
    size_t size = get_multi_length(); 
    std::auto_ptr<TCHAR> pBuf( new TCHAR[ size ]) ;
    get_multi( pBuf.get(), size );
    REGISTRY_SETINTERNAL(-);
    set_multi( pBuf.get(), size );
}

#endif

};//namespace windows

};//namespace macho


#endif
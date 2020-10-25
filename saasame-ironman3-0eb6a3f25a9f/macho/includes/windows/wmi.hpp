// wmi.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_WMI__
#define __MACHO_WINDOWS_WMI__
#include "..\config\config.hpp"
#include "..\common\exception_base.hpp"
#include "..\windows\protected_data.hpp"
#include <memory>
#include <atlbase.h>
#include <atlcom.h>
#include <initguid.h>
#include <comdef.h>
#include <comutil.h>
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

namespace macho{

namespace windows{

#ifndef _WIN32_WINNT_WS08
#error Need to use Windows 2008 SDK. 
#endif

struct  wmi_exception :  public exception_base {};
#define BOOST_THROW_WMI_EXCEPTION( no, message ) BOOST_THROW_EXCEPTION_BASE( wmi_exception, no, message )
#define BOOST_THROW_WMI_EXCEPTION_STRING(message) BOOST_THROW_EXCEPTION_BASE_STRING( wmi_exception, message)        

typedef enum WMI_OBJECT_GETTEXT_ENUM{
    WMI_OBJECT_GETTEXT_MOF = 0,
    WMI_OBJECT_GETTEXT_XML = 1
};

typedef enum WMI_JOBSTATE_ENUM {
 WMI_JOBSTATE_UNKNOWN      = 0,
 WMI_JOBSTATE_NEW          = 2,
 WMI_JOBSTATE_STARTING     = 3,
 WMI_JOBSTATE_RUNNING      = 4,
 WMI_JOBSTATE_SUSPENDED    = 5,
 WMI_JOBSTATE_SHUTTINGDOWN = 6,
 WMI_JOBSTATE_COMPLETED    = 7,
 WMI_JOBSTATE_TERMINATED   = 8,
 WMI_JOBSTATE_KILLED       = 9,
 WMI_JOBSTATE_EXCEPTION    = 10,
 WMI_JOBSTATE_SERVICE      = 11
};

class safe_array {
public:
    safe_array();    
    safe_array(SAFEARRAY* psa);    
     ~safe_array();
    operator SAFEARRAY*() { return _psa; }
    const safe_array& operator =(SAFEARRAY* psa);
    HRESULT create(VARTYPE vt, DWORD nDimensions, DWORD nElements);
    HRESULT destroy();
    HRESULT put_element(LONG index, PVOID pData);
    HRESULT get_element(LONG index, PVOID pData);
    LONG    count(DWORD nDimension);
private:
    void copy(SAFEARRAY* psa);
    SAFEARRAY* _psa;
};

class wmi_object;
typedef std::vector< wmi_object > wmi_object_table;

class wmi_services{
public:
    wmi_services() : _local(true), _impersonation_level(RPC_C_IMP_LEVEL_IMPERSONATE), _authentication_level(RPC_C_AUTHN_LEVEL_DEFAULT){}
    wmi_services( wmi_services& wmi ) : _local(true) {
        copy( wmi );
    }
    const wmi_services &operator =( const wmi_services& wmi ) {
        if ( this != &wmi ) copy( wmi );
        return( *this );
    }
     ~wmi_services(){}
     HRESULT connect(std::wstring name_space, std::wstring server = L".", std::wstring user = L"" , std::wstring password = L"" , DWORD impersonation_level = RPC_C_IMP_LEVEL_IMPERSONATE, DWORD authentication_level = RPC_C_AUTHN_LEVEL_DEFAULT);
     HRESULT connect(std::wstring name_space, std::wstring server, std::wstring domain, std::wstring user, std::wstring password, DWORD impersonation_level = RPC_C_IMP_LEVEL_IMPERSONATE, DWORD authentication_level = RPC_C_AUTHN_LEVEL_DEFAULT);
     HRESULT spawn_instance(std::wstring class_, wmi_object& instace);
     HRESULT get_wmi_object(std::wstring path, wmi_object& object);
     wmi_object get_wmi_object(std::wstring path);
     HRESULT get_wmi_object_with_spawn_instance(std::wstring path, wmi_object& object);
     HRESULT get_input_parameters(std::wstring class_, std::wstring method, wmi_object& inparameters);
     HRESULT exec_method(std::wstring class_, std::wstring method, wmi_object& inparameters, wmi_object& outparameters , DWORD& return_value, std::wstring& error = std::wstring());
     HRESULT async_exec_method(std::wstring class_, std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value );
     HRESULT exec_query( std::wstring query, wmi_object_table& objects);
     HRESULT open_namespace(std::wstring name_space, wmi_services& wmi );
     wmi_object_table exec_query( std::wstring query );
     wmi_object_table query_wmi_objects(std::wstring class_);
     wmi_object query_wmi_object(std::wstring class_);
private:
    void copy(const wmi_services& wmi, bool is_child = false);
    HRESULT                      set_auth(IUnknown *pUnknown);
    ATL::CComPtr<IWbemServices>  _services_ptr;     // IWbemServices pointer
    std::wstring                 _server;           // Server connected to
#ifdef _DEBUG
    std::wstring                 _domain;
    std::wstring                 _user;             // User name used to connect
    std::wstring                 _password;         // Password used to connect
#else
    protected_data               _domain;
    protected_data               _user;             // User name used to connect
    protected_data               _password;         // Password used to connect
#endif
    std::wstring                 _name_space;
    bool                         _local;            // Local or remote user cred
    DWORD					     _impersonation_level;
    DWORD					     _authentication_level;
};

class wmi_element{
public:
    wmi_element( wmi_object& obj, std::wstring name = L"" ): _obj(obj), _existed(false){
        _name = name;
    }
    ~wmi_element(){}
    wmi_element&    operator =( const std::wstring value );
    wmi_element&    operator =( LPCWSTR value ) { return ( *this = std::wstring( value ) );  }
    wmi_element&    operator =( const string_table_w &value );
    wmi_element&    operator =( bool    value )      { return set_value<bool>( value ); }
    wmi_element&    operator =( ULONGLONG value);
    wmi_element&    operator =( LONGLONG  value )    { return set_value<LONGLONG>( value ); }
    wmi_element&    operator =( const wmi_object  &value );
    wmi_element&    operator =( DWORD value );
    wmi_element&    operator =( LONG  value )        { return set_value<LONG>( value ); }
    wmi_element&    operator =( int   value )        { return set_value<int>( value ); }
    wmi_element&    operator =( short  value)        { return set_value<short>(value); }
    wmi_element&    operator =( unsigned short value){ return set_value<unsigned short>(value); }
    wmi_element&    operator =( uint16_table value)   { return set_value<uint16_table>(value); }

    //operator LPCWSTR()          { return (get_value<std::wstring>()).c_str();  } // Can't use this becasue it will return garbage data
    operator INT()              { return operator DWORD(); }
    operator UINT()             { return operator DWORD(); }
    operator LONG()             { return operator DWORD(); }
    operator LONGLONG()         { return operator ULONGLONG();}
    operator std::wstring()     { return get_value<std::wstring>(); }
    //operator std::string()      { return stringutils::convert_unicode_to_ansi(get_value<std::wstring>()); }
    operator short()            { return operator unsigned short(); }
    operator unsigned short()   { return get_value<unsigned short>(); }
    operator uint8_t()          { return get_value<uint8_t>(); }
    operator bool()             { return get_value<bool>(); }
    operator DWORD()            { return get_value<DWORD>(); }
    operator ULONGLONG()        { return get_value<ULONGLONG>(); }
    operator string_table_w()   { return get_value<string_table_w>(); }
    operator wmi_object_table() { return get_value<wmi_object_table>(); }
    operator uint8_table()      { return get_value<uint8_table>(); }
    operator uint16_table()     { return get_value<uint16_table>(); }
    operator uint32_table()     { return get_value<uint32_table>(); }
    //operator wmi_object()       { return get_wmi_object();}
    wmi_object                  get_wmi_object();
    wmi_element& set_name( std::wstring name )            { _name = name; return (*this); }
    std::wstring inline get_name()   const                { return _name; }
    bool         inline exists() const                { return _existed; }
    template<typename T> wmi_element& set_value( T value );
    template<typename T> T get_value();
private:
    wmi_element( const wmi_element &element );
    const wmi_element &operator =( const wmi_element& element  );
    wmi_object&  _obj;
    std::wstring _name;
    bool         _existed;
};

class wmi_object{
public:
    wmi_object( const wmi_object& obj );
    wmi_object( IWbemClassObject* object_ptr = NULL, wmi_services& wmi = wmi_services() );
    ~wmi_object(){}
    void              copy ( const wmi_object& obj ) ;
    const wmi_object& operator =( const wmi_object& obj  ) ;
    const wmi_object& operator =(IWbemClassObject* object_ptr );
    bool    inline    is_valid() const { return ( _obj_ptr != NULL ); }
    wmi_element&      operator[](LPCWSTR name) { return operator[]( std::wstring(name)); }
    wmi_element&      operator[]( const std::wstring name ) ;
    //operator IWbemClassObject*() { return _obj_ptr; }

    HRESULT get_text( std::wstring& text, WMI_OBJECT_GETTEXT_ENUM option = WMI_OBJECT_GETTEXT_XML );
    HRESULT clone( wmi_object& clone );
    
    HRESULT get_parameter( const std::wstring parameter, DWORD& value);
    HRESULT get_parameter( const std::wstring parameter, bool& value);
    HRESULT get_parameter( const std::wstring parameter, ULONGLONG& value);
    HRESULT get_parameter( const std::wstring parameter, std::wstring& value);
    HRESULT get_parameter( const std::wstring parameter, string_table_w& value);
    HRESULT get_parameter( const std::wstring parameter, uint8_table& value);
    HRESULT get_parameter( const std::wstring parameter, uint16_table& value);
    HRESULT get_parameter( const std::wstring parameter, uint32_table& value);

    HRESULT get_parameter( const std::wstring parameter, wmi_object& value);
    HRESULT get_parameter( const std::wstring parameter, wmi_object_table& value);
    HRESULT get_parameter( const std::wstring parameter, uint8_t& value);
    HRESULT get_parameter( const std::wstring parameter, unsigned short& value);
    HRESULT get_parameter( const std::wstring &parameter, _variant_t &vt, CIMTYPE &type );

    HRESULT set_parameter( const std::wstring &parameter, _variant_t &vt, CIMTYPE type = CIM_EMPTY );    
    template<typename T> HRESULT set_parameter( const std::wstring parameter, T value);
    HRESULT set_parameter( const std::wstring parameter, uint16_t value);
    HRESULT set_parameter( const std::wstring parameter, uint32_t value);
    HRESULT set_parameter( const std::wstring parameter, ULONGLONG value);
    HRESULT set_parameter( const std::wstring parameter, wchar_t value);
    HRESULT set_parameter( const std::wstring parameter, const std::wstring value);
    HRESULT set_parameter( const std::wstring parameter, const string_table_w& value);
    HRESULT set_parameter(const std::wstring parameter, std::vector<uint16_t>& value){
        const std::vector<uint16_t> v = value;
        return set_parameter(parameter, v);
    }
    HRESULT set_parameter( const std::wstring parameter, const std::vector<uint16_t>& value);
    HRESULT set_parameter( const std::wstring parameter, const wmi_object& value);
    HRESULT set_parameter( const std::wstring parameter, bool value);
    HRESULT          exec_method( std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value, std::wstring& error = std::wstring() );
    HRESULT          exec_method( std::wstring method, wmi_object& inparameters, bool& return_value );
    HRESULT          exec_method( std::wstring method, wmi_object& inparameters, wmi_object& outparameters, bool& return_value );
    wmi_object       exec_method( std::wstring method, wmi_object& inparameters = wmi_object() );
    HRESULT          async_exec_method( std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value );
    HRESULT          get_input_parameters( std::wstring method, wmi_object& inparameters);
    wmi_object       get_input_parameters( std::wstring method );
    wmi_object_table get_relateds(const std::wstring result_class, const std::wstring assoc_class);
    wmi_object       get_related(const std::wstring result_class, const std::wstring assoc_class);
private:
    friend HRESULT wmi_services::spawn_instance(std::wstring class_, wmi_object& instace);
    friend HRESULT wmi_services::get_input_parameters(std::wstring class_, std::wstring method, wmi_object& inparameters);
    friend HRESULT wmi_services::get_wmi_object(std::wstring path, wmi_object& object);
    friend HRESULT wmi_services::get_wmi_object_with_spawn_instance(std::wstring path, wmi_object& object);
    friend HRESULT wmi_services::exec_method(std::wstring class_, std::wstring method, wmi_object& inparameters, wmi_object& outparameters , DWORD& return_value , std::wstring& error );
    friend HRESULT wmi_services::async_exec_method(std::wstring class_, std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value );
    const wmi_object &operator = ( const wmi_services& wmi ){
        _wmi = wmi;
        return( *this );
    }

    ATL::CComPtr<IWbemClassObject>  _obj_ptr;
    std::wstring                    _path;
    std::wstring                    _relpath;
    std::wstring                    _class;
    DWORD                           _genus;
    DWORD                           _property_count;
    wmi_services                    _wmi;
    wmi_element                     _element;
};

class wmi_job {
public:
    wmi_job( wmi_services& wmi, wmi_object& object, std::wstring name = L"Job" );
     ~wmi_job();

    HRESULT update();
    HRESULT get_state(DWORD& state);
    HRESULT get_error_code(DWORD& code);
    HRESULT get_error_description(std::wstring& desc);
  
    bool    is_running();
    HRESULT wait_until_job_completes(DWORD& code, std::wstring& desc);

private:
    wmi_services& _wmi;
    wmi_object&   _object;
    wmi_object    _job;
    std::wstring  _name;
};

template<typename T> T wmi_element::get_value(){
    T value;
    HRESULT hr = _obj.get_parameter( _name, value );
    _existed = ( FAILED(hr) ) ? false : true;
    return value;
}

template<typename T> wmi_element& wmi_element::set_value( T value ){
    HRESULT hr = _obj.set_parameter( _name, value );
    if ( FAILED( hr) ){
        _existed = false;
#if _UNICODE  
        stdstring msg = boost::str( boost::wformat(L"Failed to set wmi_object parameter (%s).") % _name );
#else
        stdstring msg = boost::str( boost::format("Failed to set wmi_object parameter (%s).") % stringutils::convert_unicode_to_ansi( _name ) );
#endif
        BOOST_THROW_WMI_EXCEPTION( hr, msg );
    }
    return *this;
}

template<typename T> HRESULT wmi_object::set_parameter( const std::wstring parameter, T value){
    _variant_t      vt = value;
    return set_parameter( parameter, vt );
}

namespace wmi{

class win32_operating_system{
public:
    win32_operating_system(wmi_object &obj) :_obj(obj), _major(0), _minor(0), _build(0){
        std::wstring version = _obj[L"Version"];
        if (version.length())
            swscanf_s(version.c_str(), L"%d.%d.%d", &_major, &_minor, &_build);
    }
    std::wstring version() { return _obj[L"Version"]; }
    std::wstring caption() { return _obj[L"Caption"]; }
    std::wstring cs_name() { return _obj[L"CSName"]; }
    std::wstring csd_version() { return _obj[L"CSDVersion"]; }
    std::wstring local_date_time() { return _obj[L"LocalDateTime"]; }
    DWORD service_pack_major_version() { return _obj[L"ServicePackMajorVersion"]; }
    DWORD service_pack_minor_version() { return _obj[L"ServicePackMinorVersion"]; }
    DWORD major() const { return _major; }
    DWORD minor() const { return _minor; }
    DWORD build() const { return _build; }
    bool  is_win2012_or_greater() { return (_major >= 6 && _minor >= 2) || (_major >6 ); }
    static win32_operating_system get(wmi_services& wmi_cimv2){
        return win32_operating_system(wmi_cimv2.query_wmi_object(L"Win32_OperatingSystem"));
    }
    static win32_operating_system get(std::wstring host = L"", std::wstring user = L"", std::wstring password = L""){
        HRESULT hr = S_OK;
        FUN_TRACE_HRESULT(hr);
        wmi_services wmi_cimv2;
        if (!host.length())
            hr = wmi_cimv2.connect(L"cimv2");
        else {
            hr = wmi_cimv2.connect(L"cimv2", host, user, password);
        }
        return win32_operating_system(wmi_cimv2.query_wmi_object(L"Win32_OperatingSystem"));
    }
    virtual ~win32_operating_system(){}
private:
    wmi_object   _obj;
    DWORD        _major;
    DWORD        _minor;
    DWORD        _build;
};

};

#ifndef MACHO_HEADER_ONLY

#include "..\common\stringutils.hpp"
#include "..\common\tracelog.hpp"
//#include "com_init.hpp"
/*************************************************
safe_array
**************************************************/

safe_array::safe_array(){
    _psa = NULL;
}

void safe_array::copy(SAFEARRAY* psa){
    HRESULT hr = SafeArrayCopy(psa, &_psa);
}
safe_array::safe_array(SAFEARRAY* psa){
    copy(psa);
}

safe_array::~safe_array(){
    destroy();
}

const safe_array& safe_array::operator =(SAFEARRAY* psa)
{
    if(_psa != psa){
        destroy();
        copy(psa);
    }
    return *this;
}

HRESULT safe_array::create(VARTYPE vt, DWORD nDimensions, DWORD nElements){

    HRESULT         hr      = S_OK;
    SAFEARRAYBOUND* psab    = NULL;
    DWORD           i       = 0;
    hr = destroy();
    psab = (SAFEARRAYBOUND*)calloc(nDimensions, sizeof(SAFEARRAYBOUND));
    if(!psab){
        hr = E_OUTOFMEMORY;
        goto final;
    }

    for(i = 0; i < nDimensions; i++){
        psab[i].cElements   = nElements;
        psab[i].lLbound     = 0;
    }

    _psa = SafeArrayCreate(vt, nDimensions, psab);
    if(!_psa){
        hr = E_OUTOFMEMORY;
        goto final;
    }

final:
    if (psab)
        free(psab);
    return hr;
}

HRESULT safe_array::destroy(){
    HRESULT hr = S_OK;
    if(_psa){
        hr = SafeArrayDestroy(_psa);
        _psa = NULL;
    }
    return hr;
}

HRESULT safe_array::put_element(LONG index, PVOID pData){
    HRESULT hr = S_OK;
    if(!_psa){
        hr = E_POINTER;
        goto final;
    }
    hr = SafeArrayPutElement(_psa, &index, pData);
final:
    return hr;
}

HRESULT safe_array::get_element(LONG index, PVOID pData){
    HRESULT hr = S_OK;
    if(!_psa){
        hr = E_POINTER;
        goto final;
    }
    hr = SafeArrayGetElement(_psa, &index, pData);
final:
    return hr;
}

LONG safe_array::count(DWORD nDimension){
    LONG count = 0;
    if(_psa && nDimension < _psa->cDims) {
        count = _psa->rgsabound[nDimension].cElements;
    }
    return count;
}

/**********************************************************
wmi_services
**********************************************************/

void wmi_services::copy(const wmi_services& wmi, bool is_child ) {
    if (!is_child){
        _services_ptr = wmi._services_ptr;
        _name_space = wmi._name_space;
    }
    _server = wmi._server;
    _domain = wmi._domain;
    _user = wmi._user;
    _password = wmi._password;
    _local = wmi._local;
    _impersonation_level = wmi._impersonation_level;
    _authentication_level = wmi._authentication_level;
}

HRESULT wmi_services::connect(std::wstring name_space, std::wstring server, std::wstring user, std::wstring password, DWORD impersonation_level, DWORD authentication_level){
    std::wstring domain, account;
    string_array arr = stringutils::tokenize2(user, L"\\", 2, false);
    if (arr.size()){
        if (arr.size() == 1)
            account = arr[0];
        else{
            domain = arr[0];
            account = arr[1];
        }
    }
    return connect(name_space, server, domain, account, password, impersonation_level, authentication_level);
}

HRESULT wmi_services::connect(std::wstring name_space, std::wstring server, std::wstring domain, std::wstring user, std::wstring password, DWORD impersonation_level, DWORD authentication_level ){

    HRESULT                 hr              = S_OK;
    CComPtr<IWbemLocator>   pWbemLocator    = NULL;
    std::wstring            sTarget         = L"";
    std::wstring            sUser           = L"";
    bool                    bConnected      = false;

    FUN_TRACE_HRESULT(hr);
    _services_ptr = NULL;
    _server       = server;
    _name_space   = name_space;
    _domain       = domain;
    _user         = user;
    _password     = password;

    hr = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID *)&pWbemLocator );
    if(FAILED(hr)){
        BOOST_THROW_WMI_EXCEPTION( hr, _T("CoCreateInstance failed.") );
    }
    if (name_space.length())
        sTarget = boost::str( boost::wformat( L"\\\\%s\\root\\%s" ) % server % name_space ) ;
    else
        sTarget = boost::str(boost::wformat(L"\\\\%s\\root") % server );

    if (!user.empty()){
        if (domain.empty()){
            sUser = boost::str(boost::wformat(L"%s\\%s") % server % user);
        }
        else{
            sUser = boost::str(boost::wformat(L"%s\\%s") % domain % user);;
        }
    }

    if( sUser.length() && password.length() ){
        hr = pWbemLocator->ConnectServer(
            _bstr_t(sTarget.c_str()),
            _bstr_t(sUser.c_str()),
            _bstr_t(password.c_str()),
            _bstr_t(L"MS_409"),
            NULL,
            _bstr_t(L""),
            0,
            &_services_ptr );
        if(FAILED(hr)){            
#if _UNICODE                
            LOG(LOG_LEVEL_ERROR, _T("pWbemLocator->ConnectServer (%s) with supplied credentials failed (0x%08X)."), sTarget.c_str(), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"pWbemLocator->ConnectServer (%s) with supplied credentials failed.") % server));
#else
            LOG(LOG_LEVEL_ERROR,_T("pWbemLocator->ConnectServer (%s) with supplied credentials failed (0x%08X)."), stringutils::convert_unicode_to_ansi(sTarget).c_str(), hr );
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format( L"pWbemLocator->ConnectServer (%s) with supplied credentials failed.")%stringutils::convert_unicode_to_ansi(server)));
#endif                   
        }
        else{
            bConnected  = true;
            _local    = false;
        }
    }

    if(!bConnected){
        hr = pWbemLocator->ConnectServer(
            _bstr_t(sTarget.c_str()),
            NULL,
            NULL,
            _bstr_t(L"MS_409"),
            NULL,
            _bstr_t(L""),
            0,
            &_services_ptr );
        if(FAILED(hr)){            
#if _UNICODE                
            LOG(LOG_LEVEL_ERROR, _T("pWbemLocator->ConnectServer (%s) with local credentials failed (0x%08X)."), sTarget.c_str(), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"pWbemLocator->ConnectServer (%s) with local credentials failed.") % server));
#else
            LOG(LOG_LEVEL_ERROR,_T("pWbemLocator->ConnectServer (%s) with local credentials failed (0x%08X)."), stringutils::convert_unicode_to_ansi(sTarget).c_str(), hr );
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format( L"pWbemLocator->ConnectServer (%s) with local credentials failed.")%stringutils::convert_unicode_to_ansi(server)));
#endif                  
        }
        else{
            _local = true;
        }
    }
    
    _impersonation_level = impersonation_level;
    _authentication_level = authentication_level;

    hr = set_auth(_services_ptr);
    if(FAILED(hr)) {
        LOG(LOG_LEVEL_ERROR,_T("set_auth failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("set_auth failed."));
    }
  
#if _UNICODE                
    LOG(LOG_LEVEL_TRACE,_T("Connected to (%s)."), sTarget.c_str());
#else
    LOG(LOG_LEVEL_TRACE,_T("Connected to (%s)."), stringutils::convert_unicode_to_ansi(sTarget).c_str());
#endif   
    return hr;
}

HRESULT wmi_services::open_namespace(std::wstring name_space, wmi_services& wmi)
{

    HRESULT hr = S_OK;

    FUN_TRACE_HRESULT(hr);

    if (!_services_ptr){
        hr = E_POINTER;
        LOG(LOG_LEVEL_ERROR, _T("_services_ptr is NULL."));
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    wmi._services_ptr = NULL;
    wmi.copy(*this, true);
    if (!_name_space.empty())
        wmi._name_space = boost::str(boost::wformat(L"%s\\%s") % _name_space %name_space);
    else
        wmi._name_space = name_space;

    hr = _services_ptr->OpenNamespace(
        _bstr_t(name_space.c_str()),
        WBEM_RETURN_WHEN_COMPLETE,
        NULL,
        &wmi._services_ptr,
        NULL);
    if (FAILED(hr)){
#if _UNICODE  
        LOG(LOG_LEVEL_ERROR, L"_services_ptr->OpenNamespace (%s) failed (0x%08X).", name_space.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->OpenNamespace (%s) failed.") %name_space) );
#else
        LOG(LOG_LEVEL_ERROR, L"_services_ptr->OpenNamespace (%s) failed (0x%08X).", convert_unicode_to_ansi(name_space).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->OpenNamespace (%s) failed.") %stringutils::convert_unicode_to_ansi(name_space)));
#endif
    }

    hr = wmi.set_auth(wmi._services_ptr);
    if (FAILED(hr)){
        LOG(LOG_LEVEL_ERROR, _T("set_auth failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("set_auth failed."));
    }

#if _UNICODE  
    LOG(LOG_LEVEL_TRACE, _T("opened namespace (%s)."), wmi._name_space.c_str());
#else
    LOG(LOG_LEVEL_TRACE, _T("opened namespace (%s)."), stringutils::convert_unicode_to_ansi(wmi._name_space).c_str());
#endif

    return hr;
}

HRESULT wmi_services::set_auth(IUnknown *pUnknown){

    HRESULT hr                  = S_OK;
    SEC_WINNT_AUTH_IDENTITY_W   auth;

    FUN_TRACE_HRESULT(hr);

    if( _local ){
        hr = CoSetProxyBlanket(
            pUnknown,
            RPC_C_AUTHN_DEFAULT,
            RPC_C_AUTHZ_NONE,
            COLE_DEFAULT_PRINCIPAL,
            _authentication_level,
            _impersonation_level,
            NULL,
            EOAC_NONE );
    }
    else{
        std::wstring domain = _domain;
        std::wstring user = _user;
        std::wstring password = _password;
        auth.User = (unsigned short *)user.c_str();
        auth.UserLength = (unsigned long)user.length();
        auth.Password = (unsigned short *)password.c_str();
        auth.PasswordLength = (unsigned long)password.length();
        auth.Domain = (unsigned short *)domain.c_str();
        auth.DomainLength = (unsigned long)domain.length();
        auth.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;

        hr = CoSetProxyBlanket(
            pUnknown,
            RPC_C_AUTHN_DEFAULT,
            RPC_C_AUTHZ_NONE,
            COLE_DEFAULT_PRINCIPAL,
            _authentication_level,
            _impersonation_level,
            &auth,
            EOAC_NONE );
    }

    if(FAILED(hr)){
        LOG( LOG_LEVEL_ERROR, _T("CoSetProxyBlanket failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("CoSetProxyBlanket failed."));
    }
    return hr;
}

HRESULT wmi_services::spawn_instance(std::wstring class_, wmi_object& instace){
    
    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pClass      = NULL;
    CComPtr<IWbemClassObject>   pInstance   = NULL;

    FUN_TRACE_HRESULT(hr);

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->GetObject(
        _bstr_t(class_.c_str()),
        0,
        NULL,
        &pClass,
        NULL );
    if(FAILED(hr)){
#if _UNICODE        
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->GetObject (%s) failed (0x%08X)."), class_.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->GetObject (%s) failed.") %class_));
#else
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->GetObject (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->GetObject (%s) failed.") %stringutils::convert_unicode_to_ansi(class_)));
#endif
    }

    hr = pClass->SpawnInstance(0, &pInstance);
    if(FAILED(hr)){
#if _UNICODE        
        LOG( LOG_LEVEL_ERROR, _T("pClass->SpawnInstance (%s) failed (0x%08X)."), class_.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"pClass->SpawnInstance (%s) failed.") %class_));
#else
        LOG( LOG_LEVEL_ERROR, _T("pClass->SpawnInstance (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"pClass->SpawnInstance (%s) failed.") %stringutils::convert_unicode_to_ansi(class_)));
#endif
    }

    instace = pInstance;
    instace = *this;

    return hr;
}

HRESULT wmi_services::get_wmi_object(std::wstring path, wmi_object& object){
    
    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pObject     = NULL;

    FUN_TRACE_HRESULT(hr);

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->GetObject(
        _bstr_t(path.c_str()),
        0,
        NULL,
        &pObject,
        NULL );
    if(FAILED(hr)){
#if _UNICODE        
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->GetObject (%s) failed (0x%08X)."), path.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->GetObject (%s) failed.") %path));
#else
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->GetObject (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(path).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->GetObject (%s) failed.") %stringutils::convert_unicode_to_ansi(path)));
#endif
    }

    object = pObject;
    object = *this;
    return hr;
}

wmi_object wmi_services::get_wmi_object(std::wstring path){
    wmi_object object;
    HRESULT hr;
    FUN_TRACE_HRESULT(hr);
    hr = get_wmi_object(path, object);
    return object;
}

HRESULT wmi_services::get_wmi_object_with_spawn_instance(std::wstring path, wmi_object& object){
    
    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pObject     = NULL;
    CComPtr<IWbemClassObject>   pSWObject     = NULL;
    
    FUN_TRACE_HRESULT(hr);

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->GetObject(
        _bstr_t(path.c_str()),
        0,
        NULL,
        &pObject,
        NULL );
    if(FAILED(hr)){
#if _UNICODE        
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->GetObject (%s) failed (0x%08X)."), path.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->GetObject (%s) failed.") %path));
#else
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->GetObject (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(path).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->GetObject (%s) failed.") %stringutils::convert_unicode_to_ansi(path)));
#endif
    }

    if ( NULL != pObject ) {
        hr = pObject->SpawnInstance(0, &pSWObject);
        if (FAILED(hr)){
#if _UNICODE        
            LOG(LOG_LEVEL_ERROR, _T("pObject->SpawnInstance (%s) failed (0x%08X)."), path.c_str(), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"pObject->SpawnInstance (%s) failed.") %path));
#else
            LOG( LOG_LEVEL_ERROR, _T("pObject->SpawnInstance (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(path).c_str(), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"pObject->SpawnInstance (%s) failed.") %stringutils::convert_unicode_to_ansi(path)));
#endif
        }
        object = pSWObject;
        object = *this;
    }
    return hr;
}

HRESULT wmi_services::get_input_parameters(std::wstring class_, std::wstring method, wmi_object& inparameters){
    
    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pClass      = NULL;
    CComPtr<IWbemClassObject>   pInClass    = NULL;
    CComPtr<IWbemClassObject>   pIn         = NULL;

    FUN_TRACE_HRESULT(hr);

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->GetObject(
        _bstr_t(class_.c_str()),
        0,
        NULL,
        &pClass,
        NULL );
    if(FAILED(hr)){
#if _UNICODE                
        LOG( LOG_LEVEL_ERROR, _T("pServices->GetObject (%s) failed (0x%08X)."), class_.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->GetObject (%s) failed.") %class_));
#else
        LOG( LOG_LEVEL_ERROR, _T("pServices->GetObject (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->GetObject (%s) failed.") %stringutils::convert_unicode_to_ansi(class_)));
#endif       
    }

    hr = pClass->GetMethod(
        _bstr_t(method.c_str()),
        0,
        &pInClass,
        NULL );
    if(FAILED(hr) ){
#if _UNICODE                
        LOG( LOG_LEVEL_ERROR, _T("pClass->GetMethod (%s) (%s) failed (0x%08X)."), class_.c_str(), method.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"pClass->GetMethod (%s) (%s) failed.") %class_ %method));
#else
        LOG( LOG_LEVEL_ERROR, _T("pClass->GetMethod (%s) (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"pClass->GetMethod (%s) (%s) failed.") %stringutils::convert_unicode_to_ansi(class_) % stringutils::convert_unicode_to_ansi(method)));
#endif  
    }

    if ( NULL != pInClass ){
        hr = pInClass->SpawnInstance(0, &pIn);
        if(FAILED(hr)){
#if _UNICODE                
            LOG( LOG_LEVEL_ERROR, _T("pInClass->SpawnInstance (%s) (%s) failed (0x%08X)."), class_.c_str(), method.c_str(), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"pClass->SpawnInstance (%s) (%s) failed.") %class_ %method));
#else
            LOG( LOG_LEVEL_ERROR, _T("pInClass->SpawnInstance (%s) (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"pClass->SpawnInstance (%s) (%s) failed.") %stringutils::convert_unicode_to_ansi(class_) % stringutils::convert_unicode_to_ansi(method)));
#endif
        }
        inparameters = pIn;
        inparameters = *this;
    }
    return hr;
}

HRESULT wmi_services::exec_method(std::wstring class_, std::wstring method, wmi_object& inparameters, wmi_object& outparameters , DWORD& return_value , std::wstring& error ){
    
    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pOut        = NULL;
    _variant_t                  vt;
    std::wstring                error_desc;

    FUN_TRACE_HRESULT(hr);

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->ExecMethod(
        _bstr_t( class_.c_str() ),
        _bstr_t( method.c_str() ),
        WBEM_RETURN_WHEN_COMPLETE,
        NULL,
        inparameters._obj_ptr,
        &pOut,
        NULL );
    if(FAILED(hr)){
#if _UNICODE  
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->ExecMethod (%s) (%s) failed (0x%08X)."),  class_.c_str(), method.c_str(), hr );
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->ExecMethod (%s) (%s) failed.") %class_ %method));
#else
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->ExecMethod (%s) (%s) failed (0x%08X)."),  stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr );
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->ExecMethod (%s) (%s) failed.") %stringutils::convert_unicode_to_ansi(class_) % stringutils::convert_unicode_to_ansi(method)));
#endif
    }

    if(!pOut){
#if _UNICODE  
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) Did not get return value object."),  class_.c_str(), method.c_str() );
#else
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) Did not get return value object."),  stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str() );
#endif
        return_value = 0;
    }
    else{
        hr = pOut->Get(L"ReturnValue", 0, &vt, NULL, NULL);
        if(FAILED(hr)){
#if _UNICODE  
            LOG(LOG_LEVEL_ERROR, _T("(%s) (%s) pOut->Get(\"ReturnValue\") (0x%08X)."), class_.c_str(), method.c_str(), hr);
#else
            LOG(LOG_LEVEL_ERROR, _T("(%s) (%s) pOut->Get(\"ReturnValue\") (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr);
#endif
            if ( hr == 0x80041002 ){ // BEM_E_NOT_FOUND
                hr = S_OK;
            }
            else{
#if _UNICODE  
                BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"(%s) (%s) pOut->Get(\"ReturnValue\") (%s) (%s) failed.") % class_ %method));
#else
                BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"(%s) (%s) pOut->Get(\"ReturnValue\") (%s) (%s) failed.") % stringutils::convert_unicode_to_ansi(class_) % stringutils::convert_unicode_to_ansi(method)));
#endif
            }
        }
        return_value = vt;

        if(4096 == return_value){
            wmi_object out( pOut, *this );            
            wmi_job    job( *this, out );
            hr = job.wait_until_job_completes(return_value, error_desc );
            if(FAILED(hr)){
#if _UNICODE  
                LOG( LOG_LEVEL_ERROR, _T("Job for (%s) (%s) failed (0x%08X)."),  class_.c_str(), method.c_str(), hr );
#else
                LOG( LOG_LEVEL_ERROR, _T("Job for (%s) (%s) failed (0x%08X)."),  stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr );
#endif
            }
        }
    }

    if(!FAILED(hr)) {
#if _UNICODE  
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) returned (%d) (%s)."),  class_.c_str(), method.c_str(), return_value, error_desc.c_str() );
#else
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) returned (%d) (%s)."),  stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), return_value,stringutils::convert_unicode_to_ansi(error_desc).c_str() );
#endif
    }
    error = error_desc;
    outparameters = pOut;
    outparameters = *this;
    return hr;
}

HRESULT wmi_services::async_exec_method(std::wstring class_, std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value ){

    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pOut        = NULL;
    _variant_t                  vt;

    FUN_TRACE_HRESULT(hr);

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->ExecMethod(
        _bstr_t( class_.c_str() ),
        _bstr_t( method.c_str() ),
        WBEM_RETURN_WHEN_COMPLETE,
        NULL,
        inparameters._obj_ptr,
        &pOut,
        NULL );
    if(FAILED(hr)){
#if _UNICODE  
        LOG(LOG_LEVEL_ERROR, _T("_services_ptr->ExecMethod (%s) (%s) failed (0x%08X)."), class_.c_str(), method.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->ExecMethod (%s) (%s) failed.") %class_ %method));
#else
        LOG(LOG_LEVEL_ERROR, _T("_services_ptr->ExecMethod (%s) (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->ExecMethod (%s) (%s) failed.") %stringutils::convert_unicode_to_ansi(class_) %stringutils::convert_unicode_to_ansi(method)));
#endif
    }

    if(!pOut){
#if _UNICODE  
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) Did not get return value object."),  class_.c_str(), method.c_str() );
#else
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) Did not get return value object."),  stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str() );
#endif
        return_value = 0;
    }
    else{
        hr = pOut->Get(L"ReturnValue", 0, &vt, NULL, NULL);
        if(FAILED(hr)){
#if _UNICODE  
            LOG(LOG_LEVEL_ERROR, _T("(%s) (%s) pOut->Get(\"ReturnValue\") (0x%08X)."), class_.c_str(), method.c_str(), hr);
#else
            LOG(LOG_LEVEL_ERROR, _T("(%s) (%s) pOut->Get(\"ReturnValue\") (0x%08X)."), stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), hr);
#endif
            if (hr == 0x80041002){ // BEM_E_NOT_FOUND
                hr = S_OK;
            }
            else{
#if _UNICODE  
                BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"(%s) (%s) pOut->Get(\"ReturnValue\") (%s) (%s) failed.") %class_ %method));
#else
                BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"(%s) (%s) pOut->Get(\"ReturnValue\") (%s) (%s) failed.") %stringutils::convert_unicode_to_ansi(class_) % stringutils::convert_unicode_to_ansi(method)));
#endif
            }
        }
        return_value = vt;

        outparameters     = pOut;
        outparameters     = *this;
    }
#if _UNICODE  
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) returned (%d)."),  class_.c_str(), method.c_str(), return_value );
#else
        LOG( LOG_LEVEL_INFO, _T("(%s) (%s) returned (%d)."),  stringutils::convert_unicode_to_ansi(class_).c_str(), stringutils::convert_unicode_to_ansi(method).c_str(), return_value );
#endif
    return hr;
}

HRESULT wmi_services::exec_query( std::wstring query, wmi_object_table& objects){
    
    HRESULT                         hr          = S_OK;
    CComPtr<IEnumWbemClassObject>   pEnum       = NULL;

    FUN_TRACE_HRESULT(hr);

    objects.clear();

    if(!_services_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr is NULL.") );
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_services_ptr is NULL."));
    }

    hr = _services_ptr->ExecQuery(
        _bstr_t(L"WQL"),
        _bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnum );
    if(FAILED(hr)){
#if _UNICODE  
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->ExecQuery (%s) failed (0x%08X)."),  query.c_str(), hr );
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_services_ptr->ExecQuery (%s) failed.") %query ));
#else
        LOG( LOG_LEVEL_ERROR, _T("_services_ptr->ExecQuery (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(query).c_str(), hr );
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format(L"_services_ptr->ExecQuery (%s) failed.") %stringutils::convert_unicode_to_ansi(query)));
#endif
    }

    hr = set_auth(pEnum);
    if(FAILED(hr)){
        LOG( LOG_LEVEL_ERROR, _T("set_auth failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("set_auth failed."));
    }

    while(TRUE){
        CComPtr<IWbemClassObject>   pObject     = NULL;
        ULONG                       nReturn     = 0;
        std::wstring                sText       = L"";

        hr = pEnum->Next(WBEM_INFINITE, 1, &pObject, &nReturn);
        if(FAILED(hr)){
            LOG( LOG_LEVEL_TRACE, _T("pEnum->Next failed (0x%08X)."), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, _T("pEnum->Next failed."));
        }

        if(0 == nReturn){
            hr = S_OK;
            break;
        } 
        wmi_object obj( pObject, *this );   
        objects.push_back(obj);
    }

#if _UNICODE  
    LOG( LOG_LEVEL_TRACE,_T("Success (%s), (%d) objects returned."), query.c_str(), objects.size());
#else
    LOG( LOG_LEVEL_TRACE,_T("Success (%s), (%d) objects returned."), stringutils::convert_unicode_to_ansi(query).c_str(), objects.size());
#endif

    return hr;        
}

wmi_object_table wmi_services::exec_query( std::wstring query ){
     wmi_object_table objects;
     HRESULT hr = S_OK;
     FUN_TRACE_HRESULT(hr);
     hr = exec_query( query, objects );
     return objects;
}

wmi_object_table wmi_services::query_wmi_objects( std::wstring class_ ){
    wmi_object_table objects;
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    std::wstring query = boost::str(boost::wformat(L"select * from %s")%class_ );
    hr = exec_query(query, objects);
    return objects;
}

wmi_object wmi_services::query_wmi_object( std::wstring class_ ){
    wmi_object_table objects;
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    std::wstring query = boost::str(boost::wformat(L"select * from %s") % class_);
    hr = exec_query(query, objects);
    if (0 == objects.size())
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"Can't find any object for the wmi class '%s'.") % class_));
    return objects[0];
}

/**********************************************************
    wmi_object
***********************************************************/

wmi_object::wmi_object( const wmi_object& obj )
: _genus(0), 
_element(*this),
_property_count(0){
    copy(obj);
}

wmi_object::wmi_object( IWbemClassObject* object_ptr, wmi_services& wmi )
: _wmi(wmi), 
_genus(0), 
_element(*this),
_property_count(0) {
    _obj_ptr = object_ptr;
}

void wmi_object::copy ( const wmi_object& obj ) { 
    _obj_ptr        = obj._obj_ptr;
    _path           = obj._path;
    _relpath        = obj._relpath;
    _class          = obj._class;
    _genus          = obj._genus;
    _wmi            = obj._wmi;
    _property_count = obj._property_count;
}

const wmi_object& wmi_object::operator =( const wmi_object& obj  ) {
    if ( this != &obj ) copy( obj );
    return( *this );
}

const wmi_object& wmi_object::operator =(IWbemClassObject* object_ptr){
    _obj_ptr = NULL;
    _obj_ptr = object_ptr;
    return *this;
}

wmi_element&   wmi_object::operator[]( const std::wstring name ) {
    return _element.set_name(name);
}

HRESULT wmi_object::get_text( std::wstring& text, WMI_OBJECT_GETTEXT_ENUM option ){
    
    HRESULT                     hr          = S_OK;
    CComPtr<IWbemObjectTextSrc> pTextSrc    = NULL;
    _bstr_t                     bstrText    = L"";

    FUN_TRACE_HRESULT(hr);

    if(!_obj_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr is NULL."));
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_obj_ptr is NULL."));
    }

    switch( option ){
    case WMI_OBJECT_GETTEXT_MOF:
        hr = _obj_ptr->GetObjectText( 0, bstrText.GetAddress());
        if(FAILED(hr)){
            LOG( LOG_LEVEL_ERROR ,_T("_obj_ptr->GetObjectText failed (0x%08X)."), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, _T("_obj_ptr->GetObjectText failed."));
        }
        break;
    case WMI_OBJECT_GETTEXT_XML:
        hr = CoCreateInstance(
            CLSID_WbemObjectTextSrc, 
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWbemObjectTextSrc, 
            (void**)&pTextSrc );
        if(FAILED(hr)){
            LOG( LOG_LEVEL_ERROR, _T("CoCreateInstance for CLSID_WbemObjectTextSrc failed (0x%08X)."), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, _T("CoCreateInstance for CLSID_WbemObjectTextSrc failed."));
        }

        hr = pTextSrc->GetText(
            0,
            _obj_ptr,
            WMI_OBJ_TEXT_CIM_DTD_2_0,
            NULL,
            bstrText.GetAddress() );
        if(FAILED(hr)){
            LOG( LOG_LEVEL_ERROR,_T("pTextSrc->GetText failed (0x%08X)."), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, _T("pTextSrc->GetText failed."));
        }
        break;
    default:
        LOG( LOG_LEVEL_ERROR, _T("Invalid option (%d)."), option);
        break;
    }

    text = (LPCWSTR)bstrText;
    return hr;
}

HRESULT wmi_object::clone( wmi_object& clone ){

    HRESULT                     hr          = S_OK;
    CComPtr<IWbemClassObject>   pClone      = NULL;
    FUN_TRACE_HRESULT(hr);

    if(!_obj_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr is NULL."));
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_obj_ptr is NULL."));
    }

    hr = _obj_ptr->Clone(&pClone);
    if(FAILED(hr)){
        LOG( LOG_LEVEL_ERROR, _T("m_pObject->Clone failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("m_pObject->Clone failed."));
    }
    clone._wmi = this->_wmi;
    clone = pClone;

    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring &parameter, _variant_t &vt, CIMTYPE &type ){
    
    HRESULT     hr  = S_OK;
    type            = CIM_ILLEGAL;

    FUN_TRACE_HRESULT(hr);

    if(!_obj_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr is NULL."));
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_obj_ptr is NULL."));
    }

    hr = _obj_ptr->Get( parameter.c_str(), 0, &vt, &type, NULL);

    if(FAILED(hr)) {
#if _UNICODE  
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr->Get (%s) failed (0x%08X)."), parameter.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_obj_ptr->Get (%s) failed.") %parameter));
#else
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr->Get (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(parameter).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format("_obj_ptr->Get (%s) failed.") %stringutils::convert_unicode_to_ansi::(parameter)));
#endif
    }

    if(VT_NULL == vt.vt){
        //hr = E_FAIL;
#if _UNICODE  
        LOG( LOG_LEVEL_INFO, _T("_obj_ptr->Get (%s) return VT_NULL."), parameter.c_str() );
        //BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_obj_ptr->Get (%s) return VT_NULL.") %parameter));
#else
        LOG( LOG_LEVEL_INFO, _T("_obj_ptr->Get (%s) return VT_NULL."), stringutils::convert_unicode_to_ansi(parameter).c_str() );
        //BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format("_obj_ptr->Get (%s) return VT_NULL.") % stringutils::convert_unicode_to_ansi::(parameter)));
#endif
    }
    return hr;    
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, DWORD& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value = 0;
    if (SUCCEEDED(hr) && VT_NULL != vt.vt)
        value = (DWORD)vt;
    return hr;
}

HRESULT wmi_object::get_parameter(const std::wstring parameter, unsigned short& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter(parameter, vt, type);
    value = 0;
    if (SUCCEEDED(hr) && VT_NULL != vt.vt)
        value = (unsigned short)vt;
    return hr;
}

HRESULT wmi_object::get_parameter(const std::wstring parameter, uint8_t& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter(parameter, vt, type);
    value = 0;
    if (SUCCEEDED(hr) && VT_NULL != vt.vt)
        value = (uint8_t)vt;
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, bool& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value = false;
    if (SUCCEEDED(hr) && VT_NULL != vt.vt)
        value = (bool)vt;
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, ULONGLONG& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value = 0;
    if (SUCCEEDED(hr) && VT_NULL != vt.vt)
        value = (ULONGLONG)vt;
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, std::wstring& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value.clear();
    if (SUCCEEDED(hr) && VT_NULL != vt.vt)
        value = (_bstr_t)vt;
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, string_table_w& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value.clear();
    if (SUCCEEDED(hr) && VT_NULL != vt.vt){
        safe_array   sa = vt.parray;
        for( int i = 0; i < sa.count(0); i++ ) {
            _bstr_t bstr = L"";
            hr = sa.get_element(i, bstr.GetAddress());
            value.push_back((LPCWSTR)bstr);
        }
    }
    return hr;
}

HRESULT wmi_object::get_parameter(const std::wstring parameter, uint8_table& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter(parameter, vt, type);
    value.clear();
    if (SUCCEEDED(hr) && VT_NULL != vt.vt){
        safe_array   sa = vt.parray;
        for (int i = 0; i < sa.count(0); i++) {
            uint8_t uint8 = 0;
            hr = sa.get_element(i, &uint8);
            value.push_back(uint8);
        }
    }
    return hr;
}

HRESULT wmi_object::get_parameter(const std::wstring parameter, uint16_table& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter(parameter, vt, type);
    value.clear();
    if (SUCCEEDED(hr) && VT_NULL != vt.vt){
        safe_array   sa = vt.parray;
        for (int i = 0; i < sa.count(0); i++) {
            uint32_t uint16 = 0;
            hr = sa.get_element(i, &uint16);
            value.push_back(uint16);
        }
    }
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, uint32_table& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value.clear();
    if (SUCCEEDED(hr) && VT_NULL != vt.vt){
        safe_array   sa = vt.parray;
        for( int i = 0; i < sa.count(0); i++ ) {
            uint32_t uint32 = 0;
            hr = sa.get_element(i, &uint32);
            value.push_back(uint32);
        }
    }
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, wmi_object& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    if (SUCCEEDED(hr) && VT_NULL != vt.vt){
        CComQIPtr<IWbemClassObject> pObject = vt.punkVal;
        value = this->_wmi;
        value = pObject;
    }
    return hr;
}

HRESULT wmi_object::get_parameter( const std::wstring parameter, wmi_object_table& value){
    _variant_t vt;
    CIMTYPE    type;
    HRESULT    hr = get_parameter( parameter, vt, type );
    value.clear();
    if (SUCCEEDED(hr) && VT_NULL != vt.vt){
        safe_array   sa = vt.parray;
        for( int i = 0; i < sa.count(0); i++ ) {
            IUnknown*                   pUnknown    = NULL;
            CComQIPtr<IWbemClassObject> pObject;
            hr = sa.get_element(i, &pUnknown);
            if( !pUnknown ) continue;        
            pObject = pUnknown;
            wmi_object obj( pObject, _wmi );
            value.push_back(obj);
        }
    }
    return hr;
}

/*
In MOF, numbers are digits that describe numerical values.MOF provides a variety of data types that translate into Automation,
and also allows those numbers to be in different formats.The following table lists the numeric values that MOF supports.
https://msdn.microsoft.com/en-us/library/aa392716(v=vs.85).aspx
*/

HRESULT wmi_object::set_parameter( const std::wstring &parameter, _variant_t &vt, CIMTYPE type ){

    HRESULT         hr  = S_OK;       
    
    FUN_TRACE_HRESULT(hr);

    if(!_obj_ptr){
        hr = E_POINTER;
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr is NULL."));
        BOOST_THROW_WMI_EXCEPTION(hr, _T("_obj_ptr is NULL."));
    }

    if (type == CIM_EMPTY){
        _variant_t _vt;
        CIMTYPE _type;
        hr = _obj_ptr->Get(parameter.c_str(), 0, &_vt, &_type, NULL);
        if (!FAILED(hr)){
            type = _type;
        }
    }

    hr = _obj_ptr->Put( parameter.c_str(), 0, &vt, type );
    if(FAILED(hr)){
#if _UNICODE  
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr->Put (%s) failed (0x%08X)."), parameter.c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"_obj_ptr->Put (%s) failed.") %parameter));
#else
        LOG( LOG_LEVEL_ERROR, _T("_obj_ptr->Put (%s) failed (0x%08X)."), stringutils::convert_unicode_to_ansi(parameter).c_str(), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format("_obj_ptr->Put (%s) failed.") % stringutils::convert_unicode_to_ansi::(parameter)));
#endif
    }
    return hr;
}

HRESULT wmi_object::set_parameter( const std::wstring parameter, ULONGLONG value){
    _variant_t      vt = boost::str( boost::wformat( L"%llu") % value).c_str();
    return set_parameter( parameter, vt, CIM_UINT64 );
}

HRESULT wmi_object::set_parameter(const std::wstring parameter, uint16_t value){
    _variant_t      vt;
    vt.vt = VT_I2;
    vt.intVal = value;
    return set_parameter(parameter, vt, CIM_UINT16);
}

HRESULT wmi_object::set_parameter(const std::wstring parameter, uint32_t value){
    _variant_t      vt;
    vt.vt = VT_I4;
    vt.intVal = value;
    return set_parameter(parameter, vt, CIM_UINT32);
}

HRESULT wmi_object::set_parameter(const std::wstring parameter, wchar_t value){
    _variant_t      vt;
    vt.vt = VT_I2;
    vt.intVal = value;
    return set_parameter(parameter, vt, CIM_CHAR16);
}

HRESULT wmi_object::set_parameter( const std::wstring parameter, const std::wstring value){
    _variant_t      vt = value.c_str();
    return set_parameter( parameter, vt );
}

HRESULT wmi_object::set_parameter( const std::wstring parameter, const string_table_w& value){
    _variant_t      vt ;
    safe_array      sa ;
    HRESULT         hr = S_OK;
 
    FUN_TRACE_HRESULT(hr);

    hr = sa.create(VT_BSTR, 1, (DWORD)value.size());
    if(FAILED(hr)){
        LOG(LOG_LEVEL_ERROR,_T("sa.create failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("sa.create failed."));
    }
    for(int i = 0; i < (int)value.size(); i++) {
        _bstr_t bstr = value.at(i).c_str();
        hr = sa.put_element(i, bstr.GetBSTR());
        if(FAILED(hr)){
            LOG(LOG_LEVEL_ERROR,_T("sa.put_element failed (0x%08X)."), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, _T("sa.put_element failed."));
        }
    }
    vt.vt       = VT_ARRAY | VT_BSTR;
    vt.parray   = sa;
    hr = set_parameter( parameter, vt );
    vt.parray   = NULL;
    return hr;
}

HRESULT wmi_object::set_parameter(const std::wstring parameter, const std::vector<uint16_t>& value){
    _variant_t      vt;
    safe_array      sa;
    HRESULT         hr = S_OK;

    FUN_TRACE_HRESULT(hr);

    hr = sa.create(VT_I2, 1, (DWORD)value.size());
    if (FAILED(hr)){
        LOG(LOG_LEVEL_ERROR, _T("sa.create failed (0x%08X)."), hr);
        BOOST_THROW_WMI_EXCEPTION(hr, _T("sa.create failed."));
    }
    for (int i = 0; i < (int)value.size(); i++) {
        hr = sa.put_element(i, (PVOID)&value.at(i));
        if (FAILED(hr)){
            LOG(LOG_LEVEL_ERROR, _T("sa.put_element failed (0x%08X)."), hr);
            BOOST_THROW_WMI_EXCEPTION(hr, _T("sa.put_element failed."));
        }
    }
    vt.vt = VT_ARRAY | VT_I2;
    vt.parray = sa;
    hr = set_parameter(parameter, vt);
    vt.parray = NULL;
    return hr;
}

HRESULT wmi_object::set_parameter( const std::wstring parameter, const wmi_object& value){
    _variant_t      vt = value._obj_ptr;
    return set_parameter( parameter, vt );
}

HRESULT wmi_object::set_parameter( const std::wstring parameter, bool value){
    _variant_t      vt = value;
    return set_parameter( parameter, vt );
}

HRESULT wmi_object::exec_method( std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value, std::wstring& error ){
    if ( !_path.length() )
        get_parameter(L"__PATH", _path );
    if ( !_relpath.length())
       get_parameter(L"__Relpath", _relpath );
    
    if ( _path.length() || _relpath.length() ){
        if( _path.length() )
            return _wmi.exec_method( _path, method, inparameters, outparameters, return_value, error );
        else
            return _wmi.exec_method( _relpath, method, inparameters, outparameters, return_value, error );
    }
    else
        return E_POINTER;
}

HRESULT wmi_object::exec_method(std::wstring method, wmi_object& inparameters, bool& return_value){
    wmi_object   outparameters;
    return exec_method(method, inparameters, outparameters, return_value);
}

HRESULT wmi_object::exec_method(std::wstring method, wmi_object& inparameters, wmi_object& outparameters, bool& return_value){
    
    DWORD        ret = 0;
    std::wstring error;
    HRESULT hr = exec_method(method, inparameters, outparameters, ret, error);
    if (SUCCEEDED(hr))
        hr = outparameters.get_parameter(L"ReturnValue", return_value);
    return hr;
}

wmi_object wmi_object::exec_method( std::wstring method, wmi_object& inparameters ){

    HRESULT        hr = S_OK;
    DWORD          return_value = 0 ;
    std::wstring   error; 
    wmi_object     outparameters;
    hr = exec_method( method, inparameters, outparameters, return_value, error );
    if( FAILED( hr ) ){
#if _UNICODE  
        BOOST_THROW_WMI_EXCEPTION( hr, boost::str( boost::wformat(L"exec_method(%s) failed. - %s. Return(%d).") %method %error %return_value ) );
#else
        BOOST_THROW_WMI_EXCEPTION( hr, boost::str( boost::format("exec_method(%s) failed. - %s. Return(%d).") %stringutils::convert_unicode_to_ansi(method) %stringutils::convert_unicode_to_ansi(error) %return_value ) );
#endif
    }
    return outparameters;
}

HRESULT wmi_object::async_exec_method( std::wstring method, wmi_object& inparameters, wmi_object& outparameters, DWORD& return_value ){
    if ( !_path.length() )
        get_parameter(L"__PATH", _path );
    if ( !_relpath.length())
       get_parameter(L"__Relpath", _relpath );
    
    if ( _path.length() || _relpath.length() ){
        if( _path.length() )
            return _wmi.async_exec_method( _path, method, inparameters, outparameters, return_value );
        else
            return _wmi.async_exec_method( _relpath, method, inparameters, outparameters, return_value );
    }
    else
        return E_POINTER;
}

HRESULT wmi_object::get_input_parameters( std::wstring method, wmi_object& inparameters){
    if ( !_class.length())
       get_parameter(L"__Class", _class );
    if ( _class.length() )
        return _wmi.get_input_parameters( _class, method, inparameters );
    else
        return E_POINTER;
}

wmi_object wmi_object::get_input_parameters( std::wstring method ){
    wmi_object inparameters;
    HRESULT hr = get_input_parameters( method, inparameters);
    if (FAILED(hr)){
#if _UNICODE  
        BOOST_THROW_WMI_EXCEPTION( hr, boost::str( boost::wformat(L"get_input_parameters(%s) failed.") %method ) );
#else
        BOOST_THROW_WMI_EXCEPTION( hr, boost::str( boost::format("get_input_parameters(%s) failed.") %stringutils::convert_unicode_to_ansi(method) ) );
#endif
    }
    return inparameters;
}

wmi_object_table wmi_object::get_relateds(const std::wstring result_class, const std::wstring assoc_class){
    wmi_object_table objects;
    HRESULT hr = S_OK;  
    if (!_path.length())
        get_parameter(L"__PATH", _path);
    std::wstring query = boost::str(boost::wformat(L"ASSOCIATORS OF {%s} WHERE RESULTCLASS=%s ASSOCCLASS=%s") %_path%result_class%assoc_class);
    hr = _wmi.exec_query(query, objects);
    if (FAILED(hr)){
#if _UNICODE  
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::wformat(L"Query \"ASSOCIATORS OF {%s} WHERE RESULTCLASS=%s ASSOCCLASS=%s\" failed.") % _path%result_class%assoc_class));
#else
        BOOST_THROW_WMI_EXCEPTION(hr, boost::str(boost::format("Query \"ASSOCIATORS OF {%s} WHERE RESULTCLASS=%s ASSOCCLASS=%s\" failed.") % stringutils::convert_unicode_to_ansi(_path) % stringutils::convert_unicode_to_ansi(result_class) % stringutils::convert_unicode_to_ansi(assoc_class)));
#endif
    }
    return objects;
}

wmi_object wmi_object::get_related(const std::wstring result_class, const std::wstring assoc_class){
    wmi_object_table objs = get_relateds(result_class, assoc_class);
    if (objs.size() == 0){
#if _UNICODE  
        BOOST_THROW_WMI_EXCEPTION(ERROR_OBJECT_NOT_FOUND, boost::str(boost::wformat(L"Query \"ASSOCIATORS OF {%s} WHERE RESULTCLASS=%s ASSOCCLASS=%s\" returned empty.") %_path%result_class%assoc_class));
#else
        BOOST_THROW_WMI_EXCEPTION(ERROR_OBJECT_NOT_FOUND, boost::str(boost::format("Query \"ASSOCIATORS OF {%s} WHERE RESULTCLASS=%s ASSOCCLASS=%s\" returned empty.") %stringutils::convert_unicode_to_ansi(_path)%stringutils::convert_unicode_to_ansi(result_class)%stringutils::convert_unicode_to_ansi(assoc_class)));
#endif
    }
    return objs[0];
}

/**********************************************************
    wmi_element
***********************************************************/

wmi_element&    wmi_element::operator =( const wmi_object& value ){
    return set_value<const wmi_object&>( value );
}

wmi_element&    wmi_element::operator =( const std::wstring value ) { 
    return set_value<const std::wstring>( value );
}

wmi_element&    wmi_element::operator =( ULONGLONG    value ){ 
    return set_value<ULONGLONG>( value );
}

wmi_element&    wmi_element::operator =( DWORD value ){ 
    return set_value<DWORD>( value );
} 

wmi_element&    wmi_element::operator =( const string_table_w& value ){ 
    return set_value<const string_table_w&>( value );
}

wmi_object wmi_element::get_wmi_object(){
    wmi_object value;
    HRESULT hr = _obj.get_parameter( _name, value );
    _existed = ( FAILED(hr) ) ? false : true;
    return value;
}

/**********************************************************
    wmi_job
***********************************************************/
wmi_job::wmi_job(wmi_services& wmi, wmi_object& object, std::wstring name)
: _wmi(wmi)
, _object(object)
, _name(name){
}

wmi_job::~wmi_job(){
}

HRESULT wmi_job::update(){

    HRESULT hr = S_OK;
    _job = NULL;
    FUN_TRACE_HRESULT(hr);
    std::wstring path;
    hr = _object.get_parameter(_name, path);
    if(FAILED(hr)){
        LOG(LOG_LEVEL_ERROR,_T("Failed to get job path (0x%08X)."), hr);
    }
    else{
        hr = _wmi.get_wmi_object( path, _job );
        if(FAILED(hr)){
            LOG(LOG_LEVEL_ERROR,_T("Failed to get job object (0x%08X)."), hr);
        }
    }

    return hr;
}

HRESULT wmi_job::get_state(DWORD& state){

    HRESULT hr = S_OK;
    state = WMI_JOBSTATE_UNKNOWN;
    FUN_TRACE_HRESULT(hr);
    hr = _job.get_parameter(L"JobState", state );
    if(FAILED(hr)){
        LOG(LOG_LEVEL_ERROR,_T("Failed to get job state (0x%08X)."), hr);
    }
    return hr;
}

HRESULT wmi_job::get_error_code(DWORD& code){

    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    hr = _job.get_parameter(L"ErrorCode", code);
    if(FAILED(hr)){
        LOG(LOG_LEVEL_ERROR,_T("Failed to get job error code (0x%08X)."), hr);
    }
    return hr;
}

HRESULT wmi_job::get_error_description(std::wstring& desc){

    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    hr = _job.get_parameter(L"ErrorDescription", desc);
    if(FAILED(hr)){
        LOG(LOG_LEVEL_ERROR,_T("Failed to get job error description (0x%08X)."), hr);
    }
    return hr;
}

bool wmi_job::is_running(){

    bool isRunning = false;
    FUN_TRACE;
    DWORD state = WMI_JOBSTATE_UNKNOWN;
    get_state(state);
    isRunning = ( (WMI_JOBSTATE_STARTING == state) || (WMI_JOBSTATE_RUNNING == state) );
    return isRunning;
}

HRESULT wmi_job::wait_until_job_completes(DWORD& code, std::wstring& desc){
    
    HRESULT hr = S_OK;
    code = 0;
    desc = L"";

    FUN_TRACE_HRESULT(hr);

    hr = update();
    if(FAILED(hr)){
        return hr;
    }

    while(is_running()){
        LOG( LOG_LEVEL_DEBUG, L"Job is still running.");
        Sleep(1000);
        hr = update();
        if(FAILED(hr)){
            return hr;
        }
    }

    hr = get_error_code(code);
    if(FAILED(hr)){
        return hr;
    }

    hr = get_error_description(desc);

    return hr;
}

#endif

};//namespace windows
};//namespace macho

#endif
// msi_component.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_MSI_COMPONENT__
#define __MACHO_WINDOWS_MSI_COMPONENT__
#include "..\config\config.hpp"

namespace macho{

namespace windows{

#include <msi.h>
#pragma comment(lib, "Msi.lib")            
// http://support.microsoft.com/kb/234788/en-us

class msi_component {
private:
    stdstring     _product_code;
    stdstring     _installed_product_name;
    stdstring     _installed_path;
    stdstring     _installation_source;
    stdstring     _component_id;
    INSTALLSTATE _installstate;
public:
    msi_component() : _installstate ( INSTALLSTATE_UNKNOWN ){ }
    msi_component(const msi_component &msi) : _installstate ( INSTALLSTATE_UNKNOWN ){
        copy( msi );
    }
    msi_component( stdstring component_id ) : _installstate ( INSTALLSTATE_UNKNOWN ){
        *this = get_component(component_id);
    }
    virtual ~msi_component(){}
    static msi_component get_component( stdstring component_id );
    void   copy( const msi_component &msi );
    const  msi_component &operator =( const msi_component &msi );

    bool         inline is_installed()            const    { return _installed_path.length() ? true : false;    };
    bool         inline is_local_install()        const   { return ( _installstate == INSTALLSTATE_LOCAL ) ? true : false; };
    bool         inline is_source_install()        const   { return ( _installstate == INSTALLSTATE_SOURCE ) ? true : false; };
    stdstring    inline product_name()            const    { return _installed_product_name ;}
    stdstring    inline installed_path()        const    { return _installed_path; }
    stdstring    inline product_code()            const    { return _product_code; }
    stdstring    inline installation_source()    const   { return _installation_source; }
    INSTALLSTATE inline installstate()          const   { return _installstate; }
    // Office 2000
    static const TCHAR Word200[39];
    static const TCHAR Excel2000[39];
    static const TCHAR PowerPoint2000[39];
    static const TCHAR Access2000[39];
    static const TCHAR Office2000[39];

    // Office XP
    static const TCHAR WordXP[39];
    static const TCHAR ExcelXP[39];
    static const TCHAR PowerPointXP[39];
    static const TCHAR AccessXP[39];
    static const TCHAR OfficeXP[39];

    // Office 2003
    static const TCHAR Word2003[39];
    static const TCHAR Excel2003[39];
    static const TCHAR PowerPoint2003[39];
    static const TCHAR Access2003[39];
    static const TCHAR Outlook2003[39];
    static const TCHAR Office2003[39];

    // Office 2007
    static const TCHAR Word2007[39];
    static const TCHAR Excel2007[39];
    static const TCHAR PowerPoint2007[39];
    static const TCHAR Access2007[39];
    static const TCHAR Outlook2007[39];
    static const TCHAR Office2007[39];

    // Office 2010 x86
    static const TCHAR Word2010_32[39];
    static const TCHAR Excel2010_32[39];
    static const TCHAR PowerPoint2010_32[39];
    static const TCHAR Access2010_32[39];
    static const TCHAR Outlook2010_32[39];
    static const TCHAR Office2010_32[39];
                    
    // Office 2010 x64
    static const TCHAR Word2010_64[39];
    static const TCHAR Excel2010_64[39];
    static const TCHAR PowerPoint2010_64[39];
    static const TCHAR Access2010_64[39];
    static const TCHAR Outlook2010_64[39];
    static const TCHAR Office2010_64[39];
};

#ifndef MACHO_HEADER_ONLY

// Office 2000
const TCHAR msi_component::Word200[39]            = _T("{CC29E963-7BC2-11D1-A921-00A0C91E2AA2}");
const TCHAR msi_component::Excel2000[39]        = _T("{CC29E96F-7BC2-11D1-A921-00A0C91E2AA2}");
const TCHAR msi_component::PowerPoint2000[39]   = _T("{CC29E94B-7BC2-11D1-A921-00A0C91E2AA2}");
const TCHAR msi_component::Access2000[39]        = _T("{CC29E967-7BC2-11D1-A921-00A0C91E2AA2}");
const TCHAR msi_component::Office2000[39]        = _T("{00000409-78E1-11D2-B60F-006097C998E7}");

// Office XP
const TCHAR msi_component::WordXP[39]            = _T("{8E46FEFA-D973-6294-B305-E968CEDFFCB9}");
const TCHAR msi_component::ExcelXP[39]            = _T("{5572D282-F5E5-11D3-A8E8-0060083FD8D3}");
const TCHAR msi_component::PowerPointXP[39]        = _T("{FC780C4C-F066-40E0-B720-DA0F779B81A9}");
const TCHAR msi_component::AccessXP[39]            = _T("{CC29E967-7BC2-11D1-A921-00A0C91E2AA3}");
const TCHAR msi_component::OfficeXP[39]            = _T("{20280409-6000-11D3-8CFE-0050048383C9}");

// Office 2003
const TCHAR msi_component::Word2003[39]            = _T("{1EBDE4BC-9A51-4630-B541-2561FA45CCC5}");
const TCHAR msi_component::Excel2003[39]        = _T("{A2B280D4-20FB-4720-99F7-40C09FBCE10A}");
const TCHAR msi_component::PowerPoint2003[39]   = _T("{C86C0B92-63C0-4E35-8605-281275C21F97}");
const TCHAR msi_component::Access2003[39]        = _T("{F2D782F8-6B14-4FA4-8FBA-565CDDB9B2A8}");
const TCHAR msi_component::Outlook2003[39]      = _T("{3CE26368-6322-4ABF-B11B-458F5C450D0F}");
const TCHAR msi_component::Office2003[39]        = _T("{90110409-6000-11D3-8CFE-0150048383C9}");

// Office 2007
const TCHAR msi_component::Word2007[39]            = _T("{0638C49D-BB8B-4CD1-B191-051E8F325736}");
const TCHAR msi_component::Excel2007[39]        = _T("{0638C49D-BB8B-4CD1-B191-052E8F325736}");
const TCHAR msi_component::PowerPoint2007[39]   = _T("{0638C49D-BB8B-4CD1-B191-053E8F325736}");
const TCHAR msi_component::Access2007[39]        = _T("{0638C49D-BB8B-4CD1-B191-054E8F325736}");
const TCHAR msi_component::Outlook2007[39]        = _T("{0638C49D-BB8B-4CD1-B191-055E8F325736}");
const TCHAR msi_component::Office2007[39]        = _T("{0638C49D-BB8B-4CD1-B191-050E8F325736}");

// Office 2010 x86
const TCHAR msi_component::Word2010_32[39]            = _T("{019C826E-445A-4649-A5B0-0BF08FCC4EEE}"); 
const TCHAR msi_component::Excel2010_32[39]            = _T("{538F6C89-2AD5-4006-8154-C6670774E980}");
const TCHAR msi_component::PowerPoint2010_32[39]    = _T("{E72E0D20-0D63-438B-BC71-92AB9F9E8B54}");
const TCHAR msi_component::Access2010_32[39]        = _T("{AE393348-E564-4894-B8C5-EBBC5E72EFC6}");
const TCHAR msi_component::Outlook2010_32[39]        = _T("{CFF13DD8-6EF2-49EB-B265-E3BFC6501C1D}");
const TCHAR msi_component::Office2010_32[39]        = _T("{398E906A-826B-48DD-9791-549C649CACE5}");
                    
// Office 2010 x64
const TCHAR msi_component::Word2010_64[39]            = _T("{C0AC079D-A84B-4CBD-8DBA-F1BB44146899}"); 
const TCHAR msi_component::Excel2010_64[39]            = _T("{8B1BF0B4-A1CA-4656-AA46-D11C50BC55A4}");
const TCHAR msi_component::PowerPoint2010_64[39]    = _T("{EE8D8E0A-D905-401D-9BC3-0D20156D5E30}");
const TCHAR msi_component::Access2010_64[39]        = _T("{02F5CBEC-E7B5-4FC1-BD72-6043152BD1D4}");
const TCHAR msi_component::Outlook2010_64[39]        = _T("{ECCC8A38-7855-46CA-88FB-3BAA7CD95E56}");
const TCHAR msi_component::Office2010_64[39]        = _T("{E6AC97ED-6651-4C00-A8FE-790DB0485859}");

const msi_component & msi_component::operator =(const msi_component &msi){
    if ( this != &msi )
        copy( msi );
    return( *this );
}

void msi_component::copy( const msi_component &msi ) { 
    _product_code           = msi._product_code;
    _installed_product_name = msi._installed_product_name;
    _installed_path         = msi._installed_path;
    _installation_source    = msi._installation_source;
    _component_id           = msi._component_id;
    _installstate           = msi._installstate;
}

msi_component msi_component::get_component( stdstring component_id ){
    msi_component msi;
    DWORD size = 0;
    TCHAR sProductCode[40];
    memset( sProductCode, 0, sizeof(sProductCode));
    msi._component_id = component_id;

    if ( ERROR_SUCCESS == MsiGetProductCode( msi._component_id.c_str(), sProductCode ) ){
        msi._product_code = sProductCode;
        TCHAR* sProduceName = NULL;
        TCHAR* sSourcePath = NULL;
        MsiGetProductInfo( sProductCode, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, NULL, &size) ;
        size ++;
        sProduceName = new TCHAR[size];
        if ( NULL != sProduceName ){
            memset(sProduceName, 0 , (size) * sizeof(TCHAR) );
            if ( ERROR_SUCCESS == MsiGetProductInfo( sProductCode, INSTALLPROPERTY_INSTALLEDPRODUCTNAME, sProduceName, &size) ){
                msi._installed_product_name = sProduceName;
            }
            delete sProduceName;
        }

        size = 0;
        MsiGetProductInfo( sProductCode, INSTALLPROPERTY_INSTALLSOURCE, NULL, &size) ;
        size ++;
        sSourcePath = new TCHAR[size];
        if ( NULL != sSourcePath ){
            memset(sSourcePath, 0 , (size) * sizeof(TCHAR) );
            if ( ERROR_SUCCESS == MsiGetProductInfo( sProductCode, INSTALLPROPERTY_INSTALLSOURCE, sSourcePath, &size) ){
                msi._installation_source = sSourcePath;
            }
            delete sSourcePath;
        }
    }
    size = 300;
    TCHAR *sPath = new TCHAR[size];
    while ( NULL != sPath ){
        msi._installstate = MsiLocateComponent(msi._component_id.c_str(), sPath, &size);
        if ( INSTALLSTATE_MOREDATA == msi._installstate ){
            delete sPath;
            size++;
            sPath = new TCHAR[size];
        }
        else{
            if ((msi._installstate == INSTALLSTATE_LOCAL) || 
                (msi._installstate == INSTALLSTATE_SOURCE)) {
                msi._installed_path = sPath;
            }
            delete sPath;
            sPath = NULL;
        }
    }
    return msi;
}

#endif

};//namespace windows
};//namespace macho

#endif
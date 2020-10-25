// com_init.hpp ----------------------------------------------//
// -----------------------------------------------------------------------------

// Copyright 2013-2014 Albert Wei

// Distributed under the Boost Software License, Version 1.0.
// See http://www.boost.org/LICENSE_1_0.txt

// -----------------------------------------------------------------------------

// Revision History
// 04-29-2014 - Initial Release

// -----------------------------------------------------------------------------

#pragma once

#ifndef __MACHO_WINDOWS_COM_INIT__
#define __MACHO_WINDOWS_COM_INIT__
#include "..\config\config.hpp"
#include "..\common\tracelog.hpp"

namespace macho{

namespace windows{

class com_init{

public:
    com_init( bool is_multi_threaded = true ) {
        _initialed = ( S_OK == initialize( is_multi_threaded ) ) ;
    }
     ~com_init(){
        if ( _initialed ) uninitialize();
    }
    HRESULT static initialize( bool is_multi_threaded );
    void    static uninitialize(){  CoUninitialize(); }
private:
    bool _initialed;
};

#ifndef MACHO_HEADER_ONLY

HRESULT com_init::initialize( bool is_multi_threaded )
{
    HRESULT hr          = S_OK;
    DWORD   coinit1     = 0;
    DWORD   coinit2     = 0;

    if(is_multi_threaded){
        coinit1 = COINIT_MULTITHREADED;
        coinit2 = COINIT_APARTMENTTHREADED;
    }
    else{
        coinit1 = COINIT_APARTMENTTHREADED;
        coinit2 = COINIT_MULTITHREADED;
    }
    
    // First choice
    hr = CoInitializeEx(NULL, coinit1);
    if(FAILED(hr)){

        LOG(LOG_LEVEL_WARNING,_T("CoInitializeEx returned (0x%08x) with model (%d)"), hr, coinit1);
        // Second choice
        hr = CoInitializeEx(NULL, coinit2);
        if(FAILED(hr)){
            LOG(LOG_LEVEL_ERROR,_T("CoInitializeEx returned (0x%08x) with model (%d)"), hr, coinit2);
        }
        else{
            LOG(LOG_LEVEL_TRACE,_T("CoInitializeEx returned (0x%08x) with model (%d)"), hr, coinit2);
        }
    }
    else{
        LOG(LOG_LEVEL_TRACE,L"CoInitializeEx returned (0x%08x) with model (%d)", hr, coinit1);
    }
    
    return hr;
}

#endif

};//namespace windows
};//namespace macho

#endif
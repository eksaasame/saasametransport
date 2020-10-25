
// DrProfiler.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
namespace po = boost::program_options;
// CDrProfilerApp:
// See DrProfiler.cpp for the implementation of this class
//

class CDrProfilerApp : public CWinApp
{
public:
	CDrProfilerApp();

// Overrides
public:
	virtual BOOL InitInstance();
    bool    command_line_parser( po::variables_map &vm );
// Implementation
    void    ShowDevice( PROFILE_STATE_ENUM state, hardware_class::ptr& c, hardware_device::ptr& device, hardware_driver::ptr& driver );
    void    SaveProfile( PROFILE_STATE_ENUM state, hardware_device::ptr& device, hardware_driver::ptr& driver, stdstring& file_name, stdstring& err );
	DECLARE_MESSAGE_MAP()
private :
    hardware_device::vtr  _devices;
};

extern CDrProfilerApp theApp;
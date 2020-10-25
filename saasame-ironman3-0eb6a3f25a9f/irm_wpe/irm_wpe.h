
// irm_wpe.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Cirm_wpeApp:
// See irm_wpe.cpp for the implementation of this class
//

class Cirm_wpeApp : public CWinApp
{
public:
	Cirm_wpeApp();

// Overrides
public:
	virtual BOOL InitInstance();


    DWORD  InitialNetWork();
// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Cirm_wpeApp theApp;
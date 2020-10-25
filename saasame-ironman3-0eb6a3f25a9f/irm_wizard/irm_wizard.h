
// irm_wizard.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Cirm_wizardApp:
// See irm_wizard.cpp for the implementation of this class
//

class Cirm_wizardApp : public CWinApp
{
public:
	Cirm_wizardApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Cirm_wizardApp theApp;
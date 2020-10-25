
// irm_wizard.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "irm_wizard.h"
#include "WizardMgmt.h"
#include "..\irm_wpe\WizardSheet.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cirm_wizardApp

BEGIN_MESSAGE_MAP(Cirm_wizardApp, CWinApp)
    ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// Cirm_wizardApp construction

Cirm_wizardApp::Cirm_wizardApp()
{
    // support Restart Manager
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}


// The one and only Cirm_wizardApp object

Cirm_wizardApp theApp;


// Cirm_wizardApp initialization

BOOL Cirm_wizardApp::InitInstance()
{
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);

    CWinApp::InitInstance();


    AfxEnableControlContainer();

    // Create the shell manager, in case the dialog contains
    // any shell tree view or shell list view controls.
    CShellManager *pShellManager = new CShellManager;

    // Activate "Windows Native" visual manager for enabling themes in MFC controls
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need
    // Change the registry key under which our settings are stored
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization
    SetRegistryKey(_T("Local AppWizard-Generated Applications"));
    macho::windows::com_init _com;
    boost::filesystem::path logfile;
    boost::filesystem::path app_path = macho::windows::environment::get_execution_full_path();
    logfile = app_path.parent_path().string() + "/logs/" + app_path.filename().stem().string() + ".log";
    boost::filesystem::create_directories(logfile.parent_path());
    macho::set_log_file(logfile.wstring());

    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    if (!macho::windows::environment::is_running_as_administrator()){
        CString message;
        message.LoadStringW(IDS_PERMISSION);
        AfxMessageBox(message);
        return 1;
    }
    CPropertySheet    sheet(L"");
    register_info     info;
    CWizardMgmt       page1(info, CWizardPage::Only);
    sheet.AddPage(&page1);
    sheet.SetWizardMode();
    sheet.m_psh.dwFlags &= ~PSH_HASHELP;
    page1.m_psp.dwFlags |= PSP_USETITLE;
    CString title;
    title.LoadStringW(CWizardMgmt::is_transport() ? IDS_TRANSPORT_REG : IDS_PACKER_REG);
    page1.m_psp.pszTitle = title;

    INT_PTR nResponse = sheet.DoModal();
    if (nResponse == IDOK)
    {
        // TODO: Place code here to handle when the dialog is
        //  dismissed with OK
    }
    else if (nResponse == IDCANCEL)
    {
        // TODO: Place code here to handle when the dialog is
        //  dismissed with Cancel
    }
    else if (nResponse == -1)
    {
        TRACE(traceAppMsg, 0, "Warning: dialog creation failed, so application is terminating unexpectedly.\n");
        TRACE(traceAppMsg, 0, "Warning: if you are using MFC controls on the dialog, you cannot #define _AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
    }

    // Delete the shell manager created above.
    if (pShellManager != NULL)
    {
        delete pShellManager;
    }

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}


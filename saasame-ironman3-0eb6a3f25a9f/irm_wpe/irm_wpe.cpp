
// irm_wpe.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "irm_wpe.h"
#include "WizardStart.h"
#include "WizardNet.h"
#include "WizardMgmt.h"
#include "WizardSheet.h"
#include "ReadyDlg.h"
#include <curl/curl.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cirm_wpeApp

BEGIN_MESSAGE_MAP(Cirm_wpeApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


DWORD Cirm_wpeApp::InitialNetWork(){
    typedef int(*WpeutilFunction)(
        HINSTANCE hInst,
        HINSTANCE hPrev,
        LPTSTR lpszCmdLine,
        int nCmdShow
        );
    HMODULE         hWpeutil = NULL;
    WpeutilFunction InitializeNetwork = NULL;
    int             result = 0;
    TCHAR           szCmdLine[] = _T("");
    hWpeutil = LoadLibrary(_T("wpeutil"));
    if (NULL == hWpeutil){
        AfxMessageBox(_T("Unable to load wpeutil.dll"));
        return GetLastError();
    }
    InitializeNetwork = (WpeutilFunction)GetProcAddress(
        hWpeutil,
        "InitializeNetworkW"
        );

    if (NULL == InitializeNetwork){
        FreeLibrary(hWpeutil);
        return GetLastError();
    }
    result = InitializeNetwork(NULL, NULL, szCmdLine, SW_SHOW);
    if (ERROR_SUCCESS == result){
      //  AfxMessageBox(_T("Network initialized"));
    }
    else{
        AfxMessageBox(boost::str(boost::wformat(L"Initialize failed: 0x%08x")% result).c_str());
    }
    FreeLibrary(hWpeutil);
    return result;
}

// Cirm_wpeApp construction

Cirm_wpeApp::Cirm_wpeApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only Cirm_wpeApp object

Cirm_wpeApp theApp;


// Cirm_wpeApp initialization

BOOL Cirm_wpeApp::InitInstance()
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
#ifndef _DEBUG
    if (macho::windows::environment::is_winpe())
        InitialNetWork();
#endif
    /*
     Using MFC application in WinPE, put the oledlg.dll and exe file together. The other way is remove AfxEnableControlContainer(); from the source code, if it does not have to use ActiveX controls.
    */

	//AfxEnableControlContainer();

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
    winpe_settings settings;
    boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
    std::wstring loader = boost::filesystem::path(work_dir / L"irm_loader.exe").wstring();
    std::wstring launcher = boost::filesystem::path(work_dir / L"irm_launcher.exe").wstring();
    std::wstring host_packer = boost::filesystem::path(work_dir / L"irm_host_packer.exe").wstring();
    bool is_rcd = boost::filesystem::exists(loader) && boost::filesystem::exists(launcher);
    std::wstring ret;
    if (settings.load()){
        settings.apply();
        winpe_settings::sync_time();
        if (boost::filesystem::exists(loader))
            process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s -i") % loader), ret, -1, true);
        if (boost::filesystem::exists(launcher))
            process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s -i") % launcher), ret, -1, true);
        if (boost::filesystem::exists(host_packer))
            process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s -i") % host_packer), ret, -1, true);
        try{
            if (boost::filesystem::exists(loader)){
                macho::windows::service loader_s = macho::windows::service::get_service(L"irm_loader");
                loader_s.set_recovery_policy({ 60000, 60000, 60000 }, 86400);
                loader_s.start();
            }
            if (boost::filesystem::exists(launcher)){
                macho::windows::service launcher_s = macho::windows::service::get_service(L"irm_launcher");
                launcher_s.set_recovery_policy({ 60000, 60000, 60000 }, 86400);
                launcher_s.start();
            }
            if (boost::filesystem::exists(host_packer)){
                macho::windows::service host_packer_s = macho::windows::service::get_service(L"irm_host_packer");
                host_packer_s.set_recovery_policy({ 60000, 60000, 60000 }, 86400);
                host_packer_s.start();
            }
        }
        catch (macho::exception_base &ex){
            AfxMessageBox(macho::get_diagnostic_information(ex).c_str());
        }
        macho::windows::registry reg;
        bool https_mode = (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))
            && reg[L"SessionId"].exists() && reg[L"MgmtAddr"].exists());
        if (!https_mode){
            std::wstring ret;
            process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s\\wpeutil.exe DisableFirewall") % macho::windows::environment::get_system_directory()), ret, -1, true);
        }
        CReadyDlg ready;
        ready.DoModal();
    }
    else{

        try{
            boost::filesystem::remove_all(boost::filesystem::path(work_dir / L"jobs"));
            boost::filesystem::remove_all(boost::filesystem::path(work_dir / L"logs"));
            boost::filesystem::remove_all(boost::filesystem::path(work_dir / L"connections"));
        }
        catch(...){
        }

        CWizardSheet       sheet(L"");
        CWizardStart       page1(settings,CWizardPage::First, is_rcd);
        CWizardNet         page2(settings,CWizardPage::Middle);
        CWizardMgmt        page3(settings,CWizardPage::Last);
        sheet.AddPage(&page1);
        sheet.AddPage(&page2);
        sheet.AddPage(&page3);

        sheet.SetWizardMode();
        sheet.m_psh.dwFlags &= ~PSH_HASHELP;
        //page1.m_psp.dwFlags &= ~PSP_HASHELP;
        //sheet.m_psh.dwFlags |= PSH_NOAPPLYNOW;
        INT_PTR nResponse = sheet.DoModal();

        if (nResponse == ID_WIZFINISH)
        {
            settings.save();
            winpe_settings::sync_time();
            // TODO: Place code here to handle when the dialog is
            //  dismissed with OK    
            if (!page3._addr.empty()){
                try{
                    macho::windows::service irm_launcher = macho::windows::service::get_service(L"irm_launcher");
                    irm_launcher.stop();
                    irm_launcher.start();
                }
                catch (...){
                    try{
                        macho::windows::service irm_host_packer = macho::windows::service::get_service(L"irm_host_packer");
                        irm_host_packer.stop();
                        irm_host_packer.start();
                    }
                    catch (...){
                    }
                }
            }
            CReadyDlg ready;
            ready.DoModal();
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


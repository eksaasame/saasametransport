
// DrProfiler.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "DrProfiler.h"
#include "DrProfilerDlg.h"
#include "..\..\buildenv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDrProfilerApp

BEGIN_MESSAGE_MAP(CDrProfilerApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDrProfilerApp construction

CDrProfilerApp::CDrProfilerApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}


// The one and only CDrProfilerApp object

CDrProfilerApp theApp;


// CDrProfilerApp initialization
bool CDrProfilerApp::command_line_parser( po::variables_map &vm ){
    
    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str( boost::format( "%s\n") % GetCommandLineA() );
#endif
    title += boost::str( boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") %PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD );
	boost::filesystem::path output = boost::filesystem::path(environment::get_environment_variable(_T("USERPROFILE")))/_T("Desktop");
    po::options_description command("");
	command.add_options()
#if _UNICODE
        ("output,o",  po::wvalue<std::wstring>()->default_value(output.wstring(), output.string()),  "output folder")
        ("logfile,f", po::wvalue<std::wstring>()->default_value(L"", ""),  "log file path")        
#else
        ("output,o",  po::value<std::string>()->default_value(output.string(), output.string()),  "output folder")
        ("logfile,f", po::value<std::string>()->default_value("", ""),  "log file path")
#endif
        ("type,t",    po::value<int>()->default_value(0, "0"), "output type ( 0 ~ 2 ) \r\n0: Self-extracting file \r\n1: Cab file \r\n2: Structed folder\r\n")
        ("level,l",   po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("console,c", "console mode to generate the output package (option)")
		("help,h"   , "produce help message (option)");
		;
	try{
#if _UNICODE
		po::store(po::wcommand_line_parser( po::split_winmain( GetCommandLine() ) ).options(command).run(), vm);
#else
        po::store(po::command_line_parser( po::split_winmain( GetCommandLine() ) ).options(command).run(), vm);
#endif
		po::notify(vm);
		if ( vm.count("help") || ( vm["type"].as<int>() > 2 ) || ( vm["type"].as<int>() < 0 ) || ( vm["level"].as<int>() > 5 ) || ( vm["level"].as<int>() < 0 ) ){
            std::cout << title << command << std::endl;
		}
        else { 
            result = true;
        }
	}
	catch ( const boost::program_options::multiple_occurrences& e ) {
		std::cout << title <<  command << "\n";
		std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
	}
	catch ( const boost::program_options::error& e ) {
		std::cout << title << command << "\n";
        std::cout << e.what() << std::endl;
    }
	catch( boost::exception &e ){
		std::cout << title << command << "\n";
		std::cout << boost::exception_detail::get_diagnostic_information( e, "Invalid command parameter format." ) << std::endl;
	}
	catch(...){
		std::cout << title << command << "\n";
		std::cout << "Invalid command parameter format." << std::endl;
	}
	return result;
}

void CDrProfilerApp::ShowDevice( PROFILE_STATE_ENUM state, hardware_class::ptr& c, hardware_device::ptr& device, hardware_driver::ptr& driver )
{
    CString message;
    if ( state == PROFILE_STATE_ON_PROGRESS ){
        message.LoadString(IDS_ENUM_PROGRESS);
        std::wcout << boost::str(boost::wformat(message.GetString())%device->device_description) << std::endl;
        _devices.push_back(device);
    }
    else if ( state == PROFILE_STATE_START ){
        message.LoadString(IDS_ENUM_START);   
        std::wcout << std::endl << message.GetString() << std::endl;
    }
    else if ( state == PROFILE_STATE_FINISHED ){ 
    }
}

void CDrProfilerApp::SaveProfile( PROFILE_STATE_ENUM state, hardware_device::ptr& device, hardware_driver::ptr& driver, stdstring& file_name, stdstring& err )
{
    CString message;
    if ( err.length() )
        std::wcout << err << std::endl;
    else if ( state == PROFILE_STATE_ON_PROGRESS ){
        message.LoadString(IDS_SAVE_PROGRESS);
        std::wcout << boost::str(boost::wformat(message.GetString())%device->device_description%file_name) << std::endl;
    }
    else if ( state == PROFILE_STATE_SYSTEM_INFO ){
        message.LoadString(IDS_SAVE_SYSTEM_INFO);
        std::wcout << message.GetString() << std::endl;
    }
    else if ( state == PROFILE_STATE_COMPRESS ){
        message.LoadString(IDS_SAVE_COMPRESS);
        std::wcout << message.GetString() << std::endl;
    }
    else if ( state == PROFILE_STATE_CREATE_SELFEXTRACT ){
        message.LoadString(IDS_SAVE_EXE);
        std::wcout << message.GetString() << std::endl;
    }
    else if ( state == PROFILE_STATE_START ){
        message.LoadString(IDS_SAVE_START);
        std::wcout << message.GetString() << std::endl;
    }
    else if ( state == PROFILE_STATE_FINISHED ){
        message = _T("Finished generating output!");
        std::wcout << message.GetString() << std::endl;
    }
}

BOOL CDrProfilerApp::InitInstance()
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
    // The important lines:
	if ( AttachConsole( ATTACH_PARENT_PROCESS ) ){ 
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
        setvbuf( stdin, NULL, _IONBF, 0 );
        setvbuf( stdout, NULL, _IONBF, 0 );
        setvbuf( stderr, NULL, _IONBF, 0 );
        std::ios::sync_with_stdio();
	}

	// Create the shell manager, in case the dialog contains
	// any shell tree view or shell list view controls.
	CShellManager *pShellManager = new CShellManager;

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
    po::variables_map vm;
    if ( command_line_parser( vm ) ){
        stdstring output = vm["output"].as<stdstring>();
        if ( output.length() == 0 ) output = (boost::filesystem::path(environment::get_environment_variable(_T("USERPROFILE")))/_T("Desktop")).wstring();
        stdstring logfile = vm["logfile"].as<stdstring>();
        if ( logfile.length() ){
            set_log_file( logfile );
            set_log_level( (TRACE_LOG_LEVEL) vm["level"].as<int>() );
            LOG( LOG_LEVEL_RECORD, boost::str( boost::wformat(L"------ %s, Version: %d.%d Build: %d ------") %PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD ).c_str() );
        }

#if defined(_WIN64)
        //if ( environment::is_64bit_process() )
#elif defined(_WIN32)
        if ( environment::is_wow64_process() ){
            stdstring x64_binary_file = boost::str( boost::wformat(L"%s\\%s") % environment::get_temp_path() % environment::get_execution_filename() );
            boost::filesystem::remove(x64_binary_file);
            ProfileModule::extract_binary_resource( _T("BINARIES"), IDR_DRPROFILE_X64, x64_binary_file );
            if ( boost::filesystem::exists(x64_binary_file) ){
                stdstring command  = boost::str( boost::wformat(L"%s -o \"%s\" -t %d -l %d")%x64_binary_file%output%vm["type"].as<int>()%vm["level"].as<int>() );
                if ( logfile.length() ) command.append( boost::str(boost::wformat(L" -f %s")%logfile) );
                if ( vm.count("console") > 0 ) command.append(L" -c");
                ProfileModule::exec_console_application( command, stdstring(), false );
                boost::filesystem::remove(x64_binary_file);
                return GetLastError();
            }
        }
        else
#endif    
        {
            ProfileModule module;
            PROFILE_TYPE_ENUM type = (PROFILE_TYPE_ENUM)vm["type"].as<int>();
            module.set_default_path(output);
            module.set_default_type(type);
            if ( vm.count("console") > 0 ){
                module.set_enumerate_devices_callback( boost::bind( &CDrProfilerApp::ShowDevice, this, _1, _2, _3, _4 ) );
                module.set_save_profile_callback( boost::bind( &CDrProfilerApp::SaveProfile, this, _1, _2, _3, _4, _5 ) );
                module.enum_devices();
                stdstring path = module.get_default_path(type);
                time_t now = time(NULL);
                TCHAR szNow[30]={0};
                stdstring computer_name = environment::get_computer_name();
                switch( type ){
                case PROFILE_TYPE_EXE:
                    _tcsftime(szNow, 30, _T("%Y_%m_%d-%H_%M_%S"), localtime(&now));
                    path.append(_T("\\")).append(boost::str(boost::wformat(L"\\%s_%s.exe") %computer_name %szNow));
                    break;
                case PROFILE_TYPE_CAB:
                    _tcsftime(szNow, 30, _T("%Y_%m_%d-%H_%M_%S"), localtime(&now));
                    path.append(_T("\\")).append(boost::str(boost::wformat(L"\\%s_%s.cab") %computer_name %szNow));
                    break;
                case PROFILE_TYPE_FOLDER:
                     _tcsftime(szNow, 30, _T("Date_%Y_%m_%d_Time_%H_%M_%S"), localtime(&now));
                     path.append(_T("\\")).append(szNow);
                    break;
                }
                module.save_profile(type, path, _devices );                
            }
            else{
	            CDrProfilerDlg dlg(module);
	            m_pMainWnd = &dlg;
	            INT_PTR nResponse = dlg.DoModal();
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

	            // Delete the shell manager created above.
	            if (pShellManager != NULL)
	            {
		            delete pShellManager;
	            }
            }
        }
    }
    FreeConsole();
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}


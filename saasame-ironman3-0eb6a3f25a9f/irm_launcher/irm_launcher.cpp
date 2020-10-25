// irm_host_agent.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "boost\program_options.hpp"
#include "boost\program_options\parsers.hpp"
#include "boost\thread.hpp"
#include "boost\date_time.hpp"
#include <io.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <stdexcept>
#include <sstream>
#ifndef _WINIOCTL
#define _WINIOCTL
#include <winioctl.h>
#endif
#include <VersionHelpers.h>
#include "launcher_service_handler.h"
#ifdef _VMWARE_VADP
#include "vmware.h"
#include "vmware_ex.h"
#include "difxapi.h"
#pragma comment( lib, "difxapi.lib" )
#pragma comment(lib, "irm_hypervisor_ex.lib")
using namespace mwdc::ironman::hypervisor_ex;
#endif
using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;
using namespace boost;
std::shared_ptr<TSSLSocketFactory> g_factory;
std::shared_ptr<TThreadedServer>  g_server;
std::shared_ptr<TThreadedServer>  g_proxy_server;

#ifdef _VMWARE_VADP
std::wstring get_error_message(DWORD message_id){
    std::wstring str;
    switch (message_id){
    case ERROR_TIMEOUT:
        str = L"This operation returned because the timeout period expired.";
        break;
    case CERT_E_EXPIRED:
        str = L"The signing certificate is expired.";
        break;
    case CERT_E_UNTRUSTEDROOT:
        str = L"The catalog file has an Authenticode signature whose certificate chain terminates in a root certificate that is not trusted.";
        break;
    case CERT_E_WRONG_USAGE:
        str = L"The certificate for the driver package is not valid for the requested usage. If the driver package does not have a valid WHQL signature, DriverPackageInstall returns this error if, in response to a driver signing dialog box, the user chose not to install a driver package, or if the caller specified the DRIVER_PACKAGE_SILENT flag.";
        break;
    case CRYPT_E_FILE_ERROR:
        str = L"The catalog file for the specified driver package was not found; or possibly, some other error occurred when DriverPackageInstall tried to verify the driver package signature.";
        break;
    case ERROR_ACCESS_DENIED:
        str = L"A caller of DriverPackageInstall must be a member of the Administrators group to install a driver package.";
        break;
    case ERROR_BAD_ENVIRONMENT:
        str = L"The current Microsoft Windows version does not support this operation. An old or incompatible version of DIFxApp.dll or DIFxAppA.dll might be present in the system. For more information about these .dll files, see How DIFxApp Works.";
        break;
    case ERROR_CANT_ACCESS_FILE:
        str = L"DriverPackageInstall could not preinstall the driver package because the specified INF file is in the system INF file directory.";
        break;
    case ERROR_FILE_NOT_FOUND:
        str = L"The INF file that was specified by DriverPackageInfPath was not found.";
        break;
    case ERROR_FILENAME_EXCED_RANGE:
        str = L"The INF file path, in characters, that was specified by DriverPackageInfPath is greater than the maximum supported path length. For more information about path length, see Specifying a Driver Package INF File Path.";
        break;
#ifdef ERROR_IN_WOW64
    case ERROR_IN_WOW64:
        str = L"The 32-bit version DIFxAPI does not work on Win64 systems. A 64-bit version of DIFxAPI is required.";
        break;
#endif
    case ERROR_INSTALL_FAILURE:
        str = L"The installation failed.";
        break;
    case ERROR_INVALID_CATALOG_DATA:
        str = L"The catalog file for the specified driver package is not valid or was not found.";
        break;
    case ERROR_INVALID_NAME:
        str = L"The specified INF file path is not valid.";
        break;
    case ERROR_INVALID_PARAMETER:
        str = L"A supplied parameter is not valid.";
        break;
    case ERROR_NO_DEVICE_ID:
        str = L"The driver package does not specify a hardware identifier or compatible identifier that is supported by the current platform.";
        break;
    case ERROR_NO_MORE_ITEMS:
        str = L"The specified driver package was not installed for matching devices because the driver packages already installed for the matching devices are a better match for the devices than the specified driver package.";
        break;
    case ERROR_NO_SUCH_DEVINST:
        str = L"The driver package was not installed on any device because there are no matching devices in the device tree.";
        break;
    case ERROR_OUTOFMEMORY:
        str = L"Available system memory was insufficient to perform the operation.";
        break;
    case ERROR_SHARING_VIOLATION:
        str = L"A component of the driver package in the DIFx driver store is locked by a thread or process. This error can occur if a process or thread, other than the thread or process of the caller, is currently accessing the same driver package as the caller.";
        break;
    case ERROR_NO_CATALOG_FOR_OEM_INF:
        str = L"The catalog file for the specified driver package was not found.";
        break;
    case ERROR_AUTHENTICODE_PUBLISHER_NOT_TRUSTED:
        str = L"The publisher of an Authenticode(tm) signed catalog was not established as trusted.";
        break;
    case ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH:
        str = L"The signing certificate is not valid for the current Windows version or it is expired.";
        break;
    case ERROR_UNSUPPORTED_TYPE:
        str = L"The driver package type is not supported.";
        break;
    case TRUST_E_NOSIGNATURE:
        str = L"The driver package is not signed.";
        break;
    case ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED:
        str = L"The publisher of an Authenticode(tm) signed catalog has not yet been established as trusted.";
        break;
    }
    return str;
}

#endif

bool command_line_parser(po::variables_map &vm){

    bool result = false;
    std::string title;
#ifndef _CONSOLE
    title = boost::str(boost::format("%s\n") % GetCommandLineA());
#endif
    //title += boost::str(boost::format("\r\n------------ %s, Version: %d.%d Build: %d ------------\r\n\r\n") % PRODUCT_NAME_STR %PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD);
    po::options_description config("Configation");
    config.add_options()
        ("session,s", po::wvalue<std::wstring>()->default_value(L"{D1932ED2-2A5C-46A4-B0BB-A74465410CCD}", "{D1932ED2-2A5C-46A4-B0BB-A74465410CCD}"), "Session id")
        ;
    po::options_description general("General");
    general.add_options()
        ("level,l", po::value<int>()->default_value(2, "2"), "log level ( 0 ~ 5 )")
        ("help,h", "produce help message (option)");
    ;
    po::options_description install("service options");
    boost::filesystem::path execution_file = macho::windows::environment::get_execution_full_path();
    install.add_options()
        (",i", "install service")
        (",u", "unistall service")
        ("restart,r", "restart service")
#ifdef _VMWARE_VADP
        ("remove", "uninstall VMP driver")
        ("add", "install VMP driver")
#endif
        ("name", po::wvalue<std::wstring>()->default_value(execution_file.filename().stem().wstring(), execution_file.filename().stem().string()), "service name")
        ("display", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DISPLAY_NAME), saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DISPLAY_NAME), "service display name (optional, installation only)")
        ("description", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DESCRIPTION), saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DESCRIPTION), "service description (optional, installation only)")
        ;

    po::options_description all("Allowed options");
    all.add(general).add(config).add(install);

    try{
        std::wstring c = GetCommandLine();
#if _UNICODE
        po::store(po::wcommand_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#else
        po::store(po::command_line_parser(po::split_winmain(GetCommandLine())).options(all).run(), vm);
#endif
        po::notify(vm);
        int max_debug_level = 5;
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MaxDebugLevel"].exists() && reg[L"MaxDebugLevel"].is_dword() && (DWORD)reg[L"MaxDebugLevel"] > 5)
                max_debug_level = (DWORD)reg[L"MaxDebugLevel"];
        }
        if (vm.count("help") || (vm["level"].as<int>() > max_debug_level) || (vm["level"].as<int>() < 0)){
            std::cout << title << all << std::endl;
        }
        else {
            if (vm.count("session") || vm.count("-i") || vm.count("-u") || vm.count("restart") 
#ifdef _VMWARE_VADP
                || vm.count("remove")
                || vm.count("add")
#endif
                )
                result = true;
            else
                std::cout << title << all << std::endl;
        }
    }
    catch (const boost::program_options::multiple_occurrences& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << " from option: " << e.get_option_name() << std::endl;
    }
    catch (const boost::program_options::error& e) {
        std::cout << title << all << "\n";
        std::cout << e.what() << std::endl;
    }
    catch (boost::exception &e){
        std::cout << title << all << "\n";
        std::cout << boost::exception_detail::get_diagnostic_information(e, "Invalid command parameter format.") << std::endl;
    }
    catch (...){
        std::cout << title << all << "\n";
        std::cout << "Invalid command parameter format." << std::endl;
    }
    return result;
}

inline application::global_context_ptr this_application() {
    return application::global_context::get();
}

class main_app
{
public:
    main_app() {}

    void worker()
    {
        // application behaviour
        // dump args
        const std::vector<std::wstring> &arg_vector =
            this_application()->find<application::args>()->arg_vector();

        LOG(LOG_LEVEL_RECORD, _T("---------- Arg List ---------"));

        // only print args on screen
        for (std::vector<std::wstring>::const_iterator it = arg_vector.begin();
            it != arg_vector.end(); ++it) {
            LOG(LOG_LEVEL_RECORD, L"%s", (*it).c_str());
        }

        LOG(LOG_LEVEL_RECORD, _T("-----------------------------"));

        // define our simple installation schema options
        po::options_description loglevel("log level options");
        loglevel.add_options()
            ("session,s", po::wvalue<std::wstring>()->default_value(L"{D1932ED2-2A5C-46A4-B0BB-A74465410CCD}", "{D1932ED2-2A5C-46A4-B0BB-A74465410CCD}"), "Session id")
            ("level,l", po::value<int>(), "Change log level")
            ;
        po::variables_map vm;
        po::store(po::parse_command_line(this_application()->find<application::args>()->argc(), this_application()->find<application::args>()->argv(), loglevel), vm);
        boost::system::error_code ec;

        if (vm.count("level"))
            set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());

        std::shared_ptr<application::status> st =
            this_application()->find<application::status>();
        int port = saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT;
        _session = vm["session"].as<std::wstring>();
        _handler = std::shared_ptr<launcher_service_handler>(new launcher_service_handler(vm["session"].as<std::wstring>()));
        
        std::shared_ptr<TProcessor> processor(new launcher_serviceProcessor(_handler));
        std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
        std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
        std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

        std::string session, mgmt_addr, machine_id;
        bool        allow_multiple = true;
        std::string computer_name = macho::stringutils::convert_unicode_to_utf8(macho::windows::environment::get_computer_name());
        macho::windows::registry reg;
        boost::filesystem::path p(macho::windows::environment::get_working_directory());
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                p = reg[L"KeyPath"].wstring();
            if (reg[L"SessionId"].exists())
                session = reg[L"SessionId"].string();
            if (reg[L"AllowMultiple"].exists() && reg[L"AllowMultiple"].is_dword() && (DWORD)reg[L"AllowMultiple"] == 0 )
                allow_multiple = false ;
            if (reg[L"Machine"].exists())
                machine_id = reg[L"Machine"].string();
            if (reg[L"MgmtAddr"].exists())
                mgmt_addr = reg[L"MgmtAddr"].string();
        }

        if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
        {
            try
            {
                g_factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                g_factory->server(true);
                g_factory->authenticate(true);
                g_factory->loadCertificate((p / "server.crt").string().c_str());
                g_factory->loadPrivateKey((p / "server.key").string().c_str());
                g_factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                if (session.length() && machine_id.length() && mgmt_addr.length() && (!allow_multiple || macho::windows::environment::is_winpe())){
                    std::shared_ptr<TServerTransport> ssl_serverTransport(new TSSLServerSocket("localhost", port, g_factory));
                    g_server = std::shared_ptr<TThreadedServer>(new TThreadedServer(processor, ssl_serverTransport, transportFactory, protocolFactory));
                    LOG(LOG_LEVEL_RECORD, L"Disable listen port from \'*\'.");
                }
                else{
                    std::shared_ptr<TServerTransport> ssl_serverTransport(new TSSLServerSocket(port, g_factory));
                    g_server = std::shared_ptr<TThreadedServer>(new TThreadedServer(processor, ssl_serverTransport, transportFactory, protocolFactory));
                }
                LOG(LOG_LEVEL_RECORD, L"SSL link enabled.");
            }
            catch (TException& ex)
            {
                g_server = nullptr;
                std::string errmsg(ex.what());
                LOG(LOG_LEVEL_ERROR, L"Enable SSL link failure: %s.", macho::stringutils::convert_utf8_to_unicode(errmsg).c_str());
            }
        }

        if (!g_server){
            if (session.length() && machine_id.length() && mgmt_addr.length() && (!allow_multiple || macho::windows::environment::is_winpe())){
                serverTransport = std::shared_ptr<TServerTransport>(new TServerSocket("localhost", port));
                LOG(LOG_LEVEL_RECORD, L"Disable listen port from \'*\'.");
            }
            g_server = std::shared_ptr<TThreadedServer>(new TThreadedServer(processor,
                serverTransport,
                transportFactory,
                protocolFactory));
        }

        if (session.length() && machine_id.length() && mgmt_addr.length()){
            std::shared_ptr<TServerTransport> proxyTransport(new TServerReverseSocket(mgmt_addr, session, machine_id, computer_name));
            g_proxy_server = std::shared_ptr<TThreadedServer>(new TThreadedServer(processor,
                proxyTransport,
                transportFactory,
                protocolFactory));
        }

        std::cout << "Starting the server..." << std::endl;
        if (g_server){
            if (g_proxy_server){
                LOG(LOG_LEVEL_RECORD, L"Running in pulling mode.");
                boost::thread_group tg;
                tg.create_thread(boost::bind(&TThreadedServer::serve, g_server));
                tg.create_thread(boost::bind(&TThreadedServer::serve, g_proxy_server));
                tg.join_all();
            }
            else{
                g_server->serve();
            }
            g_proxy_server = NULL;
            g_server = NULL;
        }
        std::cout << "Done." << std::endl;

        /* while (st->state() != application::status::stopped)
        {
        if (st->state() == application::status::paused)
        LOG(LOG_LEVEL_WARNING, _T("paused..."));
        else{
        LOG(LOG_LEVEL_TRACE, _T("running..."));
        }
        boost::this_thread::sleep(boost::posix_time::seconds(1));
        }*/
    }

    // param
    int operator()()
    {
        // launch a work thread
        boost::thread thread(&main_app::worker, this);

        this_application()->find<application::wait_for_termination_request>()->wait();

        return 0;
    }

    // windows/posix

    bool stop()
    {
        LOG(LOG_LEVEL_RECORD, _T("Stopping application..."));
        if (_handler)
            _handler->terminate((std::string)_session);
        return true; // return true to stop, false to ignore
    }

    // windows specific (ignored on posix)

    bool pause()
    {
        LOG(LOG_LEVEL_RECORD, _T("Pause application..."));
        return true; // return true to pause, false to ignore
    }

    bool resume()
    {
        LOG(LOG_LEVEL_RECORD, _T("Resume application..."));
        return true; // return true to resume, false to ignore
    }

private:
    std::shared_ptr<launcher_service_handler> _handler;
    macho::guid_                          _session;
};

bool setup(application::context& context)
{
    strict_lock<application::aspect_map> guard(context);

    std::shared_ptr<application::args> myargs
        = context.find<application::args>(guard);

    std::shared_ptr<application::path> mypath
        = context.find<application::path>(guard);

    // provide setup for windows service
#if defined(BOOST_WINDOWS_API)
#if !defined(__MINGW32__)

    // get our executable path name
    boost::filesystem::path executable_path_name = mypath->executable_path_name();

    // define our simple installation schema options
    po::options_description install("service options");
    install.add_options()
        ("help", "produce a help message")
        (",i", "install service")
        (",u", "unistall service")
        ("name", po::wvalue<std::wstring>()->default_value(mypath->executable_name().stem().wstring(), mypath->executable_name().stem().string()), "service name")
        ("display", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DISPLAY_NAME), saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DISPLAY_NAME), "service display name (optional, installation only)")
        ("description", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DESCRIPTION), saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_DESCRIPTION), "service description (optional, installation only)")
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(myargs->argc(), myargs->argv(), install), vm);
    boost::system::error_code ec;

    if (vm.count("help"))
    {
        std::cout << install << std::endl;
        return true;
    }

    if (vm.count("-i"))
    {
        application::example::install_windows_service(
            application::setup_arg(vm["name"].as<std::wstring>()),
            application::setup_arg(vm["display"].as<std::wstring>()),
            application::setup_arg(vm["description"].as<std::wstring>()),
            application::setup_arg(executable_path_name)).install(ec);

        std::cout << ec.message() << std::endl;

        return true;
    }

    if (vm.count("-u"))
    {
        application::example::uninstall_windows_service(
            application::setup_arg(vm["name"].as<std::wstring>()),
            application::setup_arg(executable_path_name)).uninstall(ec);

        std::cout << ec.message() << std::endl;

        return true;
    }

#endif
#endif

    return false;
}

int _tmain(int argc, _TCHAR* argv[]){
    if (!(macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_BACKUP_NAME). ");
    }
    if (!(macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, true))){
        LOG(LOG_LEVEL_ERROR, L"Failed to set token privilege (SE_RESTORE_NAME). ");
    }
    po::variables_map vm;
    boost::filesystem::path logfile;
    boost::filesystem::path app_path = macho::windows::environment::get_execution_full_path();
    SetDllDirectory(app_path.parent_path().parent_path().wstring().c_str());
    logfile = app_path.parent_path().string() + "/logs/" + app_path.filename().stem().string() + ".log";
    boost::filesystem::create_directories(logfile.parent_path());
    macho::set_log_file(logfile.wstring());
    if (command_line_parser(vm)){
        macho::windows::com_init _com;
        set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());
#ifdef _VMWARE_VADP
        if (vm.count("remove")){
            boost::filesystem::path repository = boost::filesystem::path(macho::windows::environment::get_system_directory()) / L"DriverStore" / L"FileRepository";
            std::vector<boost::filesystem::path> files = macho::windows::environment::get_files(repository, L"vmp.inf", true);
            for each (auto file in files){
                DWORD Flags = DRIVER_PACKAGE_DELETE_FILES | DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_FORCE;
                std::wstring  driver_package_inf_path = file.wstring();
                LPTSTR _driver_package_inf_path = (LPTSTR)driver_package_inf_path.c_str();  // An INF file for PnP driver package
                INSTALLERINFO *pAppInfo = NULL;      // No application association
                BOOL NeedReboot = FALSE;
                DWORD ReturnCode = DriverPackageUninstall(_driver_package_inf_path, Flags, pAppInfo, &NeedReboot);
                if (ERROR_SUCCESS != ReturnCode){
                    std::wstring str = get_error_message(ReturnCode);
                    LOG(LOG_LEVEL_WARNING, _T("Uninstall Driver (%s) - Result (0x%08X - %s)\n"), _driver_package_inf_path, ReturnCode, str.c_str());
                }
            }
            macho::windows::device_manager  dev_mgmt;
            hardware_device::vtr class_devices;
            class_devices = dev_mgmt.get_devices();
            foreach(hardware_device::ptr &d, class_devices){
                foreach(stdstring id, d->hardware_ids){
                    if (id == L"root\\vmp"){
                        dev_mgmt.remove_device(d->device_instance_id);
                        break;
                    }
                }
            }
            return 0;
        }
        else if (vm.count("add")){
            std::wstring device_path;
            std::wstring  driver_package_inf_path = boost::filesystem::path(boost::filesystem::path(macho::windows::environment::get_execution_full_path()).parent_path() / L"DRIVER" / L"vmp.inf").wstring();
            macho::windows::device_manager  dev_mgmt;
            hardware_device::vtr class_devices;
            class_devices = dev_mgmt.get_devices(L"SCSIAdapter");
            foreach(hardware_device::ptr &d, class_devices){
                if (d->service == L"vmp"){
                    device_path = dev_mgmt.get_device_path(d->device_instance_id, (LPGUID)&GUID_DEVINTERFACE_STORAGEPORT);
                    LOG(LOG_LEVEL_INFO, L"Device Path : %s", device_path.c_str());
                    break;
                }
            }
            LPTSTR _driver_package_inf_path = (LPTSTR)driver_package_inf_path.c_str();  // An INF file for PnP driver package
            if (!device_path.empty()){
                DWORD Flags = DRIVER_PACKAGE_LEGACY_MODE | DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_FORCE;
                INSTALLERINFO *pAppInfo = NULL;      // No application association
                BOOL NeedReboot = FALSE;
                if (boost::filesystem::exists(_driver_package_inf_path)){
                    DWORD ReturnCode = DriverPackageInstall(_driver_package_inf_path, Flags, pAppInfo, &NeedReboot);
                    if (ERROR_SUCCESS != ReturnCode){
                        std::wstring str = get_error_message(ReturnCode);
                        LOG(LOG_LEVEL_WARNING, _T("Install Driver (%s) - Result (0x%08X - %s)\n"), _driver_package_inf_path, ReturnCode, str.c_str());
                    }
                }
            }
            else{
                int retry = 1;
                do{
                    DWORD ReturnCode = DriverPackagePreinstall(_driver_package_inf_path, DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_LEGACY_MODE);
                    if (ERROR_SUCCESS != ReturnCode){
                        if (ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED == ReturnCode){
                            certificate_store::ptr trusted_publisher = certificate_store::open_store(CERT_SYSTEM_STORE_LOCAL_MACHINE, L"TrustedPublisher");
                            if (trusted_publisher){
                                setup_inf_file inf;
                                inf.load(driver_package_inf_path);
                                boost::filesystem::path inf_file = driver_package_inf_path;
                                boost::filesystem::path catalog_file = inf_file.parent_path() / inf.catalog_file();
                                authenticode_signed_info::ptr signed_info = certificate_store::get_authenticode_signed_info(catalog_file);
                                if (signed_info && signed_info->signer_certificate){
                                    std::wstring cert_name = signed_info->signer_certificate->friendly_name();
                                    if (trusted_publisher->add_certificate(*signed_info->signer_certificate)){
                                        LOG(LOG_LEVEL_RECORD, _T("Add certificate(%s) into Trusted Publisher."), cert_name.c_str());
                                        continue;
                                    }
                                }
                            }
                        }
                        std::wstring str = get_error_message(ReturnCode);
                        LOG(LOG_LEVEL_ERROR, _T("PreInstall Driver (%s) - Result (0x%08X - %s)\n"), driver_package_inf_path, ReturnCode, str.c_str());
                    }
                    dev_mgmt.install(driver_package_inf_path, L"root\\vmp");
                } while (retry--);
            }
            return 0;
        }
#endif
        if (vm.count("restart")){
            try{
                boost::this_thread::sleep(boost::posix_time::seconds(1));
                macho::windows::service_table services = macho::windows::service::get_all_services();
                foreach(macho::windows::service& sc, services){
                    std::vector<std::wstring> p = macho::stringutils::tokenize2(sc.path_name(), L"\"", 0, false);
                    if (p.size() && 0 == _wcsnicmp(app_path.wstring().c_str(), p[0].c_str(), app_path.wstring().length())){
                        sc.stop();
                        sc.start();
                        LOG(LOG_LEVEL_RECORD, _T("Starting the service %s."), sc.name().c_str());
                        return 0;
                    }
                }
            }
            catch (macho::windows::service_exception &ex){
            }
            catch (...){
            }
            return 1;
        }
        macho::windows::mutex running(app_path.filename().wstring());
        if (!running.trylock()){
            std::wstring out = boost::str(boost::wformat(L"The application (%s) is already running.") % app_path.filename().wstring());
            LOG(LOG_LEVEL_RECORD, out.c_str());
            std::wcout << out << std::endl;
        }
        else{

            WSADATA wsaData;
            int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (iResult != 0) {
                printf("WSAStartup failed: %d\n", iResult);
                return 1;
            }
            application::global_context_ptr ctx = application::global_context::create();

            // auto_handler will automatically add termination, pause and resume (windows) handlers
            application::auto_handler<main_app> app(ctx);
            // application::detail::handler_auto_set<myapp> app(app_context);

            // my server aspects

            this_application()->insert<application::path>(
                std::make_shared<application::path_default_behaviour>(argc, argv));

            this_application()->insert<application::args>(
                std::make_shared<application::args>(argc, argv));

            try{
                // check if we need setup
                if (setup(*ctx.get())){
                    std::cout << "[I] Setup changed the current configuration." << std::endl;
                    application::global_context::destroy();
                    return 0;
                }
            }
            catch (boost::program_options::unknown_option & e){
            }

            auto_lock lock(running);

            bool is_service_mode = false;
            try{
                macho::windows::service_table services = macho::windows::service::get_all_services();
                foreach(macho::windows::service& sc, services){
                    std::vector<std::wstring> p = macho::stringutils::tokenize2(sc.path_name(), L"\"", 0, false);
                    if (p.size() && 0 == _wcsnicmp(app_path.wstring().c_str(), p[0].c_str(), app_path.wstring().length())){
                        is_service_mode = true;
                        if (!macho::windows::environment::is_running_as_local_system()){
                            sc.start();
                            LOG(LOG_LEVEL_RECORD, _T("Starting the service %s."), sc.name().c_str());
                            return 0;
                        }
                        break;
                    }
                }
            }
            catch (macho::windows::service_exception &ex){
            }
            catch (...){
            }
#ifdef _VMWARE_VADP
            if (!vmware_portal_ex::vixdisk_init()){
                LOG(LOG_LEVEL_ERROR, _T("Failed to init vixdisk library."));
            }
            else if (is_service_mode && macho::windows::environment::is_running_as_local_system()){
#else
            if (is_service_mode && macho::windows::environment::is_running_as_local_system()){
#endif
                // my server instantiation
                LOG(LOG_LEVEL_RECORD, _T("It is running on service mode."));
                boost::system::error_code ec;
                int result = application::launch<application::server>(app, ctx, ec);
                if (ec){
                    std::cout << "[E] " << ec.message()
                        << " <" << ec.value() << "> " << std::endl;
                }
            }
            else{
                LOG(LOG_LEVEL_RECORD, _T("It is not running on service mode."));
#ifndef _DEBUG
                if (macho::windows::environment::is_winpe())
#endif
                {
                    int port = saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT;
                    std::shared_ptr<launcher_service_handler> handler(new launcher_service_handler(vm["session"].as<std::wstring>()));
                    std::shared_ptr<TProcessor> processor(new launcher_serviceProcessor(handler));
                    std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
                    std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
                    std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
                    macho::windows::registry reg;
                    boost::filesystem::path p(macho::windows::environment::get_working_directory());
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                            p = reg[L"KeyPath"].wstring();
                    }
                    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
                    {
                        try
                        {
                            g_factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
                            g_factory->server(true);
                            g_factory->authenticate(true);
                            g_factory->loadCertificate((p / "server.crt").string().c_str());
                            g_factory->loadPrivateKey((p / "server.key").string().c_str());
                            g_factory->loadTrustedCertificates((p / "server.crt").string().c_str());
                            std::shared_ptr<TServerTransport> ssl_serverTransport(new TSSLServerSocket(port, g_factory));
                            g_server = std::shared_ptr<TThreadedServer>(new TThreadedServer(processor, ssl_serverTransport, transportFactory, protocolFactory));

                            LOG(LOG_LEVEL_RECORD, L"SSL link enabled.");
                        }
                        catch (TException& ex)
                        {
                            g_server = nullptr;
                            std::string errmsg(ex.what());
                            LOG(LOG_LEVEL_ERROR, L"Enable SSL link failure: %s.", macho::stringutils::convert_utf8_to_unicode(errmsg).c_str());
                        }
                    }

                    if (!g_server){
                        g_server = std::shared_ptr<TThreadedServer>(new TThreadedServer(processor,
                            serverTransport,
                            transportFactory,
                            protocolFactory));
                    }

                    std::cout << "Starting the server..." << std::endl;
                    g_server->serve();
                    g_server = NULL;
                    std::cout << "Done." << std::endl;
                }
            }
            application::global_context::destroy();
        }
    }
    g_factory = NULL;
    return 0;
}

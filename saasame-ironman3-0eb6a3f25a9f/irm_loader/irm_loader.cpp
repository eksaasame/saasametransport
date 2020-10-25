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
#include "loader_service_handler.h"
#include "..\gen-cpp\webdav.h"
#ifdef _VMWARE_VADP
#include "vmware.h"
#include "vmware_ex.h"
#pragma comment(lib, "irm_hypervisor_ex.lib")
using namespace mwdc::ironman::hypervisor_ex;
#endif
using namespace macho;
using namespace macho::windows;
namespace po = boost::program_options;
using namespace boost;

std::shared_ptr<TSSLSocketFactory> g_factory;
std::shared_ptr<TThreadedServer>  g_server;

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
        ("name", po::wvalue<std::wstring>()->default_value(execution_file.filename().stem().wstring(), execution_file.filename().stem().string()), "service name")
        ("display", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LOADER_SERVICE_DISPLAY_NAME), saasame::transport::g_saasame_constants.LOADER_SERVICE_DISPLAY_NAME), "service display name (optional, installation only)")
        ("description", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LOADER_SERVICE_DESCRIPTION), saasame::transport::g_saasame_constants.LOADER_SERVICE_DESCRIPTION), "service description (optional, installation only)")
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
            if (vm.count("session") || vm.count("-i") || vm.count("-u"))
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
        int port = saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT;
        _session = vm["session"].as<std::wstring>();
        registry reg;
        int workers = boost::thread::hardware_concurrency();
        int delay = 0;
        boost::filesystem::path p(macho::windows::environment::get_working_directory());
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            workers = (int)reg[L"ConcurrentLoaderJobNumber"];
            delay = (int)reg[L"Delay"];

            if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
                p = reg[L"KeyPath"].wstring();
            reg.close();
        }
        if (delay)
            boost::this_thread::sleep(boost::posix_time::seconds(delay));
        _handler = std::shared_ptr<loader_service_handler>(new loader_service_handler(vm["session"].as<std::wstring>(), (workers ? workers * 2 : boost::thread::hardware_concurrency() * 2) + 1));
        std::shared_ptr<TProcessor> processor(new loader_serviceProcessor(_handler));
        std::shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
        std::shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
        std::shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());
       
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

        try{
            macho::windows::com_init _com;
            wmi_object inputOpenStore;
            wmi_object outputOpenStore;
            DWORD dwReturn;
            wmi_services wmi_defender;
            HRESULT hr = wmi_defender.connect(L"Microsoft\\Windows\\Defender");
            if (!FAILED(hr)){
                hr = wmi_defender.get_input_parameters(L"MSFT_MpPreference", L"Add", inputOpenStore);
                string_table_w exclusion_process;
                exclusion_process.push_back(macho::windows::environment::get_execution_full_path());
                inputOpenStore.set_parameter(L"ExclusionProcess", exclusion_process);
                inputOpenStore.set_parameter(L"Force", true);
                hr = wmi_defender.exec_method(L"MSFT_MpPreference", L"Add", inputOpenStore, outputOpenStore, dwReturn);
            }
        }
        catch (...){}

        std::cout << "Starting the server..." << std::endl;
        g_server->serve();
        g_server = NULL;
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
    std::shared_ptr<loader_service_handler> _handler;
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
        ("display", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LOADER_SERVICE_DISPLAY_NAME), saasame::transport::g_saasame_constants.LOADER_SERVICE_DISPLAY_NAME), "service display name (optional, installation only)")
        ("description", po::wvalue<std::wstring>()->default_value(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.LOADER_SERVICE_DESCRIPTION), saasame::transport::g_saasame_constants.LOADER_SERVICE_DESCRIPTION), "service description (optional, installation only)")
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

    po::variables_map vm;
    boost::filesystem::path logfile;
    boost::filesystem::path app_path = macho::windows::environment::get_execution_full_path();
    SetDllDirectory(app_path.parent_path().parent_path().wstring().c_str());
    logfile = app_path.parent_path().string() + "/logs/" + app_path.filename().stem().string() + ".log";
    boost::filesystem::create_directories(logfile.parent_path());
    macho::set_log_file(logfile.wstring());
    if (command_line_parser(vm)){
        set_log_level((TRACE_LOG_LEVEL)vm["level"].as<int>());
        macho::windows::mutex running(app_path.filename().wstring());
        if (!running.trylock()){
            std::wstring out = boost::str(boost::wformat(L"The application (%s) is already running.") % app_path.filename().wstring());
            LOG(LOG_LEVEL_RECORD, out.c_str());
            std::wcout << out << std::endl;
        }
        else{

            //WSADATA wsaData;
            //int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
            //if (iResult != 0) {
            //    printf("WSAStartup failed: %d\n", iResult);
            //    return 1;
            //}
            webdav::initialize();
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

            macho::windows::com_init _com;
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
                    int port = saasame::transport::g_saasame_constants.LOADER_SERVICE_PORT;
                    std::shared_ptr<loader_service_handler> handler(new loader_service_handler(vm["session"].as<std::wstring>()));
                    std::shared_ptr<TProcessor> processor(new loader_serviceProcessor(handler));
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
            webdav::uninitialize();
        }
    }
    g_factory = NULL;
    return 0;
}
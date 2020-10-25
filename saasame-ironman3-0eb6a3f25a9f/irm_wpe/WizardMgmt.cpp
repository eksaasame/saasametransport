// WizardMgmt.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wpe.h"
#include "WizardMgmt.h"
#include "afxdialogex.h"
#include <VersionHelpers.h>
#include <regex>
#include "..\gen-cpp\service_op.h"
#include "..\gen-cpp\mgmt_op.h"
// CWizardMgmt dialog

IMPLEMENT_DYNAMIC(CWizardMgmt, CPropertyPage)

CWizardMgmt::CWizardMgmt(winpe_settings & settings, SheetPos posPositionOnSheet)
: CWizardPage(posPositionOnSheet, CWizardMgmt::IDD), _settings(settings)
{
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    std::shared_ptr<TTransport> transport;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    _factory = std::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
    _factory->authenticate(false);
    _factory->loadCertificate((p / "server.crt").string().c_str());
    _factory->loadPrivateKey((p / "server.key").string().c_str());
    _factory->loadTrustedCertificates((p / "server.crt").string().c_str());
    std::shared_ptr<AccessManager> accessManager(new MyAccessManager());
    _factory->access(accessManager);
}

CWizardMgmt::~CWizardMgmt()
{
}

void CWizardMgmt::DoDataExchange(CDataExchange* pDX)
{
    CWizardPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_MGMT, _mgmt_addr);
    DDX_Control(pDX, IDC_CHK_SSL, _is_https);
    DDX_Control(pDX, IDC_EDIT_USERNAME, _username);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, _password);
    DDX_Control(pDX, IDC_SYSLINK_RESET, _ClearBtn);
    DDX_Control(pDX, IDC_CHK_KEEP, _keep_settings);
    DDX_Control(pDX, IDC_EDIT_HOSTNAME, _hostname);
}

BEGIN_MESSAGE_MAP(CWizardMgmt, CWizardPage)
    ON_NOTIFY(NM_CLICK, IDC_SYSLINK_RESET, &CWizardMgmt::OnClickSyslinkReset)
END_MESSAGE_MAP()

// CWizardMgmt message handlers
BOOL CWizardMgmt::OnWizardFinish()
{
    CString hostname;
    _hostname.GetWindowTextW(hostname);
    bool result = false;
    if (result = std::regex_match(hostname.GetString(), std::wregex(L"[a-zA-Z][a-zA-Z0-9-]{0,14}"))) {
        _settings.host_name = hostname.GetString();
        registry reg(REGISTRY_CREATE);
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            reg[_T("HostName")] = _settings.host_name;
        }
    }
    else if (hostname.GetLength() == 0){
        AfxMessageBox(L"The host name is required.");
        return FALSE;
    }
    else if (hostname.GetLength() > 15){
        AfxMessageBox(L"The host name is limited to 15 characters.");
        return FALSE;
    }
    else if (std::regex_match(hostname.GetString(), std::wregex(L"[0-9-][a-zA-Z0-9-]{0,14}"))){
        CString message;
        message.Format(L"The first character of new host name \"%s\" is nonvalid.", hostname.GetString());
        AfxMessageBox(message);
        return FALSE;
    }
    else {
        CString message;
        message.Format(L"The new host name \"%s\" contains characters that are not allowed. Characters that are not allowed include ' ~ ! @ # $ %% ^ & * ( ) = + _ [ ] { } \\ | ; : . ' \" ,  < > / and ? ", hostname.GetString());
        AfxMessageBox(message);
        return FALSE;
    }

    BeginWaitCursor(); //SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
    std::wstring ret;
    if (macho::windows::environment::is_winpe()){
        boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
        std::wstring loader = boost::filesystem::path(work_dir / L"irm_loader.exe").wstring();
        std::wstring launcher = boost::filesystem::path(work_dir / L"irm_launcher.exe").wstring();
        std::wstring host_packer = boost::filesystem::path(work_dir / L"irm_host_packer.exe").wstring();
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
    }
    BOOL register_result = TRUE;
    if (_session.empty() && _addr.empty()){
        register_result = RegisterServices();
    }
    else{
        std::set<service_info> service_infos;
        physical_machine_info MachineInfo;
        GetPhysicalMachineInfo(MachineInfo, service_infos,
            is_transport() ? saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT : saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT);
    }
    EndWaitCursor(); //RestoreWaitCursor();
    if (register_result){
        _settings.is_keep = TRUE == _keep_settings.GetCheck();
        CString mgmt_addr;
        _mgmt_addr.GetWindowTextW(mgmt_addr);
        if (mgmt_addr.IsEmpty()){
            process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s\\wpeutil.exe DisableFirewall") % macho::windows::environment::get_system_directory()), ret, -1, true);
        }
        registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            _settings.client_id = reg[_T("Client")].wstring();
            _settings.machine_id = reg[_T("Machine")].wstring();
            bytes digital_id = reg[_T("DigitalId")];
            _settings.digital_id = digital_id.get();
            if (reg[_T("SessionId")].exists())
                _settings.session_id = reg[_T("SessionId")].wstring();
            if (reg[_T("MgmtAddr")].exists())
                _settings.mgmt_addr = reg[_T("MgmtAddr")].wstring();
        }
        return CWizardPage::OnWizardFinish();
    }
    else{
        return FALSE;
    }
}

bool CWizardMgmt::is_transport(){
    try{
        macho::windows::service irm_launcher = macho::windows::service::get_service(L"irm_launcher");
        return true;
    }
    catch (...){
    }
    return false;
}

BOOL CWizardMgmt::RegisterServices()
{
    BOOL Ret = FALSE;
    std::set<service_info> service_infos;
    physical_machine_info MachineInfo;
    CString mgmt_addr;
    _mgmt_addr.GetWindowTextW(mgmt_addr);
    if (mgmt_addr.IsEmpty()){
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            reg[L"SessionId"].delete_value();
            reg[L"MgmtAddr"].delete_value();
            reg[L"AllowMultiple"].delete_value();
        }
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
        return TRUE;
    }
    std::string addr = macho::stringutils::convert_unicode_to_utf8(mgmt_addr.GetString());
    LOG(LOG_LEVEL_RECORD, L"mgmt_addr: %s", macho::stringutils::convert_ansi_to_unicode(addr).c_str());
    try{
        if (GetPhysicalMachineInfo(MachineInfo, service_infos,
            is_transport() ? saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT : saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT)){
            _machine_id = MachineInfo.machine_id;
#if 0
            std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;
            std::shared_ptr<saasame::transport::management_serviceClient>    client;
            std::shared_ptr<TTransport>                                      transport;
            if (TRUE == _is_https.GetCheck()){
                std::shared_ptr<TCurlClient>           http = std::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % addr%uri)));
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(http));
                std::shared_ptr<TProtocol>             protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = std::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            else{
                std::shared_ptr<THttpClient>           http = std::shared_ptr<THttpClient>(new THttpClient(addr, 80, uri));
                transport = std::shared_ptr<TTransport>(new TBufferedTransport(http));
                std::shared_ptr<TProtocol>             protocol = std::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = std::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            try{
                transport->open();
                CString username, password;
                _username.GetWindowTextW(username);
                _password.GetWindowTextW(password);
                register_service_info reg_info;
                reg_info.mgmt_addr = addr;
                reg_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
                reg_info.password = macho::stringutils::convert_unicode_to_utf8(password.GetString());
                register_return ret;
                if (is_transport()){
                    foreach(service_info info, service_infos){
                        if (info.id == saasame::transport::g_saasame_constants.LAUNCHER_SERVICE){
                            reg_info.version = info.version;
                            reg_info.path = info.path;
                        }
                    }
                    reg_info.service_types.insert(saasame::transport::g_saasame_constants.LOADER_SERVICE);
                    reg_info.service_types.insert(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE);
                    reg_info.__set_service_types(reg_info.service_types);
                    client->register_service(ret, "", reg_info, MachineInfo);
                }
                else{
                    foreach(service_info info, service_infos){
                        if (info.id == saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE){
                            reg_info.version = info.version;
                            reg_info.path = info.path;
                        }
                        reg_info.service_types.insert(info.id);
                    }
                    register_physical_packer_info packer_info;
                    packer_info.mgmt_addr = addr;
                    packer_info.packer_addr = _machine_id;
                    packer_info.path = reg_info.path;
                    packer_info.version = reg_info.version;
                    client->register_physical_packer(ret, "", packer_info, MachineInfo);
                }
                transport->close();
                if (ret.session.length()){
                    _session = ret.session;
                    _addr = addr;
                    Ret = TRUE;
                    macho::windows::registry reg(macho::windows::REGISTRY_CREATE);
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        reg[L"SessionId"] = macho::stringutils::convert_ansi_to_unicode(ret.session);
                        reg[L"MgmtAddr"] = macho::stringutils::convert_ansi_to_unicode(_addr);
                        reg[L"AllowMultiple"] = (DWORD)0;              
                    }
                    if (!ret.message.empty())
                        AfxMessageBox(macho::stringutils::convert_ansi_to_unicode(ret.message).c_str());
                }
            }
            catch (saasame::transport::invalid_operation &ex){
                AfxMessageBox(macho::stringutils::convert_ansi_to_unicode(ex.why).c_str());
                LOG(LOG_LEVEL_ERROR, L"invalid_operation: %s", macho::stringutils::convert_ansi_to_unicode(ex.why).c_str());
                if (transport->isOpen())
                    transport->close();
            }
            catch (TException &tx){
                AfxMessageBox(macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
                LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
                if (transport->isOpen())
                    transport->close();
            }
            curl_global_cleanup();
            SSL_COMP_free_compression_methods();
#else
            mgmt_op op({ addr }, addr, 0, TRUE == _is_https.GetCheck());
            if (op.open()){
                try{
                    CString username, password;
                    _username.GetWindowTextW(username);
                    _password.GetWindowTextW(password);
                    register_service_info reg_info;
                    reg_info.mgmt_addr = addr;
                    reg_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
                    reg_info.password = macho::stringutils::convert_unicode_to_utf8(password.GetString());
                    register_return ret;
                    if (is_transport()){
                        foreach(service_info info, service_infos){
                            if (info.id == saasame::transport::g_saasame_constants.LAUNCHER_SERVICE){
                                reg_info.version = info.version;
                                reg_info.path = info.path;
                            }
                        }
                        reg_info.service_types.insert(saasame::transport::g_saasame_constants.LOADER_SERVICE);
                        reg_info.service_types.insert(saasame::transport::g_saasame_constants.LAUNCHER_SERVICE);
                        reg_info.__set_service_types(reg_info.service_types);
                        op.client()->register_service(ret, "", reg_info, MachineInfo);
                    }
                    else{
                        foreach(service_info info, service_infos){
                            if (info.id == saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE){
                                reg_info.version = info.version;
                                reg_info.path = info.path;
                            }
                            reg_info.service_types.insert(info.id);
                        }
                        register_physical_packer_info packer_info;
                        packer_info.mgmt_addr = addr;
                        packer_info.packer_addr = _machine_id;
                        packer_info.path = reg_info.path;
                        packer_info.version = reg_info.version;
						packer_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
						packer_info.password = macho::stringutils::convert_unicode_to_utf8(password.GetString());
                        op.client()->register_physical_packer(ret, "", packer_info, MachineInfo);
                    }
                    if (ret.session.length()){
                        _session = ret.session;
                        _addr = addr;
                        Ret = TRUE;
                        macho::windows::registry reg(macho::windows::REGISTRY_CREATE);
                        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                            reg[L"SessionId"] = macho::stringutils::convert_ansi_to_unicode(ret.session);
                            reg[L"MgmtAddr"] = macho::stringutils::convert_ansi_to_unicode(_addr);
                            reg[L"AllowMultiple"] = (DWORD)0;
                        }
                        if (!ret.message.empty())
							AfxMessageBox(macho::stringutils::convert_utf8_to_unicode(ret.message).c_str());
                    }
                }
                catch (saasame::transport::invalid_operation &ex){
					AfxMessageBox(macho::stringutils::convert_utf8_to_unicode(ex.why).c_str());
					LOG(LOG_LEVEL_ERROR, L"invalid_operation: %s", macho::stringutils::convert_utf8_to_unicode(ex.why).c_str());
                }
                catch (TException &tx){
					AfxMessageBox(macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
					LOG(LOG_LEVEL_ERROR, L"TExceptioin: %s", macho::stringutils::convert_ansi_to_unicode(tx.what()).c_str());
                }
            }
            else{
                AfxMessageBox(L"Cannot connect to management server!");
            }
#endif
        }
    }
    catch (...){

    }
    return Ret;
}

BOOL CWizardMgmt::GetPhysicalMachineInfo(physical_machine_info &MachineInfo, std::set<service_info> &service_infos, int port)
{
#if 0
    std::string host = "localhost";
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    std::shared_ptr<TTransport> transport;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
    {
        try
        {
            std::shared_ptr<TSSLSocket> ssl_socket = std::shared_ptr<TSSLSocket>(_factory->createSocket(host, port));
            ssl_socket->setConnTimeout(30 * 1000);
            transport = std::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
            transport = nullptr;
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    if (!transport){
        std::shared_ptr<TSocket> socket(new TSocket(host, port));
        socket->setConnTimeout(30 * 1000);
        transport = std::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport){
        std::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            transport->open();
            client.get_service_list(service_infos, "");
            client.get_host_detail(MachineInfo, "", machine_detail_filter::type::FULL);
            transport->close();
            return TRUE;
        }
        catch (TException& ex) {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
#else
    thrift_connect<common_serviceClient> thrift(port);
    if (thrift.open()){
        try {
            thrift.client()->get_service_list(service_infos, "");
            thrift.client()->get_host_detail(MachineInfo, "", machine_detail_filter::type::FULL);
            return TRUE;
        }
        catch (TException& ex) {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
#endif
    return FALSE;
}

BOOL CWizardMgmt::OnInitDialog()
{
    CWizardPage::OnInitDialog();

    // TODO:  Add extra initialization here
    _is_https.SetCheck(TRUE);
    macho::windows::registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"SessionId"].exists())
            _session = reg[L"SessionId"].string();
        if (reg[L"MgmtAddr"].exists())
            _addr = reg[L"MgmtAddr"].string();
    }
    if (!_session.empty() && !_addr.empty()){
        _mgmt_addr.SetWindowTextW(macho::stringutils::convert_ansi_to_unicode(_addr).c_str());
        _mgmt_addr.EnableWindow(FALSE);
        _username.EnableWindow(FALSE);
        _password.EnableWindow(FALSE);
    }
    bool boot_from_disk = false;
    macho::windows::com_init com;
    storage::ptr stg = storage::get();
    if (stg){
        storage::disk::vtr disks = stg->get_disks();
        foreach(storage::disk::ptr d, disks){
            foreach(storage::volume::ptr v, d->get_volumes()){
                if (v->drive_letter() == L"X"){
                    boot_from_disk = true;
                }
            }
        }
    }
    if (boot_from_disk){
        _keep_settings.ShowWindow(SW_SHOW);
        _keep_settings.SetCheck(TRUE);
    }
    _hostname.SetWindowTextW(macho::windows::environment::get_computer_name().c_str());
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CWizardMgmt::OnClickSyslinkReset(NMHDR *pNMHDR, LRESULT *pResult)
{
    // TODO: Add your control notification handler code here
    *pResult = 0;
    _mgmt_addr.SetWindowTextW(L"");
    _mgmt_addr.EnableWindow(TRUE);
    _username.SetWindowTextW(L"");
    _username.EnableWindow(TRUE);
    _password.SetWindowTextW(L"");
    _password.EnableWindow(TRUE);
    _session.clear();
    _addr.c_str();
}

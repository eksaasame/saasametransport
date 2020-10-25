// WizardMgmt.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wizard.h"
#include "WizardMgmt.h"
#include "afxdialogex.h"

// CWizardMgmt dialog

IMPLEMENT_DYNAMIC(CWizardMgmt, CPropertyPage)

CWizardMgmt::CWizardMgmt(register_info& info, SheetPos posPositionOnSheet)
: CWizardPage(posPositionOnSheet, CWizardMgmt::IDD), _info(info)
{

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
    DDX_Control(pDX, IDC_CHK_MULTI, _is_allow_multiple);
}

BEGIN_MESSAGE_MAP(CWizardMgmt, CWizardPage)
    ON_NOTIFY(NM_CLICK, IDC_SYSLINK_RESET, &CWizardMgmt::OnClickSyslinkReset)
END_MESSAGE_MAP()

// CWizardMgmt message handlers

BOOL CWizardMgmt::OnWizardFinish()
{
    // TODO: Add your specialized code here and/or call the base class
    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
    BOOL register_result = (_info.session.empty()) ? RegisterServices() : TRUE;
    RestoreWaitCursor();
    if (register_result)
        return CWizardPage::OnWizardFinish();
    else
        return FALSE;
}

LRESULT CWizardMgmt::OnWizardNext()
{
    // TODO: Add your specialized code here and/or call the base class
    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
    BOOL register_result = (_info.session.empty()) ? RegisterServices() : TRUE;
    RestoreWaitCursor();
    if (register_result)
        return CWizardPage::OnWizardNext();
    else
        return -1;
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
    _info.addr = macho::stringutils::convert_unicode_to_utf8(mgmt_addr.GetString());
    try{
        if (GetPhysicalMachineInfo(MachineInfo, service_infos, 
            is_transport() ? saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT : saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT)){
            _info.machine_id = MachineInfo.machine_id;
            std::string uri = saasame::transport::g_saasame_constants.MANAGEMENT_SERVICE_PATH;

            boost::shared_ptr<saasame::transport::management_serviceClient>    client;
            boost::shared_ptr<TTransport>                                      transport;
            if (_info.is_https = (TRUE == _is_https.GetCheck())){
                boost::shared_ptr<TCurlClient>           http = boost::shared_ptr<TCurlClient>(new TCurlClient(boost::str(boost::format("https://%s%s") % _info.addr%uri)));
                transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
                boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            else{
                boost::shared_ptr<THttpClient>           http = boost::shared_ptr<THttpClient>(new THttpClient(_info.addr, 80, uri));
                transport = boost::shared_ptr<TTransport>(new TBufferedTransport(http));
                boost::shared_ptr<TProtocol>             protocol = boost::shared_ptr<TProtocol>(new TBinaryProtocol(transport));
                client = boost::shared_ptr<saasame::transport::management_serviceClient>(new saasame::transport::management_serviceClient(protocol));
            }
            try{
                transport->open();
                CString username, password;
                _username.GetWindowTextW(username);
                _password.GetWindowTextW(password);
                std::string session_id;
                register_service_info reg_info;
                reg_info.mgmt_addr = _info.addr;
                reg_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
                reg_info.password = macho::stringutils::convert_unicode_to_utf8(password.GetString());
                if (is_transport()){
                    foreach(service_info info, service_infos){
                        if (info.id == saasame::transport::g_saasame_constants.SCHEDULER_SERVICE){
                            reg_info.version = info.version;
                            reg_info.path = info.path;
                        }
                        reg_info.service_types.insert(info.id);
                    }
                    reg_info.__set_service_types(reg_info.service_types);
                    client->register_service(session_id, "", reg_info, MachineInfo);
                    transport->close();
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
                    packer_info.mgmt_addr = _info.addr;
                    packer_info.packer_addr = _info.machine_id;
                    packer_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
                    packer_info.password = macho::stringutils::convert_unicode_to_utf8(password.GetString());
                    packer_info.path = reg_info.path;
                    packer_info.version = reg_info.version;
                    client->register_physical_packer(session_id, _info.session, packer_info, MachineInfo);
                    transport->close();
                }
                if (session_id.length()){
                    _info.session = session_id;
                    _info.username = reg_info.username;
                    _info.password = reg_info.password;
                    Ret = TRUE;
                    macho::windows::registry reg;
                    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                        reg[L"SessionId"] = macho::stringutils::convert_ansi_to_unicode(session_id);
                        reg[L"MgmtAddr"] = _info.addr;
                        if (TRUE == _is_allow_multiple.GetCheck())
                            reg[L"AllowMultiple"] = (DWORD)1;
                        else
                            reg[L"AllowMultiple"] = (DWORD)0;
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
                    AfxMessageBox(L"Registered!");
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
        }
    }
    catch (...){

    }
    return Ret;
}

BOOL CWizardMgmt::GetPhysicalMachineInfo(physical_machine_info &MachineInfo, std::set<service_info> &service_infos, int port)
{
    std::string host = "localhost";
    macho::windows::registry reg;
    boost::filesystem::path p(macho::windows::environment::get_working_directory());
    boost::shared_ptr<TTransport> transport;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
        if (reg[L"KeyPath"].exists() && reg[L"KeyPath"].is_string())
            p = reg[L"KeyPath"].wstring();
    }
    if (boost::filesystem::exists(p / "server.key") && boost::filesystem::exists(p / "server.crt"))
    {
        try
        {
            boost::shared_ptr<TSSLSocketFactory> factory;
            factory = boost::shared_ptr<TSSLSocketFactory>(new TSSLSocketFactory());
            factory->authenticate(false);
            factory->loadCertificate((p / "server.crt").string().c_str());
            factory->loadPrivateKey((p / "server.key").string().c_str());
            factory->loadTrustedCertificates((p / "server.crt").string().c_str());
            boost::shared_ptr<AccessManager> accessManager(new MyAccessManager());
            factory->access(accessManager);
            boost::shared_ptr<TSSLSocket> ssl_socket = boost::shared_ptr<TSSLSocket>(factory->createSocket(host, port));
            ssl_socket->setConnTimeout(1000);
            transport = boost::shared_ptr<TTransport>(new TBufferedTransport(ssl_socket));
        }
        catch (TException& ex) {
            transport = nullptr;
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
    if (!transport){
        boost::shared_ptr<TSocket> socket(new TSocket(host, port));
        socket->setConnTimeout(1000);
        transport = boost::shared_ptr<TTransport>(new TBufferedTransport(socket));
    }
    if (transport){
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        common_serviceClient client(protocol);
        try {
            transport->open();
            client.get_service_list(service_infos,"");
            client.get_host_detail(MachineInfo, "", machine_detail_filter::type::FULL);
            transport->close();
            return TRUE;
        }
        catch (TException& ex) {
            LOG(LOG_LEVEL_ERROR, L"%s", stringutils::convert_ansi_to_unicode(ex.what()).c_str());
        }
    }
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
            _info.session = reg[L"SessionId"].string();
        if (reg[L"MgmtAddr"].exists())
            _info.addr = reg[L"MgmtAddr"].string();
        if (reg[L"AllowMultiple"].exists() && reg[L"AllowMultiple"].is_dword())
            _is_allow_multiple.SetCheck((DWORD)reg[L"AllowMultiple"] > 0 ? TRUE : FALSE );
        else
            _is_allow_multiple.SetCheck(FALSE);
    }
    if (!_info.session.empty() && !_info.addr.empty()){
        _mgmt_addr.SetWindowTextW(macho::stringutils::convert_ansi_to_unicode(_info.addr).c_str());
        _mgmt_addr.EnableWindow(FALSE);
        _username.EnableWindow(FALSE);
        _password.EnableWindow(FALSE);
        _is_allow_multiple.EnableWindow(FALSE);
    }
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
    _is_allow_multiple.EnableWindow(TRUE);
    _is_allow_multiple.SetCheck(FALSE);
    _info.session.clear();
    _info.addr.c_str();
}
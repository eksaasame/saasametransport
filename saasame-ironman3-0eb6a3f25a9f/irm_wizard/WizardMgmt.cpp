// WizardMgmt.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wizard.h"
#include "WizardMgmt.h"
#include "afxdialogex.h"
#include "..\gen-cpp\service_op.h"
#include "..\gen-cpp\mgmt_op.h"
// CWizardMgmt dialog

IMPLEMENT_DYNAMIC(CWizardMgmt, CPropertyPage)

CWizardMgmt::CWizardMgmt(register_info& info, SheetPos posPositionOnSheet)
: CWizardPage(posPositionOnSheet, CWizardMgmt::IDD), _info(info)
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
    DDX_Control(pDX, IDC_SYSLINK_RESET, _ClearBtn);
    DDX_Control(pDX, IDC_CHK_MULTI, _is_allow_multiple);
    DDX_Control(pDX, IDC_STATIC_STATUS, _status);
}

BEGIN_MESSAGE_MAP(CWizardMgmt, CWizardPage)
    ON_NOTIFY(NM_CLICK, IDC_SYSLINK_RESET, &CWizardMgmt::OnClickSyslinkReset)
    ON_EN_CHANGE(IDC_EDIT_MGMT, &CWizardMgmt::OnChangeEdit)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, &CWizardMgmt::OnChangeEdit)
END_MESSAGE_MAP()

// CWizardMgmt message handlers

BOOL CWizardMgmt::OnWizardFinish()
{
    // TODO: Add your specialized code here and/or call the base class
    BeginWaitCursor(); //SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
    BOOL register_result = (_info.session.empty()) ? RegisterServices() : TRUE;
    EndWaitCursor(); //RestoreWaitCursor();
    if (register_result)
        return CWizardPage::OnWizardFinish();
    else
        return FALSE;
}

LRESULT CWizardMgmt::OnWizardNext()
{
    // TODO: Add your specialized code here and/or call the base class
    BeginWaitCursor(); //SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
    BOOL register_result = (_info.session.empty()) ? RegisterServices() : TRUE;
    EndWaitCursor(); //RestoreWaitCursor();
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
    CString mgmt_addr, username;
    _mgmt_addr.GetWindowTextW(mgmt_addr);
    _username.GetWindowTextW(username);
    if (mgmt_addr.IsEmpty() || username.IsEmpty()){
        return FALSE;
    }
    _info.addr = macho::stringutils::convert_unicode_to_utf8(mgmt_addr.GetString());
    CString connecting;
    connecting.LoadStringW(IDS_CONNECTING);
    _status.SetWindowTextW(connecting);
    try{
        if (GetPhysicalMachineInfo(MachineInfo, service_infos, 
            is_transport() ? saasame::transport::g_saasame_constants.LAUNCHER_SERVICE_PORT : saasame::transport::g_saasame_constants.PHYSICAL_PACKER_SERVICE_PORT)){
            _info.machine_id = MachineInfo.machine_id;
            mgmt_op op({ _info.addr }, _info.addr, 0, TRUE == _is_https.GetCheck());
            if (op.open()){
                try{
                    register_return ret;
                    register_service_info reg_info;
                    reg_info.mgmt_addr = _info.addr;
                    reg_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
                    bool _is_transport = is_transport();
                    if (_is_transport){
                        foreach(service_info info, service_infos){
                            if (info.id == saasame::transport::g_saasame_constants.SCHEDULER_SERVICE){
                                reg_info.version = info.version;
                                reg_info.path = info.path;
                            }
                            reg_info.service_types.insert(info.id);
                        }
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
                        packer_info.mgmt_addr = _info.addr;
                        packer_info.packer_addr = _info.machine_id;
                        packer_info.username = macho::stringutils::convert_unicode_to_utf8(username.GetString());
                        packer_info.path = reg_info.path;
                        packer_info.version = reg_info.version;
                        op.client()->register_physical_packer(ret, _info.session, packer_info, MachineInfo);
                    }
                    if (ret.session.length()){
                        _info.session = ret.session;
                        _info.username = reg_info.username;
                        _info.password = reg_info.password;
                        Ret = TRUE;
                        macho::windows::registry reg;
                        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
                            reg[L"SessionId"] = macho::stringutils::convert_ansi_to_unicode(ret.session);
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
                CString cannot_connect;
                cannot_connect.LoadStringW(IDS_CANNOT_CONNECT);
                AfxMessageBox(cannot_connect);
            }
        }
    }
    catch (...){

    }
    if (Ret){
        CString connected;
        connected.LoadStringW(IDS_CONNECTED);
        _status.SetWindowTextW(connected);
    }
    else{
        CString disconnected;
        disconnected.LoadStringW(IDS_DISCONNECTED);
        _status.SetWindowTextW(disconnected);
    }
    return Ret;
}

BOOL CWizardMgmt::GetPhysicalMachineInfo(physical_machine_info &MachineInfo, std::set<service_info> &service_infos, int port)
{
    if (is_transport()){
        try{
            macho::windows::service irm_launcher = macho::windows::service::get_service(L"irm_launcher");
            irm_launcher.start();
        }
        catch (...){
        }
    }
    else{
        try{
            macho::windows::service irm_host_packer = macho::windows::service::get_service(L"irm_host_packer");
            irm_host_packer.start();
        }
        catch (...){
        }
    }
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
    return FALSE;
}

BOOL CWizardMgmt::OnInitDialog()
{
    CWizardPage::OnInitDialog();
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
        _is_allow_multiple.EnableWindow(FALSE);
        CString connected;
        connected.LoadStringW(IDS_CONNECTED);
        _status.SetWindowTextW(connected);
        CPropertySheet* pSheet = (CPropertySheet*)GetParent();
        pSheet->SetWizardButtons(PSWIZB_DISABLEDFINISH);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
}

void CWizardMgmt::OnClickSyslinkReset(NMHDR *pNMHDR, LRESULT *pResult)
{
    // TODO: Add your control notification handler code here
    *pResult = 0;
    if (!_info.session.empty()){
        CString disconnect;
        disconnect.LoadStringW(IDS_DISCONNECT);
        if (IDYES == AfxMessageBox(disconnect, MB_YESNO)){
            _mgmt_addr.SetWindowTextW(L"");
            _mgmt_addr.EnableWindow(TRUE);
            _username.SetWindowTextW(L"");
            _username.EnableWindow(TRUE);
            _is_allow_multiple.EnableWindow(TRUE);
            _is_allow_multiple.SetCheck(FALSE);
            _info.session.clear();
            _info.addr.c_str();
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
            CString disconnected;
            disconnected.LoadStringW(IDS_DISCONNECTED);
            _status.SetWindowTextW(disconnected);
        }
    }
}

BOOL CWizardMgmt::OnSetActive()
{
    //Hole den Dialog
    CPropertySheet* pSheet = (CPropertySheet*)GetParent();
    ASSERT_KINDOF(CPropertySheet, pSheet);
    CString connect;
    connect.LoadStringW(IDS_CONNECT);
    pSheet->SetFinishText(connect);
    pSheet->SetWizardButtons(PSWIZB_DISABLEDFINISH);
    return CPropertyPage::OnSetActive();
}


void CWizardMgmt::OnChangeEdit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CWizardPage::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.
    CString mgmt_addr, username;
    _mgmt_addr.GetWindowTextW(mgmt_addr);
    _username.GetWindowTextW(username);
    CPropertySheet* pSheet = (CPropertySheet*)GetParent();
    if (mgmt_addr.IsEmpty() || username.IsEmpty()){
        pSheet->SetWizardButtons(PSWIZB_DISABLEDFINISH);
    }
    else{
        pSheet->SetWizardButtons(PSWIZB_FINISH);
    }
}

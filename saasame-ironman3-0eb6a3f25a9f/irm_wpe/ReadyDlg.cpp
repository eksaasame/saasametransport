// ReadyDlg.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wpe.h"
#include "ReadyDlg.h"
#include "afxdialogex.h"
#include "macho.h"

// CReadyDlg dialog

IMPLEMENT_DYNAMIC(CReadyDlg, CDialogEx)

CReadyDlg::CReadyDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CReadyDlg::IDD, pParent), _terminated(false)
{

}

CReadyDlg::~CReadyDlg()
{
    _terminated = true;
}

void CReadyDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_NET, _networks);
    DDX_Control(pDX, IDC_IP_ADDRESS, _address);
    DDX_Control(pDX, IDOK, _reboot_btn);
    DDX_Control(pDX, IDC_STATIC_HOST, _hostname);
}


BEGIN_MESSAGE_MAP(CReadyDlg, CDialogEx)
    ON_BN_CLICKED(IDOK, &CReadyDlg::OnBnClickedOk)
    ON_CBN_SELCHANGE(IDC_COMBO_NET, &CReadyDlg::OnSelchangeComboNet)
    ON_NOTIFY(NM_CLICK, IDC_CMD, &CReadyDlg::OnNMClickCmd)
END_MESSAGE_MAP()


// CReadyDlg message handlers

void CReadyDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    if (IDOK == AfxMessageBox(L"The system will be reboot. Please remove the media!", MB_OKCANCEL)){
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"MBROnUEFIBIOS"].exists() && ((DWORD)reg[L"MBROnUEFIBIOS"]) > 0){
                AfxMessageBox(L"Please make sure MBR disk to boot from UEFI BIOS!", MB_OK);
            }
        }
        macho::windows::environment::shutdown_system(true, L"", 0);
        CDialogEx::OnOK();
    }
}

void CReadyDlg::check_recovery_status(){
    while (!_terminated){
        macho::windows::registry reg;
        if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH))){
            if (reg[L"RecoveryImageReady"].exists() && ((DWORD)reg[L"RecoveryImageReady"]) > 0){
                _reboot_btn.EnableWindow(TRUE);
                break;
            }
        }
        boost::this_thread::sleep(boost::posix_time::seconds(5));
    }
}

BOOL CReadyDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  Add extra initialization here
    _adapters.clear();
    _address.EnableWindow(FALSE);
    macho::windows::network::adapter::vtr adapters = macho::windows::network::get_network_adapters();
    int i = 0;
    foreach(macho::windows::network::adapter::ptr adapter, adapters){
        if (adapter->physical_adapter() && adapter->net_enabled()){
            macho::windows::network::adapter_config::ptr config = adapter->get_setting();
            if (config->ip_enabled()){
                _adapters.push_back(adapter);
                _networks.InsertString(i, boost::str(boost::wformat(L"%s - %s") % adapter->net_connection_id() % adapter->description()).c_str());
                i++;
            }
        }
    }
    if (_adapters.size()){
        _networks.SetCurSel(0);
        _current_selection = _adapters[0]->get_setting();
        show_network_config(_current_selection);
    }
    _thread = boost::thread(&CReadyDlg::check_recovery_status, this);
    std::wstring hostname = macho::windows::environment::get_computer_name();
    registry reg;
    if (reg.open(macho::stringutils::convert_ansi_to_unicode(saasame::transport::g_saasame_constants.CONFIG_PATH)) && reg[_T("HostName")].exists()){
        hostname = reg[_T("HostName")].wstring();
    }
    _hostname.SetWindowTextW(hostname.c_str());
    //macho::windows::storage::ptr stg = macho::windows::storage::get();
    //if (stg){
    //    macho::windows::storage::disk::vtr disks = stg->get_disks();
    //    foreach(macho::windows::storage::disk::ptr &d, disks)
    //        AfxMessageBox(d->friendly_name().c_str());
    //}
    //else{
    //    AfxMessageBox(L"Failed to open the storage object");
    //}
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


void CReadyDlg::OnSelchangeComboNet()
{
    // TODO: Add your control notification handler code here
    _current_selection = _adapters[_networks.GetCurSel()]->get_setting();
    show_network_config(_current_selection);
}

void CReadyDlg::show_network_config(macho::windows::network::adapter_config::ptr& config){
    macho::string_array_w address = config->ip_address();
    DWORD nField0 = 0, nField1 = 0, nField2 = 0, nField3 = 0;
    if (address.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(address.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
        _address.SetAddress(nField0, nField1, nField2, nField3);
    }
    else{
        _address.ClearAddress();
    }
}

void CReadyDlg::OnNMClickCmd(NMHDR *pNMHDR, LRESULT *pResult)
{
    system("start cmd.exe");
    // TODO: Add your control notification handler code here
    *pResult = 0;
}

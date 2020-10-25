// WizardNet.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wpe.h"
#include "WizardNet.h"
#include "afxdialogex.h"
#include "winpe_util.h"
#include <VersionHelpers.h>
// CWizardNet dialog

IMPLEMENT_DYNAMIC(CWizardNet, CPropertyPage)

CWizardNet::CWizardNet(winpe_settings & settings, SheetPos posPositionOnSheet)
:CWizardPage(posPositionOnSheet, CWizardNet::IDD), _settings(settings),
    _current_selection_index(0)
{

}

CWizardNet::~CWizardNet()
{
}

void CWizardNet::DoDataExchange(CDataExchange* pDX)
{
    CWizardPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_CONN, _connections);
    DDX_Control(pDX, IDC_EDIT_MAC, _mac);
    DDX_Control(pDX, IDC_RADIO_AUTOIP, _auto_dhcp);
    DDX_Control(pDX, IDC_RADIO_MANULIP, _static_ip);
    DDX_Control(pDX, IDC_IP_IPADDRESS, _address);
    DDX_Control(pDX, IDC_IP_SUBNET, _subnet);
    DDX_Control(pDX, IDC_IP_GATEWAY, _getway);
    DDX_Control(pDX, IDC_RADIO_AUTODNS, _auto_dns);
    DDX_Control(pDX, IDC_RADIO_MANULDNS, _static_dns);
    DDX_Control(pDX, IDC_IP_DNS1, _dns1);
    DDX_Control(pDX, IDC_IP_DNS2, _dns2);
    DDX_Control(pDX, IDC_COMBO_SPEED_DUPLEX, _speed_duplexs);
}


BEGIN_MESSAGE_MAP(CWizardNet, CWizardPage)
    ON_BN_CLICKED(IDC_RADIO_AUTOIP, &CWizardNet::OnClickedRadioAutoip)
    ON_BN_CLICKED(IDC_RADIO_AUTODNS, &CWizardNet::OnClickedRadioAutodns)
    ON_COMMAND(IDC_RADIO_MANULDNS, &CWizardNet::OnRadioManuldns)
    ON_COMMAND(IDC_RADIO_MANULIP, &CWizardNet::OnRadioManulip)
    ON_CBN_SELCHANGE(IDC_COMBO_CONN, &CWizardNet::OnSelchangeComboConn)
    ON_NOTIFY(IPN_FIELDCHANGED, IDC_IP_IPADDRESS, &CWizardNet::OnFieldchangedIpIpaddress)
END_MESSAGE_MAP()

// CWizardNet message handlers

BOOL CWizardNet::OnInitDialog()
{
    CWizardPage::OnInitDialog();
    // TODO:  Add extra initialization here
    _adapters.clear();
    macho::windows::network::adapter::vtr adapters = macho::windows::network::get_network_adapters();
    int i = 0;
    foreach(macho::windows::network::adapter::ptr adapter, adapters){
        if (adapter->physical_adapter() && adapter->net_enabled()){
            macho::windows::network::adapter_config::ptr config = adapter->get_setting();
            if (config->ip_enabled()){
                _adapters.push_back(adapter);
                _connections.InsertString(i, boost::str(boost::wformat(L"%s - %s") % adapter->net_connection_id() % adapter->description()).c_str());
                i++;
            }
        }
    }

    if (_adapters.size()){
        _connections.SetCurSel(0);
        _current_selection = _adapters[0]->get_setting();
        show_network_config(_current_selection);
    }
    else{
        _auto_dhcp.EnableWindow(FALSE);
        _static_ip.EnableWindow(FALSE);
        _address.EnableWindow(FALSE);
        _subnet.EnableWindow(FALSE);
        _getway.EnableWindow(FALSE);
        _auto_dns.EnableWindow(FALSE);
        _static_dns.EnableWindow(FALSE);
        _dns1.EnableWindow(FALSE);
        _dns2.EnableWindow(FALSE);
    }
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CWizardNet::show_network_config(macho::windows::network::adapter_config::ptr& config){

    uint32_t index = config->index();
    _speed_duplexs.ResetContent();
    _speed_duplexs_map.clear();
    macho::windows::registry reg;
    if (reg.open(boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\%04i\\Ndi\\Params\\*SpeedDuplex\\enum") % index))){
        for (int i = 0; i < reg.count(); i++){
            _speed_duplexs.InsertString(i, reg[i].wstring().c_str());
            _speed_duplexs_map[i] = reg[i].name();
        }
        reg.close();
        if (reg.open(boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\%04i") % index))){
            if (reg[L"*SpeedDuplex"].exists() && reg[L"*SpeedDuplex"].is_string()){
                typedef std::map<int, std::wstring> _speed_duplexs_map_type;
                std::wstring default_speed_duplex = reg[L"*SpeedDuplex"].wstring();
                foreach(_speed_duplexs_map_type::value_type & v, _speed_duplexs_map){
                    if (v.second == default_speed_duplex){
                        _speed_duplexs.SetCurSel(v.first);
                        break;
                    }
                }
            }
            reg.close();
        }
    }
    _mac.SetWindowTextW(config->mac_address().c_str());
    if (config->dhcp_enabled()){
        _auto_dhcp.SetCheck(TRUE);
        _auto_dns.EnableWindow(TRUE);
        _static_ip.SetCheck(FALSE);
        _address.EnableWindow(FALSE);
        _subnet.EnableWindow(FALSE);
        _getway.EnableWindow(FALSE);
        _address.ClearAddress();
        _subnet.ClearAddress();
        _getway.ClearAddress();
    }
    else{
        _auto_dhcp.SetCheck(FALSE);
        _static_ip.SetCheck(TRUE);
        _auto_dns.SetCheck(FALSE);
        _static_dns.SetCheck(TRUE);
        _auto_dns.EnableWindow(FALSE);
        _address.EnableWindow(TRUE);
        _subnet.EnableWindow(TRUE);
        _getway.EnableWindow(TRUE);
        macho::string_array_w address = config->ip_address();
        DWORD nField0 = 0, nField1 = 0, nField2 = 0, nField3 = 0;
        if (address.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(address.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
            _address.SetAddress(nField0, nField1, nField2, nField3);
        }
        macho::string_array_w subnet = config->ip_subnet();
        if (subnet.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(subnet.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
            _subnet.SetAddress(nField0, nField1, nField2, nField3);
        }
        macho::string_array_w getway = config->default_ip_gateway();
        if (getway.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(getway.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
            _getway.SetAddress(nField0, nField1, nField2, nField3);
        }
    }
    macho::string_array_w dns = config->dns_server_search_order();    
    std::wstring setting_reg_path = boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s") % config->setting_id());

    if (dns.size() > 0 && reg.open(setting_reg_path) && reg[L"NameServer"].exists() && reg[L"NameServer"].wstring().length()){
        _auto_dns.SetCheck(FALSE);
        _static_dns.SetCheck(TRUE);
        _dns1.EnableWindow(TRUE);
        _dns2.EnableWindow(TRUE);
        DWORD nField0 = 0, nField1 = 0, nField2 = 0, nField3 = 0;
        if (4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(dns.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
            _dns1.SetAddress(nField0, nField1, nField2, nField3);
        }
        if (dns.size() > 1){
            if (4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(dns.at(1)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
                _dns2.SetAddress(nField0, nField1, nField2, nField3);
            }
        }
        else{
            _dns2.ClearAddress();
        }
    }
    else{
        _dns1.ClearAddress();
        _dns2.ClearAddress();
        if (config->dhcp_enabled()){
            _auto_dns.SetCheck(TRUE);
            _static_dns.SetCheck(FALSE);
            _dns1.EnableWindow(FALSE);
            _dns2.EnableWindow(FALSE);
        }
        else{
            _auto_dns.SetCheck(FALSE);
            _static_dns.SetCheck(TRUE);
            _dns1.EnableWindow(TRUE);
            _dns2.EnableWindow(TRUE);
        }
    }
}

void CWizardNet::OnClickedRadioAutoip()
{
    // TODO: Add your control notification handler code here
    _static_ip.SetCheck(FALSE);
    _address.ClearAddress();
    _subnet.ClearAddress();
    _getway.ClearAddress();
    _address.EnableWindow(FALSE);
    _subnet.EnableWindow(FALSE);
    _getway.EnableWindow(FALSE);
    _auto_dns.EnableWindow(TRUE);
}

void CWizardNet::OnClickedRadioAutodns()
{
    // TODO: Add your control notification handler code here
    _static_dns.SetCheck(FALSE);
    _dns1.ClearAddress();
    _dns2.ClearAddress();
    _dns1.EnableWindow(FALSE);
    _dns2.EnableWindow(FALSE);
}

void CWizardNet::OnRadioManuldns()
{
    // TODO: Add your command handler code here
    _auto_dns.SetCheck(FALSE);
    _dns1.EnableWindow(TRUE);
    _dns2.EnableWindow(TRUE);
}

void CWizardNet::OnRadioManulip()
{
    // TODO: Add your command handler code here
    _auto_dhcp.SetCheck(FALSE);
    _address.EnableWindow(TRUE);
    _subnet.EnableWindow(TRUE);
    _getway.EnableWindow(TRUE);
    _auto_dns.SetCheck(FALSE);
    _auto_dns.EnableWindow(FALSE);
    _static_dns.SetCheck(TRUE);
    _dns1.EnableWindow(TRUE);
    _dns2.EnableWindow(TRUE);
}

void CWizardNet::OnSelchangeComboConn()
{
    // TODO: Add your control notification handler code here
    if (apply_network_config(_current_selection)){
        _current_selection_index = _connections.GetCurSel();
        _current_selection = _adapters[_current_selection_index]->get_setting();
        show_network_config(_current_selection);
    }
    else{
        _connections.SetCurSel(_current_selection_index);
    }
}

LRESULT CWizardNet::OnWizardNext()
{
    // TODO: Add your specialized code here and/or call the base class
    if (_adapters.size()){
        BeginWaitCursor(); //SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
        if (apply_network_config(_current_selection)){
            
            EndWaitCursor(); //RestoreWaitCursor();
            return CWizardPage::OnWizardNext();
        }
        else{
            EndWaitCursor(); //RestoreWaitCursor();
            AfxMessageBox(L"Failed to change the network settings!", MB_OK | MB_ICONEXCLAMATION);
        }
    }
    else{
        AfxMessageBox(L"Doesn't have any connected network adapter!", MB_OK | MB_ICONEXCLAMATION);
    }
    return -1;
}

BOOL CWizardNet::OnWizardFinish()
{
    // TODO: Add your specialized code here and/or call the base class
    if (_adapters.size()){
        BeginWaitCursor(); //SetCursor(AfxGetApp()->LoadStandardCursor(IDC_WAIT));
        if (apply_network_config(_current_selection)){
            if (macho::windows::environment::is_winpe()){
                boost::filesystem::path work_dir = macho::windows::environment::get_working_directory();
                std::wstring loader = boost::filesystem::path(work_dir / L"irm_loader.exe").wstring();
                std::wstring launcher = boost::filesystem::path(work_dir / L"irm_launcher.exe").wstring();
                std::wstring host_packer = boost::filesystem::path(work_dir / L"irm_host_packer.exe").wstring();
                std::wstring ret;
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
                process::exec_console_application_with_timeout(boost::str(boost::wformat(L"%s\\wpeutil.exe DisableFirewall") % macho::windows::environment::get_system_directory()), ret, -1, true);
            }
            EndWaitCursor(); //RestoreWaitCursor();
            return CWizardPage::OnWizardFinish();
        }
        else{
            EndWaitCursor(); //RestoreWaitCursor();
            AfxMessageBox(L"Failed to change the network settings!", MB_OK | MB_ICONEXCLAMATION);
        }
    }
    else{
        AfxMessageBox(L"Doesn't have any connected network adapter!", MB_OK | MB_ICONEXCLAMATION);
    }
    return FALSE;
}

bool CWizardNet::apply_network_config(macho::windows::network::adapter_config::ptr& config){
    bool result = true;
    bool is_changed = false;
    bool is_dhcp_enable = config->dhcp_enabled();
    macho::string_array_w new_dnss, new_address, new_subnet, new_getway;
    macho::string_array_w address = config->ip_address();
    macho::string_array_w subnet = config->ip_subnet();
    macho::string_array_w getway = config->default_ip_gateway();
    macho::string_array_w dnss;
    std::wstring setting_reg_path = boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\%s") % config->setting_id());
    macho::windows::registry reg;
    if (reg.open(setting_reg_path) && reg[L"NameServer"].exists() && reg[L"NameServer"].wstring().length()){
        dnss = config->dns_server_search_order();
    }

    if (TRUE == _auto_dhcp.GetCheck()){
        is_changed = !is_dhcp_enable;
    }
    else if (TRUE == _static_ip.GetCheck()){
        is_changed = is_dhcp_enable;
        DWORD nField0 = 0, nField1 = 0, nField2 = 0, nField3 = 0;
        BYTE _nField0 = 0, _nField1 = 0, _nField2 = 0, _nField3 = 0;
        int cols = _address.GetAddress(_nField0, _nField1, _nField2, _nField3);
        if( cols != 4 || 0 == _nField0 || 0 == _nField3){
            if (0 == _nField0)
                _address.SetFieldFocus(0);
            else if (0 == _nField1)
                _address.SetFieldFocus(1);
            else if (0 == _nField2)
                _address.SetFieldFocus(2);
            else if (0 == _nField3)
                _address.SetFieldFocus(3);
            else
                _address.SetFieldFocus(0);
            return false;
        }
        if (cols != 0){
            new_address.push_back(boost::str(boost::wformat(L"%d.%d.%d.%d") % _nField0%_nField1%_nField2%_nField3));
            if (address.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(address.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
                if (nField0 != _nField0 || nField1 != _nField1 || nField2 != _nField2 || nField3 != _nField3)
                    is_changed = true;
            }
            else{
                is_changed = true;
            }
        }
        _nField0 = 0;
        _nField1 = 0;
        _nField2 = 0;
        _nField3 = 0;
        cols = _subnet.GetAddress(_nField0, _nField1, _nField2, _nField3);
        if (cols != 4 || 0 == _nField0 ){
            if (0 == _nField0)
                _subnet.SetFieldFocus(0);
            else if (0 == _nField1)
                _subnet.SetFieldFocus(1);
            else if (0 == _nField2)
                _subnet.SetFieldFocus(2);
            else if (0 == _nField3)
                _subnet.SetFieldFocus(3);
            else
                _subnet.SetFieldFocus(0);
            return false;
        }
        if (cols != 0){
            new_subnet.push_back(boost::str(boost::wformat(L"%d.%d.%d.%d") % _nField0%_nField1%_nField2%_nField3));
            if (subnet.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(subnet.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
                if (nField0 != _nField0 || nField1 != _nField1 || nField2 != _nField2 || nField3 != _nField3)
                    is_changed = true;
            }
            else{
                is_changed = true;
            }
        }
        _nField0 = 0;
        _nField1 = 0;
        _nField2 = 0;
        _nField3 = 0;
        cols = _getway.GetAddress(_nField0, _nField1, _nField2, _nField3);
        if (cols != 0){
            if (cols != 4 || 0 == _nField0 || 0 == _nField3){
                if (0 == _nField0)
                    _getway.SetFieldFocus(0);
                else if (0 == _nField1)
                    _getway.SetFieldFocus(1);
                else if (0 == _nField2)
                    _getway.SetFieldFocus(2);
                else if (0 == _nField3)
                    _getway.SetFieldFocus(3);
                else
                    _getway.SetFieldFocus(0);
                return false;
            }

            new_getway.push_back(boost::str(boost::wformat(L"%d.%d.%d.%d") % _nField0%_nField1%_nField2%_nField3));
            if (getway.size() && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(getway.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
                if (nField0 != _nField0 || nField1 != _nField1 || nField2 != _nField2 || nField3 != _nField3)
                    is_changed = true;
            }
            else{
                is_changed = true;
            }
        }
        else if (getway.size())
            is_changed = true;
    }

    if (result && TRUE == _static_dns.GetCheck()){
        DWORD nField0 = 0, nField1 = 0, nField2 = 0, nField3 = 0;
        BYTE _nField0 = 0, _nField1 = 0, _nField2 = 0, _nField3 = 0;
        if (0 == _dns1.GetAddress(nField0) && 0 != _dns2.GetAddress(nField1)){
            _dns1.SetAddress(nField1);
            _dns2.ClearAddress();
        }
        nField0 = 0;
        nField1 = 0;
        int cols = _dns1.GetAddress(_nField0, _nField1, _nField2, _nField3);
        if (cols != 0){
            if (cols != 4 || 0 == _nField0 || 0 == _nField3){
                if (0 == _nField0)
                    _dns1.SetFieldFocus(0);
                else if (0 == _nField1)
                    _dns1.SetFieldFocus(1);
                else if (0 == _nField2)
                    _dns1.SetFieldFocus(2);
                else if (0 == _nField3)
                    _dns1.SetFieldFocus(3);
                else
                    _dns1.SetFieldFocus(0);
                return false;
            }
            new_dnss.push_back(boost::str(boost::wformat(L"%d.%d.%d.%d") % _nField0%_nField1%_nField2%_nField3));
            if (dnss.size() > 0 && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(dnss.at(0)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
                if (nField0 != _nField0 || nField1 != _nField1 || nField2 != _nField2 || nField3 != _nField3)
                    is_changed = true;
            }
            else {
                is_changed = true;
            }
        }
        else if (dnss.size() > 0)
            is_changed = true;

        _nField0 = 0;
        _nField1 = 0;
        _nField2 = 0;
        _nField3 = 0;

        cols = _dns2.GetAddress(_nField0, _nField1, _nField2, _nField3);
        if (cols != 0){
            if (cols != 4 || 0 == _nField0 || 0 == _nField3){
                if (0 == _nField0)
                    _dns2.SetFieldFocus(0);
                else if (0 == _nField1)
                    _dns2.SetFieldFocus(1);
                else if (0 == _nField2)
                    _dns2.SetFieldFocus(2);
                else if (0 == _nField3)
                    _dns2.SetFieldFocus(3);
                else
                    _dns2.SetFieldFocus(0);
                return false;
            }
            new_dnss.push_back(boost::str(boost::wformat(L"%d.%d.%d.%d") % _nField0%_nField1%_nField2%_nField3));
            if (dnss.size() > 1 && 4 == sscanf_s(macho::stringutils::convert_unicode_to_ansi(dnss.at(1)).c_str(), "%d.%d.%d.%d", &nField0, &nField1, &nField2, &nField3)){
                if (nField0 != _nField0 || nField1 != _nField1 || nField2 != _nField2 || nField3 != _nField3)
                    is_changed = true;
            }
            else{
                is_changed = true;
            }
        }
        else if (dnss.size() > 1)
            is_changed = true;

        if (!is_changed && (TRUE == _static_ip.GetCheck()) && dnss.size()){
            is_changed = true;
        }            
    }

    if (is_changed){
        macho::windows::network::adapter::ptr adapter = _adapters[_current_selection_index];
        winpe_settings::set_ip_settings_by_netsh_command(config->setting_id(), adapter->net_connection_id(), new_address, new_subnet, new_dnss, new_getway, L"0", std::vector<std::wstring>(), true); 
        winpe_network_setting setting;
        setting.mac_addr = adapter->mac_address();
        setting.tbIPAddress = new_address;
        setting.tbSubnetMask = new_subnet;
        setting.tbDNSServers = new_dnss;
        setting.tbGateways = new_getway;
        _settings.network_settings[setting.mac_addr] = setting;
        /*if (result == config->enable_dhcp()){
            if (new_address.size())
                result = config->enable_static(new_address, new_subnet);
            if (result && new_getway.size())
                result = config->set_gateways(new_getway);          
            if (result && new_dnss.size())
                result = config->set_dns_server_search_order(new_dnss);
        }*/
    }
    if (_speed_duplexs_map.size()){
        macho::windows::registry reg;
        if (reg.open(boost::str(boost::wformat(L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\%04i") % config->index()))){
            if (reg[L"*SpeedDuplex"].exists() && reg[L"*SpeedDuplex"].is_string()){
                if (_speed_duplexs_map[_speed_duplexs.GetCurSel()] != reg[L"*SpeedDuplex"].wstring()){
                    macho::windows::network::adapter::ptr adapter = _adapters[_current_selection_index];
                    std::wstring disable_command = boost::str(boost::wformat(L"cmd.exe /C netsh set interface name=\"%s\" admin=DISABLED ") % adapter->net_connection_id());
                    std::wstring enable_command = boost::str(boost::wformat(L"cmd.exe /C netsh set interface name=\"%s\" admin=ENABLED ") % adapter->net_connection_id());
                    reg[L"*SpeedDuplex"] = _speed_duplexs_map[_speed_duplexs.GetCurSel()];
                    macho::windows::process::exec_console_application_with_retry(disable_command);
                    macho::windows::process::exec_console_application_with_retry(enable_command);
                }
            }
            reg.close();
        }
    }
    return result;
}

void CWizardNet::OnFieldchangedIpIpaddress(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
    // TODO: Add your control notification handler code here
    BYTE _nField0 = 0, _nField1 = 0, _nField2 = 0, _nField3 = 0;
    int cols = _address.GetAddress(_nField0, _nField1, _nField2, _nField3);
    if (cols == 4){
        if ( 0 == _subnet.GetAddress(_nField0, _nField1, _nField2, _nField3)){
            _subnet.SetAddress(255, 255, 255, 0);
        }
    }

    *pResult = 0;
}

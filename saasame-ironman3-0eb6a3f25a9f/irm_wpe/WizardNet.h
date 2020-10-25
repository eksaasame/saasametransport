#pragma once

#include "WizardPage.h"
#include "afxwin.h"
#include "afxcmn.h"

// CWizardNet dialog

class CWizardNet : public CWizardPage
{
	DECLARE_DYNAMIC(CWizardNet)

public:
    CWizardNet(winpe_settings & settings, SheetPos posPositionOnSheet = Middle);   // standard constructor
	virtual ~CWizardNet();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_NET };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    virtual BOOL OnInitDialog();
private:
    winpe_settings&                              _settings;
    macho::windows::network::adapter::vtr        _adapters;
    macho::windows::network::adapter_config::ptr _current_selection;
    CComboBox                                    _connections;
    CComboBox                                    _speed_duplexs;
    std::map<int, std::wstring>                  _speed_duplexs_map;
    CEdit                                        _mac;
    CButton                                      _auto_dhcp;
    CButton                                      _static_ip;
    CIPAddressCtrl                               _address;
    CIPAddressCtrl                               _subnet;
    CIPAddressCtrl                               _getway;
    CButton                                      _auto_dns;
    CButton                                      _static_dns;
    CIPAddressCtrl                               _dns1;
    CIPAddressCtrl                               _dns2;
    int                                          _current_selection_index;
    void show_network_config(macho::windows::network::adapter_config::ptr& config);
    bool apply_network_config(macho::windows::network::adapter_config::ptr& config);
public:
    afx_msg void OnClickedRadioAutoip();
    afx_msg void OnClickedRadioAutodns();
    afx_msg void OnRadioManuldns();
    afx_msg void OnRadioManulip();
    afx_msg void OnSelchangeComboConn();
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    afx_msg void OnFieldchangedIpIpaddress(NMHDR *pNMHDR, LRESULT *pResult);
};

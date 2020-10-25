#pragma once
#include "stdafx.h"
#include "WizardPage.h"
#include "afxwin.h"
#include "afxcmn.h"
// CWizardMgmt dialog

class CWizardMgmt : public CWizardPage
{
	DECLARE_DYNAMIC(CWizardMgmt)

public:
    CWizardMgmt(winpe_settings & settings, SheetPos posPositionOnSheet = Middle);   // standard constructor
	virtual ~CWizardMgmt();
    virtual BOOL OnWizardFinish();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_MGMT };
    
    CEdit           _mgmt_addr;
    CButton         _is_https;
    std::string     _machine_id;
    std::string     _addr;
    std::string     _session;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
    BOOL  RegisterServices();
    BOOL  GetPhysicalMachineInfo(physical_machine_info &MachineInfo, std::set<service_info> &service_infos, int port);

public:
    virtual BOOL OnInitDialog();
private:
    bool is_transport();
    CEdit _username;
    CEdit _password;
    CLinkCtrl _ClearBtn;
    std::shared_ptr<TSSLSocketFactory> _factory;
    winpe_settings& _settings;
public:
    afx_msg void OnClickSyslinkReset(NMHDR *pNMHDR, LRESULT *pResult);
private:
    CButton _keep_settings;
    CEdit   _hostname;
};

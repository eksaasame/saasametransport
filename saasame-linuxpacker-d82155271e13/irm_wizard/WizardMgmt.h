#pragma once
#pragma once
#include "stdafx.h"
#include "..\irm_wpe\WizardPage.h"
#include "afxwin.h"
#include "afxcmn.h"
// CWizardMgmt dialog

class CWizardMgmt : public CWizardPage
{
	DECLARE_DYNAMIC(CWizardMgmt)

public:
    CWizardMgmt(register_info& info, SheetPos posPositionOnSheet = Middle);   // standard constructor
    virtual ~CWizardMgmt();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_MGMT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
    BOOL  RegisterServices();
    BOOL  GetPhysicalMachineInfo(physical_machine_info &MachineInfo, std::set<service_info> &service_infos, int port);
    bool  is_transport();

    CEdit _mgmt_addr;
    CButton _is_https;
    CEdit _username;
    CEdit _password;
    CLinkCtrl _ClearBtn;
    register_info& _info;

public:
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    afx_msg void OnClickSyslinkReset(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL OnInitDialog();
private:
    CButton _is_allow_multiple;
};

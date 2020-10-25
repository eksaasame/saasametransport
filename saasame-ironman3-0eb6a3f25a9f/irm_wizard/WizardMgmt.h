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

    CEdit _mgmt_addr;
    CButton _is_https;
    CEdit _username;
    CLinkCtrl _ClearBtn;
    register_info& _info;

public:
    static bool  is_transport();
    virtual LRESULT OnWizardNext();
    virtual BOOL OnWizardFinish();
    afx_msg void OnClickSyslinkReset(NMHDR *pNMHDR, LRESULT *pResult);
    virtual BOOL OnInitDialog();
private:
    CButton _is_allow_multiple;
    std::shared_ptr<TSSLSocketFactory> _factory;
public:
    virtual BOOL OnSetActive();
private:
    CStatic _status;
public:
    afx_msg void OnChangeEdit();
};

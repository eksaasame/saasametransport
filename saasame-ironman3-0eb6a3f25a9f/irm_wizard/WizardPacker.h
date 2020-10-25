#pragma once
#include "stdafx.h"
#include "..\irm_wpe\WizardPage.h"
#include "afxwin.h"
#include "afxcmn.h"

class CWizardPacker : public CWizardPage
{
	DECLARE_DYNAMIC(CWizardPacker)

public:
    CWizardPacker(register_info& info, SheetPos posPositionOnSheet = Middle);   // standard constructor
	virtual ~CWizardPacker();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_PACKER };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
    BOOL GetPhysicalMachineInfo(std::string host, physical_machine_info &MachineInfo, service_info &Info);
    CEdit _packer;
    CListBox _packers;
    register_info& _info;
public:
    afx_msg void OnBnClickedBtnRegister();
};

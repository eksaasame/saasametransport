#pragma once

// CWizardSheet

class CWizardSheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CWizardSheet)
private:

public:
	CWizardSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CWizardSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	virtual ~CWizardSheet();

protected:
	DECLARE_MESSAGE_MAP()
public:
    afx_msg BOOL OnNcCreate(LPCREATESTRUCT lpCreateStruct);
    virtual BOOL OnInitDialog();
};



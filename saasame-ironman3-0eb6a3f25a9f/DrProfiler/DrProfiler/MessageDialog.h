#pragma once
#include "afxwin.h"


// CMessageDialog dialog

class CMessageDialog : public CDialogEx
{
	DECLARE_DYNAMIC(CMessageDialog)

public:
	CMessageDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CMessageDialog();

    void inline SetMessageText( CString message ) {
        m_txtMessage.SetWindowTextW(message);
    } 
// Dialog Data
	enum { IDD = IDD_DIALOG_PROCESSS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
    CStatic m_txtMessage;
public:
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
};

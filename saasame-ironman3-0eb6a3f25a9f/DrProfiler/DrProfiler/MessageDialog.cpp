// MessageDialog.cpp : implementation file
//

#include "stdafx.h"
#include "DrProfiler.h"
#include "MessageDialog.h"
#include "afxdialogex.h"


// CMessageDialog dialog

IMPLEMENT_DYNAMIC(CMessageDialog, CDialogEx)

CMessageDialog::CMessageDialog(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMessageDialog::IDD, pParent)
{

}

CMessageDialog::~CMessageDialog()
{
}

void CMessageDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_STATIC_MESSAGE, m_txtMessage);
}


BEGIN_MESSAGE_MAP(CMessageDialog, CDialogEx)
    ON_WM_CTLCOLOR()
END_MESSAGE_MAP()


// CMessageDialog message handlers


HBRUSH CMessageDialog::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    //HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

    // TODO:  Change any attributes of the DC here

    // TODO:  Return a different brush if the default is not desired
    return (HBRUSH)GetStockObject(WHITE_BRUSH);
}

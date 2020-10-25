// WizardSheet.cpp : implementation file
//

#include "stdafx.h"
#include "WizardSheet.h"


// CWizardSheet

IMPLEMENT_DYNAMIC(CWizardSheet, CPropertySheet)

CWizardSheet::CWizardSheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{

}

CWizardSheet::CWizardSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{

}

CWizardSheet::~CWizardSheet()
{
}


BEGIN_MESSAGE_MAP(CWizardSheet, CPropertySheet)
    ON_WM_NCCREATE()
END_MESSAGE_MAP()


// CWizardSheet message handlers


BOOL CWizardSheet::OnNcCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (!CPropertySheet::OnNcCreate(lpCreateStruct))
        return FALSE;

    // TODO:  Add your specialized creation code here
    ModifyStyle( 0, WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION, SWP_NOZORDER ); 

    return TRUE;
}


BOOL CWizardSheet::OnInitDialog()
{
    BOOL bResult = CPropertySheet::OnInitDialog();
    // TODO:  Add your specialized code here
    CMenu* mnu = this->GetSystemMenu(FALSE);
    if ( mnu )
        mnu->ModifyMenu(SC_CLOSE,MF_BYCOMMAND | MF_GRAYED );
    //HICON icon = LoadIcon( AfxGetApp()-> m_hInstance, MAKEINTRESOURCE(IDR_MAINFRAME) );
    //SetIcon( icon, FALSE );
    //SetIcon( icon, TRUE );
    return bResult;
}

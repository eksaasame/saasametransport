// WizardPage.cpp : implementation file
//

#include "stdafx.h"
#include "WizardPage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//  WizardPage.cpp - Definition der Klasse
//
//  Beschreibung:
//     Von CPropertyPage abgeleitete Klasse zur Erstellung von
//     Wizards. Durch den ersten im Konstruktur übergebenen Parameter
//     können die Navigationsbuttons, entsprechend ihrer Position im Wizard,
//	   richtig konfiguriert werden.
//	
//     Mögliche Konfigurationen: Only, First, Middle, Last
//
//  History:   Datum        Autor			Kommentar
//             2000-10-15   Pisoftware/RP   Erstellung
//             2001-12-12   Pisoftware/AH	Erweiterung SetWindowText
//
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CWizardPage, CPropertyPage)

CWizardPage::CWizardPage( SheetPos posPositionOnSheet, UINT nIDTemplate, UINT nIDCaption )
	: m_posPositionOnSheet(posPositionOnSheet),
	  CPropertyPage( nIDTemplate, nIDCaption )
{
	//{{AFX_DATA_INIT(CWizardPage)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
    this->m_psp.dwFlags &= ~PSP_HASHELP;;
}

CWizardPage::~CWizardPage()
{
}

void CWizardPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizardPage)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizardPage, CPropertyPage)
	//{{AFX_MSG_MAP(CWizardPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizardPage message handlers

BOOL CWizardPage::OnSetActive() 
{
	//Hole den Dialog
	CPropertySheet* pSheet = (CPropertySheet*)GetParent();
	ASSERT_KINDOF(CPropertySheet, pSheet);

	switch ( m_posPositionOnSheet )
	{
		//zeige nur den "Fertigstellen" Button bei nur einer Seite
		case Only:
			pSheet->SetWizardButtons( PSWIZB_FINISH );
			break;

		//zeige den "Zurück" Button nicht auf der ersten Seite
		case First:
		pSheet->SetWizardButtons( PSWIZB_NEXT );
			break;

		//zeige "Zurück" und "Weiter" Buttons in der Mitte
		case Middle:
			pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
			break;

		case Last:
		//zeige "Zurück" und "Fertigstellen" Buttons auf der letzten Seite
			pSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
			break;

	}

	return CPropertyPage::OnSetActive();
}

BOOL CWizardPage::OnWizardFinish() 
{
	//das vergisst die MFC sonst:
	if ( OnKillActive() != TRUE )
		return FALSE;

/*
	//die MS KB schlägt folgenden Code vor	
	if (!UpdateData())  //note: parameter is TRUE by default
	{
		TRACE0("UpdateData failed during wizard finish\n");
		return FALSE;
	}
*/

	return CPropertyPage::OnWizardFinish();

}

void CWizardPage::SetWindowText(LPCTSTR lpszString)
{
    // kennzeichnet als Titel
	this->m_psp.dwFlags |= PSP_USETITLE;
	
	// schreibt den String
	this->m_psp.pszTitle =  lpszString ;

}

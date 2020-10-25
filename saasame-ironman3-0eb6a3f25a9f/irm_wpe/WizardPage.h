#if !defined(AFX_WIZARDPAGE_H__165396A3_A266_11D4_BE85_00E0296A8EEF__INCLUDED_)
#define AFX_WIZARDPAGE_H__165396A3_A266_11D4_BE85_00E0296A8EEF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// WizardPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
//  WizardPage.h - Deklaration der Klasse
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

class CWizardPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CWizardPage)

public:
	enum SheetPos { Only = 0, First=1, Middle=2, Last=3 };


// Construction
public:
	CWizardPage( SheetPos posPositionOnSheet = Middle, UINT nIDTemplate = 0, UINT nIDCaption = 0 );
	~CWizardPage();

	inline SheetPos GetPosition();
	inline void SetPosition( SheetPos value );

	void SetWindowText( LPCTSTR lpszString );

// Dialog Data
	//{{AFX_DATA(CWizardPage)
	enum { IDD = 0 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizardPage)
	public:
	virtual BOOL OnSetActive();
	virtual BOOL OnWizardFinish();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizardPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	SheetPos	m_posPositionOnSheet;

};

inline CWizardPage::SheetPos CWizardPage::GetPosition()
{
	return m_posPositionOnSheet;
}

inline void CWizardPage::SetPosition( SheetPos value )
{
	m_posPositionOnSheet = value;
}

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZARDPAGE_H__165396A3_A266_11D4_BE85_00E0296A8EEF__INCLUDED_)

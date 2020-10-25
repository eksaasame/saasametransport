
// DrProfilerDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "WndResizer.h"
// CDrProfilerDlg dialog
class CDrProfilerDlg : public CDialogEx
{
// Construction
public:
	CDrProfilerDlg( ProfileModule& module, CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DRPROFILER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
    
	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private :
	CWndResizer										 m_resizer;
    ProfileModule&                                   m_module;
    CMessageDialog*                                  m_pMsgDialog;
    CImageList*                                      m_pImageList;  
    SP_CLASSIMAGELIST_DATA                           m_classImageListData;
    CColumnTreeCtrl                                  m_columnTree;
    CButton                                          m_btnSave;
    CStatic                                          m_version;
    std::map<stdstring, hardware_device::ptr, stringutils::no_case_string_less>  m_devices_map;
    int                                              image;
    void                                             ShowDevice( PROFILE_STATE_ENUM state, hardware_class::ptr& c, hardware_device::ptr& device, hardware_driver::ptr& driver );
    void                                             SaveProfile( PROFILE_STATE_ENUM state, hardware_device::ptr& device, hardware_driver::ptr& driver, stdstring& file_name, stdstring& err );
    void                                             CheckTreeViewItmes( BOOL fCheck = TRUE );
    std::map<stdstring, HTREEITEM, stringutils::no_case_string_less> m_classes;
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedBtnSelectAll();
    afx_msg void OnBnClickedBtnUnselectAll();
private:
    CButton m_btnSelectAll;
    CButton m_btnUnselectAll;
    bool    m_bRunning;
    
public:
    afx_msg void OnMove(int x, int y);
};

#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "boost\thread.hpp"
#include "boost\signals2\signal.hpp"

// CReadyDlg dialog

class CReadyDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CReadyDlg)

public:
	CReadyDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CReadyDlg();

// Dialog Data
	enum { IDD = IDD_DIALOG_READY };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
private:
    CComboBox _networks;
    CIPAddressCtrl _address;
    macho::windows::network::adapter::vtr        _adapters;
    macho::windows::network::adapter_config::ptr _current_selection;
    void show_network_config(macho::windows::network::adapter_config::ptr& config); 
    void                check_recovery_status();
    boost::thread       _thread;
    CButton             _reboot_btn;
    bool                _terminated;
public:
    afx_msg void OnBnClickedOk();
    virtual BOOL OnInitDialog();
    afx_msg void OnSelchangeComboNet();
    afx_msg void OnNMClickCmd(NMHDR *pNMHDR, LRESULT *pResult);
private:
    CStatic _hostname;
};

#pragma once

#include "WizardPage.h"
#include "afxcmn.h"
#include "macho.h"
// CWizardStart dialog

class CWizardStart : public CWizardPage
{
	DECLARE_DYNAMIC(CWizardStart)

public:
    CWizardStart(winpe_settings & settings, SheetPos posPositionOnSheet = Middle, bool is_rcd = true);   // standard constructor
	virtual ~CWizardStart();

// Dialog Data
	enum { IDD = IDD_PROPPAGE_START };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
public:

    afx_msg void OnBnClickedBtnInsert();
private:
    winpe_settings&                                                                                     _settings;
    bool                                                                                                _is_rcd;
    macho::windows::hardware_class::vtr                                                                 _classes;
    macho::windows::hardware_device::vtr                                                                _devices;
    std::wstring                                                                                        _computer_name;
    CImageList*                                                                                         _p_image_list;
    SP_CLASSIMAGELIST_DATA                                                                              _class_image_list_data;
    CTreeCtrl                                                                                           _devices_tree;
    std::map<std::wstring, bool, macho::stringutils::no_case_string_less_w>                             _display_classes;
    std::map<std::wstring, std::vector<std::wstring>, macho::stringutils::no_case_string_less_w>        _hidden_class_devices;
    macho::windows::device_manager                                                                      _dev_mgmt;
    bool show_devices_tree();
    HTREEITEM add_tree_item(HTREEITEM parent, macho::windows::hardware_device::ptr &device, bool need_expand);
    bool driver_package_install(std::wstring driver_package_inf_path, bool is_force);
    std::wstring get_error_message(DWORD message_id);
public:
    virtual BOOL OnInitDialog();
    virtual LRESULT OnWizardNext();
    afx_msg void OnRclickDevicestree(NMHDR *pNMHDR, LRESULT *pResult);
};

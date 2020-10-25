#pragma once
#include "afxcmn.h"

#include "macho.h"
// DeviceProperties dialog

class DeviceProperties : public CDialogEx
{
	DECLARE_DYNAMIC(DeviceProperties)

public:
    DeviceProperties(macho::windows::hardware_device & device, macho::windows::hardware_driver::ptr driver, CWnd* pParent = NULL);   // standard constructor
	virtual ~DeviceProperties();

// Dialog Data
	enum { IDD = IDD_DEVICE_PROPERTIES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	DECLARE_MESSAGE_MAP()
private:
    CTreeCtrl _properties;
    macho::windows::hardware_device                                                 &_device;
    macho::windows::hardware_driver::ptr                                            _driver;
    std::map<std::wstring, std::wstring, macho::stringutils::no_case_string_less_w> _strings;
public:
    virtual BOOL OnInitDialog();
};

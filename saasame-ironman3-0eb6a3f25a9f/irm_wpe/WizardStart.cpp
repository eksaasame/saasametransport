// WizardStart.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wpe.h"
#include "WizardStart.h"
#include "DeviceProperties.h"
#include "afxdialogex.h"
#include <devguid.h>
#include "difxapi.h"

#pragma comment( lib, "difxapi.lib" )

using namespace macho::windows;

// CWizardStart dialog

IMPLEMENT_DYNAMIC(CWizardStart, CPropertyPage)

CWizardStart::CWizardStart(winpe_settings & settings, SheetPos posPositionOnSheet, bool is_rcd)
: CWizardPage(posPositionOnSheet, CWizardStart::IDD), _is_rcd(is_rcd), _settings(settings)
{
}

CWizardStart::~CWizardStart()
{
}

void CWizardStart::DoDataExchange(CDataExchange* pDX)
{
    CWizardPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DEVICESTREE, _devices_tree);
}

BEGIN_MESSAGE_MAP(CWizardStart, CWizardPage)
    ON_BN_CLICKED(IDC_BTN_INSERT, &CWizardStart::OnBnClickedBtnInsert)
    ON_NOTIFY(NM_RCLICK, IDC_DEVICESTREE, &CWizardStart::OnRclickDevicestree)
END_MESSAGE_MAP()

// CWizardStart message handlers

void CWizardStart::OnBnClickedBtnInsert()
{
    // TODO: Add your control notification handler code here
    //drvload 
    CString szFilter = _T("Inf File(*.inf)|*.inf||");
    CFileDialog filedlg(TRUE, NULL, NULL,
        OFN_HIDEREADONLY,
          szFilter, NULL);
    filedlg.m_ofn.lpstrInitialDir = environment::get_working_directory().c_str();
    if (filedlg.DoModal() == IDOK) {
        if (driver_package_install((LPWSTR)filedlg.GetPathName().GetString(), true)){
            _devices_tree.DeleteAllItems();
            show_devices_tree();
        }
    }
}

bool CWizardStart::show_devices_tree()
{
    _classes = _dev_mgmt.get_classes();
    _devices = _dev_mgmt.get_devices();

#if 1
    TVINSERTSTRUCT tvInsertItem;
    int image;
    memset(&tvInsertItem, 0, sizeof(TVINSERTSTRUCT));
    SetupDiGetClassImageIndex(&_class_image_list_data, &GUID_DEVCLASS_COMPUTER, &image);

    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvInsertItem.item.iImage = image;
    tvInsertItem.item.iSelectedImage = image;
    tvInsertItem.item.pszText = (LPWSTR)_computer_name.c_str();

    HTREEITEM rootItem = _devices_tree.InsertItem(&tvInsertItem);
    _devices_tree.SetItemState(rootItem, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);

    foreach(hardware_class::ptr &c, _classes){
        HTREEITEM classItem = NULL;
        if (!_display_classes.count(c->name))
            continue;
        std::vector<std::wstring> _hiddens;
        if (_hidden_class_devices.count(c->name))
            _hiddens = _hidden_class_devices[c->name];

        hardware_device::vtr class_devices;
        foreach(hardware_device::ptr &d, _devices){
            if (d->sz_class_guid.length()){
                if (macho::guid_(d->sz_class_guid) == c->guid){
                    bool is_hidden = false;
                    foreach(std::wstring &_hidden, _hiddens){
                        if (_hidden.length() &&
                            (!_tcsicmp(_hidden.c_str(), d->sz_class.c_str()) ||
                            !_tcsicmp(_hidden.c_str(), d->enumeter.c_str()))){
                            is_hidden = true;
                            break;
                        }
                    }
                    if (!is_hidden){
                        if (NULL == classItem){
                            _display_classes[c->name] = true;
                            SetupDiGetClassImageIndex(&_class_image_list_data, &c->guid, &image);
                            tvInsertItem.hParent = rootItem;
                            tvInsertItem.item.iImage = image;
                            tvInsertItem.item.iSelectedImage = image;
                            tvInsertItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                            tvInsertItem.item.pszText = (LPWSTR)c->description.c_str();
                            classItem = _devices_tree.InsertItem(&tvInsertItem);
                            _devices_tree.SetItemState(classItem, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);
                        }
                        tvInsertItem.hParent = classItem;
                        tvInsertItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_STATE | TVIF_PARAM | TVIF_SELECTEDIMAGE;
                        tvInsertItem.item.pszText = (LPWSTR)d->device_description.c_str();
                        tvInsertItem.item.lParam = (LPARAM)d.get();
                        tvInsertItem.item.stateMask = TVIS_OVERLAYMASK;
                        tvInsertItem.item.state = 0;
                        if (d->has_problem()) {
                            if (d->is_disabled()){
                                tvInsertItem.item.state = INDEXTOOVERLAYMASK(
                                    IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
                            }
                            else{
                                tvInsertItem.item.state = INDEXTOOVERLAYMASK(
                                    IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
                            }
                        }
                        HTREEITEM devItem = _devices_tree.InsertItem(&tvInsertItem);
                        _devices_tree.SetItemState(devItem, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);
                    }
                }
            }
        }
        if (NULL != classItem)
            _devices_tree.Expand(classItem, TVE_EXPAND);
    }
#else
    bool bNeedExpand = false;
    hardware_device::ptr root_device = _dev_mgmt.get_root_device();
    HTREEITEM rootItem = add_tree_item(TVI_ROOT, root_device, bNeedExpand);
    _devices_tree.SetItemState(rootItem, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);
#endif
    _devices_tree.Expand(rootItem, TVE_EXPAND);

    return false;
}

HTREEITEM CWizardStart::add_tree_item(HTREEITEM parent, macho::windows::hardware_device::ptr &device, bool need_expand)
{
    HTREEITEM       hCurrentItem = NULL;
    TVINSERTSTRUCT  tvInsertItem;
    int             image = 0;
    macho::guid_    guid;

    memset(&tvInsertItem, 0, sizeof(TVINSERTSTRUCT));

    if (device->sz_class.length())
        guid = device->sz_class_guid;
    else
        guid = GUID_DEVCLASS_COMPUTER;
    std::vector<std::wstring> _hiddens;
    if (_hidden_class_devices.count(device->sz_class))
        _hiddens = _hidden_class_devices[device->sz_class];
    foreach(std::wstring &_hidden, _hiddens){
        if ( (!_tcsicmp(_hidden.c_str(), device->sz_class.c_str()) ||
            !_tcsicmp(_hidden.c_str(), device->enumeter.c_str()))){
            return NULL;
        }
    }
    
    SetupDiGetClassImageIndex(&_class_image_list_data, &((GUID)guid), &image);
    tvInsertItem.hParent = parent;
    tvInsertItem.item.iImage = image;
    tvInsertItem.item.iSelectedImage = image;
    tvInsertItem.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_STATE;
    tvInsertItem.item.stateMask = TVIS_OVERLAYMASK;

    if (device->has_problem()){
        if (device->is_disabled()){
            tvInsertItem.item.state = INDEXTOOVERLAYMASK(
                IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
        }
        else{
            tvInsertItem.item.state = INDEXTOOVERLAYMASK(
                IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);
        }
    }

    if (TVI_ROOT == parent)
        tvInsertItem.item.pszText = (LPWSTR)_computer_name.c_str();
    else{
        tvInsertItem.item.lParam = (LPARAM)device.get();
        tvInsertItem.item.pszText = (LPWSTR)device->device_description.c_str();
    }
    hCurrentItem = _devices_tree.InsertItem(&tvInsertItem);
    foreach(std::wstring &c, device->childs){
        foreach(hardware_device::ptr &d, _devices){
            if (d->device_instance_id == c){
                HTREEITEM hChildItem = add_tree_item(hCurrentItem, d, true);
                _devices_tree.SetItemState(hChildItem, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);
                break;
            }
        }
    }
    return hCurrentItem;
}

BOOL CWizardStart::OnInitDialog()
{
    CWizardPage::OnInitDialog();
    _computer_name = macho::windows::environment::get_computer_name();
    _class_image_list_data.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (SetupDiGetClassImageList(&_class_image_list_data) == TRUE) {
        _p_image_list = CImageList::FromHandle(_class_image_list_data.ImageList);
        _devices_tree.SetImageList(_p_image_list, TVSIL_NORMAL);
    }
    // TODO:  Add extra initialization here
    _hidden_class_devices[_T("LegacyDriver")].push_back(_T("LegacyDriver"));
    _hidden_class_devices[_T("Volume")].push_back(_T("Volume"));
    _hidden_class_devices[_T("System")].push_back(_T("Root"));
    _hidden_class_devices[_T("Net")].push_back(_T("Root"));
    _hidden_class_devices[_T("SCSIAdapter")].push_back(_T("Root"));
    _hidden_class_devices[_T("Display")].push_back(_T("Root"));
    _hidden_class_devices[_T("Processor")].push_back(_T("Processor"));
    _hidden_class_devices[_T("Net")].push_back(_T("SW"));
    _hidden_class_devices[_T("Net")].push_back(_T("SWD"));
    //_hidden_class_devices[_T("DiskDrive")].push_back( _T("DiskDrive"));
    //_hidden_class_devices[_T("HDC")].push_back(_T("PCIIDE"));

    _display_classes[_T("USBDevice")] = false;
    _display_classes[_T("hdc")] = false;
    _display_classes[_T("Net")] = false;
    _display_classes[_T("DiskDrive")] = false;
    _display_classes[_T("SCSIAdapter")] = false;
    _display_classes[_T("Unknown")] = false;
    if (_is_rcd){
        macho::windows::com_init com;
        wmi_services wmi;
        wmi.connect(L"CIMV2");
        uint64_t physical_memory;
        wmi_object computer_system = wmi.query_wmi_object(L"Win32_ComputerSystem");
        wmi_object_table physical_memorys = wmi.query_wmi_objects(L"Win32_PhysicalMemory");
        if (physical_memorys.size()){
            int64_t total_physical_mamory = 0;
            foreach(wmi_object physical_memory, physical_memorys)
                total_physical_mamory += (int64_t)physical_memory[L"Capacity"];
            physical_memory = total_physical_mamory / (1024 * 1024);
        }
        else{
            physical_memory = computer_system[L"TotalPhysicalMemory"];
        }
        if (physical_memory < 2048){
            AfxMessageBox(L"The memory must be equal to or greater than 2GB to recovery Linux instance.");
        }
    }
    show_devices_tree();

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}


LRESULT CWizardStart::OnWizardNext()
{
    // TODO: Add your specialized code here and/or call the base class
    if (_display_classes[_T("DiskDrive")] && 
        (_display_classes[_T("hdc")] || _display_classes[_T("SCSIAdapter")]) && 
        _display_classes[_T("Net")] )
        return CWizardPage::OnWizardNext();   
    std::wstring message;
    if (!_display_classes[_T("Net")])
        message = L"Please make sure the network adapter driver is ready!";
    else if (_display_classes[_T("hdc")] || _display_classes[_T("SCSIAdapter")])
        message = L"Please make sure the SCSI adapter driver or IDE device driver is ready!";
    else        
        message = L"Please make sure you have any disk for operation!";
    AfxMessageBox(message.c_str(), MB_ICONSTOP | MB_OK);
    return -1;
}

void CWizardStart::OnRclickDevicestree(NMHDR *pNMHDR, LRESULT *pResult)
{
    // TODO: Add your control notification handler code here
    CMenu menu;
    menu.CreatePopupMenu();
    HTREEITEM item = NULL;
    UINT flags;
    // verify that we have a mouse click in the check box area
    DWORD pos = GetMessagePos();
    CPoint point(LOWORD(pos), HIWORD(pos));
    _devices_tree.ScreenToClient(&point);
    item = _devices_tree.HitTest(point, &flags);

    if (!item)
        item = _devices_tree.GetSelectedItem();
    else
        _devices_tree.SelectItem(item);

    hardware_device* hw = (hardware_device*)_devices_tree.GetItemData(item);

    if (hw){
        ULONG ulState = _devices_tree.GetItemState(item, TVIS_STATEIMAGEMASK) >> 12;
        menu.AppendMenu(MF_STRING, 1, _T("Properties"));
        CPoint pt;
        GetCursorPos(&pt);
        int ret = menu.TrackPopupMenu(TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, this);
        switch (ret){
        case 1:
            hardware_driver::ptr driver = _dev_mgmt.get_device_driver(*hw);
            DeviceProperties page(*hw, driver, this);
            page.DoModal();
            break;
        }
    }
    *pResult = 0;
}


std::wstring CWizardStart::get_error_message(DWORD message_id){
    std::wstring str;
    switch (message_id){
    case CERT_E_EXPIRED:
        str = L"The signing certificate is expired.";
        break;
    case CERT_E_UNTRUSTEDROOT:
        str = L"The catalog file has an Authenticode signature whose certificate chain terminates in a root certificate that is not trusted.";
        break;
    case CERT_E_WRONG_USAGE:
        str = L"The certificate for the driver package is not valid for the requested usage. If the driver package does not have a valid WHQL signature, DriverPackageInstall returns this error if, in response to a driver signing dialog box, the user chose not to install a driver package, or if the caller specified the DRIVER_PACKAGE_SILENT flag.";
        break;
    case CRYPT_E_FILE_ERROR:
        str = L"The catalog file for the specified driver package was not found; or possibly, some other error occurred when DriverPackageInstall tried to verify the driver package signature.";
        break;
    case ERROR_ACCESS_DENIED:
        str = L"A caller of DriverPackageInstall must be a member of the Administrators group to install a driver package.";
        break;
    case ERROR_BAD_ENVIRONMENT:
        str = L"The current Microsoft Windows version does not support this operation. An old or incompatible version of DIFxApp.dll or DIFxAppA.dll might be present in the system. For more information about these .dll files, see How DIFxApp Works.";
        break;
    case ERROR_CANT_ACCESS_FILE:
        str = L"DriverPackageInstall could not preinstall the driver package because the specified INF file is in the system INF file directory.";
        break;
    case ERROR_FILE_NOT_FOUND:
        str = L"The INF file that was specified by DriverPackageInfPath was not found.";
        break;
    case ERROR_FILENAME_EXCED_RANGE:
        str = L"The INF file path, in characters, that was specified by DriverPackageInfPath is greater than the maximum supported path length. For more information about path length, see Specifying a Driver Package INF File Path.";
        break;
#ifdef ERROR_IN_WOW64
    case ERROR_IN_WOW64:
        str = L"The 32-bit version DIFxAPI does not work on Win64 systems. A 64-bit version of DIFxAPI is required.";
        break;
#endif
    case ERROR_INSTALL_FAILURE:
        str = L"The installation failed.";
        break;
    case ERROR_INVALID_CATALOG_DATA:
        str = L"The catalog file for the specified driver package is not valid or was not found.";
        break;
    case ERROR_INVALID_NAME:
        str = L"The specified INF file path is not valid.";
        break;
    case ERROR_INVALID_PARAMETER:
        str = L"A supplied parameter is not valid.";
        break;
    case ERROR_NO_DEVICE_ID:
        str = L"The driver package does not specify a hardware identifier or compatible identifier that is supported by the current platform.";
        break;
    case ERROR_NO_MORE_ITEMS:
        str = L"The specified driver package was not installed for matching devices because the driver packages already installed for the matching devices are a better match for the devices than the specified driver package.";
        break;
    case ERROR_NO_SUCH_DEVINST:
        str = L"The driver package was not installed on any device because there are no matching devices in the device tree.";
        break;
    case ERROR_OUTOFMEMORY:
        str = L"Available system memory was insufficient to perform the operation.";
        break;
    case ERROR_SHARING_VIOLATION:
        str = L"A component of the driver package in the DIFx driver store is locked by a thread or process. This error can occur if a process or thread, other than the thread or process of the caller, is currently accessing the same driver package as the caller.";
        break;
    case ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH:
        str = L"The signing certificate is not valid for the current Windows version or it is expired.";
        break;
    case ERROR_UNSUPPORTED_TYPE:
        str = L"The driver package type is not supported.";
        break;
    case TRUST_E_NOSIGNATURE:
        str = L"The driver package is not signed.";
        break;
    }
    return str;
}

bool CWizardStart::driver_package_install(std::wstring driver_package_inf_path, bool is_force){
    LPTSTR _driver_package_inf_path = (LPTSTR)driver_package_inf_path.c_str();  // An INF file for PnP driver package
    DWORD Flags = DRIVER_PACKAGE_ONLY_IF_DEVICE_PRESENT | DRIVER_PACKAGE_SILENT | DRIVER_PACKAGE_LEGACY_MODE;
    if (is_force)
        Flags |= DRIVER_PACKAGE_FORCE;

    INSTALLERINFO *pAppInfo = NULL;      // No application association
    BOOL NeedReboot = FALSE;
    DWORD ReturnCode = DriverPackageInstall(_driver_package_inf_path, Flags, pAppInfo, &NeedReboot);

    if (ERROR_SUCCESS != ReturnCode){
        std::wstring str = get_error_message(ReturnCode);
        AfxMessageBox(str.c_str(), MB_OK | MB_ICONEXCLAMATION);
    }
    else{
        return true;
    }
    return false;
}
// DeviceProperties.cpp : implementation file
//

#include "stdafx.h"
#include "irm_wpe.h"
#include "DeviceProperties.h"
#include "afxdialogex.h"


// DeviceProperties dialog

IMPLEMENT_DYNAMIC(DeviceProperties, CDialogEx)

DeviceProperties::DeviceProperties(macho::windows::hardware_device & device, macho::windows::hardware_driver::ptr driver, CWnd* pParent /*=NULL*/)
	: _device(device),
    _driver(driver),
    CDialogEx(DeviceProperties::IDD, pParent)
{

}

DeviceProperties::~DeviceProperties()
{
}

void DeviceProperties::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_TREE_PROPERTIES, _properties);
}


BEGIN_MESSAGE_MAP(DeviceProperties, CDialogEx)
END_MESSAGE_MAP()


// DeviceProperties message handlers


BOOL DeviceProperties::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // TODO:  Add extra initialization here
    HTREEITEM       hFirstItem = NULL;
    HTREEITEM       hCurrentItem = NULL;
    HTREEITEM       hParentItem = NULL;
    TVINSERTSTRUCT  tvInsertItem;

    _strings[_T("Device Description")] = _T("Device Description");
    memset(&tvInsertItem, 0, sizeof(TVINSERTSTRUCT));
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Device Description")].c_str(); //Device Description
    hParentItem = _properties.InsertItem(&tvInsertItem);
    hFirstItem = hParentItem;
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.device_description.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    _strings[_T("Device Instance Id")] = _T("Device Instance Id");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Device Instance Id")].c_str(); //Device InstanceId       
    hParentItem = _properties.InsertItem(&tvInsertItem);
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.device_instance_id.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    _strings[_T("Hardware IDs")] = _T("Hardware IDs");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Hardware IDs")].c_str(); //Device Inf Path
    hParentItem = _properties.InsertItem(&tvInsertItem);
    for (size_t i = 0; i < _device.hardware_ids.size(); ++i){
        tvInsertItem.hParent = hParentItem;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_device.hardware_ids[i].c_str();
        hCurrentItem = _properties.InsertItem(&tvInsertItem);
    }
    _properties.Expand( hParentItem, TVE_EXPAND );

    _strings[_T("Compatible IDs")] = _T("Compatible IDs");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Compatible IDs")].c_str(); //Device Inf Path
    hParentItem = _properties.InsertItem(&tvInsertItem);
    for (size_t i = 0; i < _device.compatible_ids.size(); ++i){
        tvInsertItem.hParent = hParentItem;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_device.compatible_ids[i].c_str();
        hCurrentItem = _properties.InsertItem(&tvInsertItem);
    }
    _properties.Expand( hParentItem, TVE_EXPAND );

    _strings[_T("Class")] = _T("Class");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Class")].c_str(); //Device Inf Path
    hParentItem = _properties.InsertItem(&tvInsertItem);
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.sz_class.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    _strings[_T("Class Guid")] = _T("Class Guid");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Class Guid")].c_str(); //Device Inf Path
    hParentItem = _properties.InsertItem(&tvInsertItem);
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.sz_class_guid.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    _strings[_T("Manufacture")] = _T("Manufacture");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Manufacture")].c_str(); //Device Inf Path
    hParentItem = _properties.InsertItem(&tvInsertItem);
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.manufacturer.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    _strings[_T("Enumeter")] = _T("Enumeter");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Enumeter")].c_str(); //Device Inf Path
    hParentItem = _properties.InsertItem(&tvInsertItem);
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.enumeter.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    _strings[_T("Inf Name")] = _T("Inf Name");
    tvInsertItem.hParent = TVI_ROOT;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Inf Name")].c_str(); //Device Inf Path 
    hParentItem = _properties.InsertItem(&tvInsertItem);
    tvInsertItem.hParent = hParentItem;
    tvInsertItem.item.mask = TVIF_TEXT;
    tvInsertItem.item.pszText = (LPWSTR)_device.inf_name.c_str();
    hCurrentItem = _properties.InsertItem(&tvInsertItem);
    _properties.Expand(hParentItem, TVE_EXPAND);

    if (_driver){
        _strings[_T("Original Inf Name")] = _T("Original Inf Name");
        tvInsertItem.hParent = TVI_ROOT;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Original Inf Name")].c_str(); //Device Inf Path 
        hParentItem = _properties.InsertItem(&tvInsertItem);
        tvInsertItem.hParent = hParentItem;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_driver->original_inf_name.c_str();
        hCurrentItem = _properties.InsertItem(&tvInsertItem);
        _properties.Expand(hParentItem, TVE_EXPAND);

        if (_driver->original_catalog_name.length()){
            _strings[_T("Catalog File")] = _T("Catalog File");
            tvInsertItem.hParent = TVI_ROOT;
            tvInsertItem.item.mask = TVIF_TEXT;
            tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Catalog File")].c_str(); //Device Inf Path
            hParentItem = _properties.InsertItem(&tvInsertItem);
            tvInsertItem.hParent = hParentItem;
            tvInsertItem.item.mask = TVIF_TEXT;
            tvInsertItem.item.pszText = (LPWSTR)_driver->original_catalog_name.c_str();
            hCurrentItem = _properties.InsertItem(&tvInsertItem);
            _properties.Expand(hParentItem, TVE_EXPAND);
        }
        
        _strings[_T("Driver Version")] = _T("Driver Version");
        tvInsertItem.hParent = TVI_ROOT;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Driver Version")].c_str(); //Device Inf Path 
        hParentItem = _properties.InsertItem(&tvInsertItem);
        tvInsertItem.hParent = hParentItem;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_driver->driver_version.c_str();
        hCurrentItem = _properties.InsertItem(&tvInsertItem);
        _properties.Expand(hParentItem, TVE_EXPAND);

        _strings[_T("Driver Date")] = _T("Driver Date");
        tvInsertItem.hParent = TVI_ROOT;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Driver Date")].c_str(); //Device Inf Path 
        hParentItem = _properties.InsertItem(&tvInsertItem);
        tvInsertItem.hParent = hParentItem;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_driver->driver_date.c_str();
        hCurrentItem = _properties.InsertItem(&tvInsertItem);
        _properties.Expand(hParentItem, TVE_EXPAND);

        _strings[_T("Matching Device Id")] = _T("Matching Device Id");
        tvInsertItem.hParent = TVI_ROOT;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Matching Device Id")].c_str(); //Device Inf Path 
        hParentItem = _properties.InsertItem(&tvInsertItem);
        tvInsertItem.hParent = hParentItem;
        tvInsertItem.item.mask = TVIF_TEXT;
        tvInsertItem.item.pszText = (LPWSTR)_driver->matching_device_id.c_str();
        hCurrentItem = _properties.InsertItem(&tvInsertItem);
        _properties.Expand(hParentItem, TVE_EXPAND);

        if (_driver->driver_files.size()){
            _strings[_T("Driver Files")] = _T("Driver Files");
            tvInsertItem.hParent = TVI_ROOT;
            tvInsertItem.item.mask = TVIF_TEXT;
            tvInsertItem.item.pszText = (LPWSTR)_strings[_T("Driver Files")].c_str(); //Device Inf Path
            hParentItem = _properties.InsertItem(&tvInsertItem);
            for (size_t i = 0; i < _driver->driver_files.size(); ++i){
                tvInsertItem.hParent = hParentItem;
                tvInsertItem.item.mask = TVIF_TEXT;
                tvInsertItem.item.pszText = (LPWSTR)_driver->driver_files[i]->target.c_str();
                hCurrentItem = _properties.InsertItem(&tvInsertItem);
            }
            _properties.Expand(hParentItem, TVE_EXPAND);
        }
    }
    _properties.EnsureVisible(hFirstItem);

    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

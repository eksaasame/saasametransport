
// DrProfilerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "DrProfiler.h"
#include "DrProfilerDlg.h"
#include "afxdialogex.h"
#include "shlobj.h"
#include "..\..\buildenv.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDrProfilerDlg dialog

using namespace macho::windows;


CDrProfilerDlg::CDrProfilerDlg( ProfileModule& module, CWnd* pParent /*=NULL*/)
	: CDialogEx(CDrProfilerDlg::IDD, pParent), m_bRunning(false), m_module(module), m_pMsgDialog(NULL)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDrProfilerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DEVICES_TREE, m_columnTree);
    DDX_Control(pDX, IDOK, m_btnSave);
    DDX_Control(pDX, IDC_BTN_SELECT_ALL, m_btnSelectAll);
    DDX_Control(pDX, IDC_BTN_UNSELECT_ALL, m_btnUnselectAll);
    DDX_Control(pDX, IDC_STATIC_VERSION, m_version);
}

BEGIN_MESSAGE_MAP(CDrProfilerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDOK, &CDrProfilerDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &CDrProfilerDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BTN_SELECT_ALL, &CDrProfilerDlg::OnBnClickedBtnSelectAll)
    ON_BN_CLICKED(IDC_BTN_UNSELECT_ALL, &CDrProfilerDlg::OnBnClickedBtnUnselectAll)
    ON_WM_MOVE()
END_MESSAGE_MAP()


// CDrProfilerDlg message handlers

BOOL CDrProfilerDlg::OnInitDialog()
{

	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
    m_pMsgDialog = new CMessageDialog( this );
    m_pMsgDialog->Create(IDD_DIALOG_PROCESSS);
	// TODO: Add extra initialization here

	BOOL bOk = FALSE;
	bOk = m_resizer.Hook(this);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_DEVICES_TREE, ANCHOR_RIGHT | ANCHOR_BOTTOM | ANCHOR_LEFT | ANCHOR_TOP);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_STATIC_VERSION, ANCHOR_TOP | ANCHOR_RIGHT);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_STATIC_TXT, ANCHOR_TOP | ANCHOR_LEFT);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_RADIO_EXE, ANCHOR_BOTTOM | ANCHOR_LEFT);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_RADIO_CAB, ANCHOR_BOTTOM | ANCHOR_LEFT);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_RADIO_FOLDER, ANCHOR_BOTTOM | ANCHOR_LEFT);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_BTN_SELECT_ALL, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_BTN_UNSELECT_ALL, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDC_STATIC_GROUP, ANCHOR_RIGHT | ANCHOR_BOTTOM | ANCHOR_LEFT );
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDOK, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk);

	bOk = m_resizer.SetAnchor(IDCANCEL, ANCHOR_RIGHT | ANCHOR_BOTTOM);
	ASSERT(bOk);

	RECT rect;
	this->GetClientRect(&rect);
	CSize size;
	size.cx = rect.right - rect.left;
	size.cy = rect.bottom - rect.top;
	m_resizer.SetMinimumSize(_T("_root"), size);
	m_resizer.SetShowResizeGrip(TRUE);

	bOk = m_resizer.InvokeOnResized();

    UINT uTreeStyle = TVS_HASBUTTONS|TVS_LINESATROOT|TVS_FULLROWSELECT|TVS_SHOWSELALWAYS|TVS_CHECKBOXES;
    m_classImageListData.cbSize =  sizeof(SP_CLASSIMAGELIST_DATA);
    if ( SetupDiGetClassImageList(&m_classImageListData) == TRUE ) {
        m_pImageList = CImageList::FromHandle( m_classImageListData.ImageList );
        m_columnTree.GetTreeCtrl().SetImageList( m_pImageList, TVSIL_NORMAL );
    }
    CString name, version, date, provider, setup_info, setup_section, device_instance, version_string;
    name.LoadString(IDS_NAME);
    version.LoadString(IDS_VERSION);
    date.LoadString(IDS_DATE);
    provider.LoadString(IDS_PROVIDER);
    setup_info.LoadString(IDS_SETUP_INFO);
    setup_section.LoadString(IDS_SETUP_SECTION);
    device_instance.LoadString(IDS_DEVICE_INSTANCE);
    version_string.LoadStringW(IDS_VERSION_STRING);
    m_version.SetWindowText(boost::str(boost::wformat(version_string.GetString())%PRODUCT_MAJOR %PRODUCT_MINOR %PRODUCT_BUILD).c_str());
    m_columnTree.InsertColumn(0, name, LVCFMT_LEFT, 350);
	m_columnTree.InsertColumn(1, version, LVCFMT_LEFT, 130);
	m_columnTree.InsertColumn(2, date, LVCFMT_LEFT, 80);
	m_columnTree.InsertColumn(3, provider, LVCFMT_LEFT, 160);
	m_columnTree.InsertColumn(4, setup_info, LVCFMT_LEFT, 160);
    m_columnTree.InsertColumn(5, setup_section, LVCFMT_LEFT, 160);
    m_columnTree.InsertColumn(6, device_instance, LVCFMT_LEFT, 580);
    m_columnTree.GetTreeCtrl().ModifyStyle(0,uTreeStyle);
    switch( m_module.get_default_type() ){
    case PROFILE_TYPE_EXE:
        ((CButton*)GetDlgItem(IDC_RADIO_EXE))->SetCheck(1);  // Checked
        break;
    case PROFILE_TYPE_CAB:
        ((CButton*)GetDlgItem(IDC_RADIO_CAB))->SetCheck(1);  // Checked
        break;
    case PROFILE_TYPE_FOLDER:
        ((CButton*)GetDlgItem(IDC_RADIO_FOLDER))->SetCheck(1);  // Checked
        break;
    default:
        ((CButton*)GetDlgItem(IDC_RADIO_EXE))->SetCheck(1);  // Checked
    }
    m_module.set_enumerate_devices_callback( boost::bind( &CDrProfilerDlg::ShowDevice, this, _1, _2, _3, _4 ) );
    m_module.set_save_profile_callback( boost::bind( &CDrProfilerDlg::SaveProfile, this, _1, _2, _3, _4, _5 ) );
    m_btnSelectAll.EnableWindow( FALSE );
    m_btnUnselectAll.EnableWindow( FALSE );
    m_module.start_to_show_devices();
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void  CDrProfilerDlg::ShowDevice( PROFILE_STATE_ENUM state, hardware_class::ptr& c, hardware_device::ptr& device, hardware_driver::ptr& driver )
{  
    CString message;
    if ( state == PROFILE_STATE_ON_PROGRESS ){
        HTREEITEM hClass;
        if ( m_classes.count( c->name ) == 0 ){    
            SetupDiGetClassImageIndex( &m_classImageListData, &c->guid, &image );
            hClass = m_columnTree.GetTreeCtrl().InsertItem(c->description.c_str(), image, image );   
            m_classes[c->name] = hClass;
            m_columnTree.GetTreeCtrl().SetItemState(hClass, INDEXTOSTATEIMAGEMASK(0), TVIS_STATEIMAGEMASK);
        }
        else{
            hClass = m_classes[c->name];
        }
        m_devices_map[ device->device_instance_id ] = device;
        HTREEITEM  hDevice = m_columnTree.GetTreeCtrl().InsertItem( TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM|TVIF_STATE,
            device->device_description.c_str(),
            image,
            image,
            ( device->has_problem() ? ( device->is_disabled() ? INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1) : INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1) ) : 0 ),
            TVIS_OVERLAYMASK,
            (LPARAM)&m_devices_map[ device->device_instance_id ],
            hClass,
            TVI_LAST );
        if ( hDevice != NULL ) {
            message.LoadString(IDS_ENUM_PROGRESS);
            m_pMsgDialog->SetMessageText(boost::str(boost::wformat(message.GetString())%device->device_description).c_str());
            m_columnTree.SetItemText(hDevice,1,driver->driver_version.c_str());
            m_columnTree.SetItemText(hDevice,2,driver->driver_date.c_str() );                 
            m_columnTree.SetItemText(hDevice,3,device->manufacturer.c_str() );
            m_columnTree.SetItemText(hDevice,4,driver->original_inf_name.c_str() );
            m_columnTree.SetItemText(hDevice,5,driver->inf_section.c_str() );
            m_columnTree.SetItemText(hDevice,6,device->device_instance_id.c_str() );
            m_columnTree.GetTreeCtrl().SetItemState(hDevice, INDEXTOSTATEIMAGEMASK(1), TVIS_STATEIMAGEMASK);
            m_columnTree.GetTreeCtrl().SetCheck(hDevice);
            m_columnTree.GetTreeCtrl().Expand(hClass,TVE_EXPAND);
        }
    }
    else if ( state == PROFILE_STATE_START ){
        message.LoadString(IDS_ENUM_START);
        m_pMsgDialog->SetMessageText(message);
        m_btnSave.EnableWindow(FALSE);
        m_pMsgDialog->ShowWindow(SW_SHOW); 
        
    }
    else if ( state == PROFILE_STATE_FINISHED ){
        m_pMsgDialog->ShowWindow(SW_HIDE); 
        m_btnSave.EnableWindow();
        m_btnSelectAll.EnableWindow();
        m_btnUnselectAll.EnableWindow();
        if ( m_module.is_stopping() ) 
            CDialogEx::OnCancel();  
    }
}

void CDrProfilerDlg::SaveProfile( PROFILE_STATE_ENUM state, hardware_device::ptr& device, hardware_driver::ptr& driver, stdstring& file_name, stdstring &err )
{
    CString message;
    if ( err.length() )
       m_pMsgDialog->SetMessageText(err.c_str());
    else if ( state == PROFILE_STATE_ON_PROGRESS ){
        message.LoadString(IDS_SAVE_PROGRESS);
        m_pMsgDialog->SetMessageText(boost::str(boost::wformat(message.GetString())%device->device_description%file_name).c_str());
    }
    else if ( state == PROFILE_STATE_SYSTEM_INFO ){
        message.LoadString(IDS_SAVE_SYSTEM_INFO);
        m_pMsgDialog->SetMessageText(message);
    }
    else if ( state == PROFILE_STATE_COMPRESS ){
        message.LoadString(IDS_SAVE_COMPRESS);
        m_pMsgDialog->SetMessageText(message);
    }
    else if ( state == PROFILE_STATE_CREATE_SELFEXTRACT ){
        message.LoadString(IDS_SAVE_EXE);
        m_pMsgDialog->SetMessageText(message);
    }
    else if ( state == PROFILE_STATE_START ){
        message.LoadString(IDS_SAVE_START);
        m_pMsgDialog->SetMessageText(message);
        m_btnSave.EnableWindow(FALSE);
        m_btnSelectAll.EnableWindow(FALSE);
        m_btnUnselectAll.EnableWindow(FALSE);
        m_pMsgDialog->ShowWindow(SW_SHOW);      
        m_bRunning = true;
    }
    else if ( state == PROFILE_STATE_FINISHED ){
        m_pMsgDialog->ShowWindow(SW_HIDE); 
        m_btnSave.EnableWindow();
        m_btnSelectAll.EnableWindow();
        m_btnUnselectAll.EnableWindow();
        m_bRunning = false;
        if ( m_module.is_stopping() ) 
            CDialogEx::OnCancel();  
    }
}

void CDrProfilerDlg::CheckTreeViewItmes( BOOL fCheck )
{
    HTREEITEM root = m_columnTree.GetTreeCtrl().GetRootItem();
    HTREEITEM child = m_columnTree.GetTreeCtrl().GetChildItem(root);
    while( NULL != child ){
        m_columnTree.GetTreeCtrl().SetCheck(child, fCheck ) ;
        HTREEITEM next = m_columnTree.GetTreeCtrl().GetNextSiblingItem(child);
        if ( NULL != next ){
            child = next;
        }
        else{
            child = NULL;
            root = m_columnTree.GetTreeCtrl().GetNextSiblingItem(root);
            if ( NULL != root )
                child = m_columnTree.GetTreeCtrl().GetChildItem(root);
        }
    };
}

void CDrProfilerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDrProfilerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDrProfilerDlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    time_t now = time(NULL);
    TCHAR szNow[30]={0};
    stdstring computer_name = environment::get_computer_name();
    hardware_device::vtr selected_devices;
    HTREEITEM root = m_columnTree.GetTreeCtrl().GetRootItem();
    HTREEITEM child = m_columnTree.GetTreeCtrl().GetChildItem(root);
    while( NULL != child ){
        if ( m_columnTree.GetTreeCtrl().GetCheck(child) )
			selected_devices.push_back(*(hardware_device::ptr*)(m_columnTree.GetTreeCtrl().GetItemData(child)));
        HTREEITEM next = m_columnTree.GetTreeCtrl().GetNextSiblingItem(child);
        if ( NULL != next ){
            child = next;
        }
        else{
            child = NULL;
            root = m_columnTree.GetTreeCtrl().GetNextSiblingItem(root);
            if ( NULL != root )
                child = m_columnTree.GetTreeCtrl().GetChildItem(root);
        }
    };
    
    if ( selected_devices.size() == 0 ){
        CString message;
        message.LoadString(IDS_SELECT_WARNING);
        AfxMessageBox(message);
    }
    else if ( ((CButton*)GetDlgItem(IDC_RADIO_FOLDER))->GetCheck() ){
        CString message;
        message.LoadString(IDS_SAVE_FOLDER);
        _tcsftime(szNow, 30, _T("Date_%Y_%m_%d_Time_%H_%M_%S"), localtime(&now));
        CFolderDialog folderDlg( message, NULL, this, BIF_USENEWUI);
        if ( IDOK == folderDlg.DoModal() ){
            stdstring path = folderDlg.GetFolderPath();
            path.append(_T("\\")).append(szNow);
            m_module.start_to_save_profile( PROFILE_TYPE_FOLDER, path, selected_devices );
        }
    }
    else{
        _tcsftime(szNow, 30, _T("%Y_%m_%d-%H_%M_%S"), localtime(&now));
        if  ( ((CButton*)GetDlgItem(IDC_RADIO_CAB))->GetCheck() ){
            CString cab_file;
            cab_file.LoadString(IDS_SAVE_CAB);
            CFileDialog filedlg( FALSE, TEXT("*.cab"), boost::str(boost::wformat(L"%s_%s.cab") %computer_name %szNow).c_str(), 
                    OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
                    cab_file, NULL );
            filedlg.m_ofn.lpstrInitialDir = environment::get_working_directory().c_str();
            if( filedlg.DoModal ()==IDOK ) {
                m_module.start_to_save_profile( PROFILE_TYPE_CAB, filedlg.GetPathName().GetString(), selected_devices );
            }
        }
        else if  ( ((CButton*)GetDlgItem(IDC_RADIO_EXE))->GetCheck() ){
            CString exe_file;
            exe_file.LoadString(IDS_SAVE_SELF_EXTRACT);
            CFileDialog filedlg( FALSE, TEXT("*.exe"), boost::str(boost::wformat(L"%s_%s.exe") %computer_name %szNow).c_str(), 
                    OFN_HIDEREADONLY | OFN_NOREADONLYRETURN | OFN_EXPLORER | OFN_OVERWRITEPROMPT,
                    exe_file, NULL );
            filedlg.m_ofn.lpstrInitialDir = environment::get_working_directory().c_str();
            if( filedlg.DoModal ()==IDOK ) {
                m_module.start_to_save_profile( PROFILE_TYPE_EXE, filedlg.GetPathName().GetString(), selected_devices );
            }
        }
    }
}

void CDrProfilerDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    m_module.stop();
    if ( !m_module.is_stopped() ){
        CString cancel;
        cancel.LoadString(IDS_CANCEL_STRING);
        m_pMsgDialog->SetMessageText(cancel);
        m_pMsgDialog->ShowWindow(SW_SHOW); 
    }
    else
        CDialogEx::OnCancel();  
}


void CDrProfilerDlg::OnBnClickedBtnSelectAll()
{
    // TODO: Add your control notification handler code here
    CheckTreeViewItmes();
}


void CDrProfilerDlg::OnBnClickedBtnUnselectAll()
{
    // TODO: Add your control notification handler code here
    CheckTreeViewItmes(FALSE);
}


void CDrProfilerDlg::OnMove(int x, int y)
{
    CDialogEx::OnMove(x, y);
    if ( m_pMsgDialog ){
        CRect rectDlg, msgDlg;
        GetWindowRect(&rectDlg);
        m_pMsgDialog->GetWindowRect(&msgDlg);
        int nXPos = x + (rectDlg.Width() / 2) - ( msgDlg.Width() / 2 )-10;
        int nYPos = y + (rectDlg.Height() / 2) - ( msgDlg.Height() / 2 ) - 50;
        ::SetWindowPos(m_pMsgDialog->m_hWnd, HWND_TOPMOST, nXPos , nYPos, msgDlg.Width(), msgDlg.Height(), SWP_NOCOPYBITS);
    }
    // TODO: Add your message handler code here
}


#include "bcd_edit.h"
#include "reg_hive_edit.h"

#define WINDOWS_BOOT_MANAGER        _T("{9dea862c-5cdd-4e70-acc1-f32b344d4795}") // Windows Boot Manager {bootmgr}
#define FIRMWARE_BOOT_MANAGER       _T("{a5a30fa2-3d06-4e9f-b5f4-a01df9d1fcba}") // Firmware Boot Manager {fwbootmgr}
#define WINDOWS_MEMORY_TESTER       _T("{b2721d73-1db4-4c62-bf78-c548a880142d}") //Windows Memory Tester {memdiag}
#define LEGACY_WINDOWS_LOADER       _T("{466f5a88-0af2-4f76-9038-095b170dc21c}") //Legacy Windows Loader {ntldr}
#define CURRENT_BOOT_ENTRY          _T("{fa926493-6f1c-4193-a414-58f0b2456d1e}") // Current boot entry {current}
#define WINDOWS_RESUME_APPLICATION  _T("{147aa509-0358-4473-b83b-d950dda00615}") // Windows Resume Application
#define GLOBAL_DEBUG_SETTINGS       _T("{4636856e-540f-4170-a130-a84776f4c654}")

typedef enum BcdDeviceObjectElementTypes {
    BcdDeviceInteger_RamdiskImageOffset = 0x35000001,
    BcdDeviceInteger_TftpClientPort = 0x35000002,
    BcdDeviceInteger_SdiDevice = 0x31000003,
    BcdDeviceInteger_SdiPath = 0x32000004,
    BcdDeviceInteger_RamdiskImageLength = 0x35000005
} BcdDeviceObjectElementTypes;

typedef enum BcdLibrary_DebuggerType {
    DebuggerSerial = 0,
    Debugger1394 = 1,
    DebuggerUsb = 2
} BcdLibrary_DebuggerType;

typedef enum BcdLibrary_SafeBoot {
    SafemodeMinimal = 0,
    SafemodeNetwork = 1,
    SafemodeDsRepair = 2
} BcdLibrary_SafeBoot;

typedef enum BcdMemDiagElementTypes {
    BcdMemDiagInteger_PassCount = 0x25000001,
    BcdMemDiagInteger_FailureCount = 0x25000003
} BcdMemDiagElementTypes;

typedef enum BcdOSLoader_NxPolicy {
    NxPolicyOptIn = 0,
    NxPolicyOptOut = 1,
    NxPolicyAlwaysOff = 2,
    NxPolicyAlwaysOn = 3
} BcdOSLoader_NxPolicy;

typedef enum BcdOSLoader_PAEPolicy {
    PaePolicyDefault = 0,
    PaePolicyForceEnable = 1,
    PaePolicyForceDisable = 2
} BcdOSLoader_PAEPolicy;

typedef enum BcdDeviceType{
    BootDevice = 1,
    PartitionDevice = 2,
    FileDevice = 3,
    RamdiskDevice = 4,
    UnknownDevice = 5
}BcdDeviceType;

typedef enum BcdBootMgrElementTypes {
    BcdBootMgrObjectList_DisplayOrder = 0x24000001,
    BcdBootMgrObjectList_BootSequence = 0x24000002,
    BcdBootMgrObject_DefaultObject = 0x23000003,
    BcdBootMgrInteger_Timeout = 0x25000004,
    BcdBootMgrBoolean_AttemptResume = 0x26000005,
    BcdBootMgrObject_ResumeObject = 0x23000006,
    BcdBootMgrObjectList_ToolsDisplayOrder = 0x24000010,
    BcdBootMgrDevice_BcdDevice = 0x21000022,
    BcdBootMgrString_BcdFilePath = 0x22000023
}   BcdBootMgrElementTypes;

typedef enum BcdLibraryElementTypes {
    BcdLibraryDevice_ApplicationDevice = 0x11000001,
    BcdLibraryString_ApplicationPath = 0x12000002,
    BcdLibraryString_Description = 0x12000004,
    BcdLibraryString_PreferredLocale = 0x12000005,
    BcdLibraryObjectList_InheritedObjects = 0x14000006,
    BcdLibraryInteger_TruncatePhysicalMemory = 0x15000007,
    BcdLibraryObjectList_RecoverySequence = 0x14000008,
    BcdLibraryBoolean_AutoRecoveryEnabled = 0x16000009,
    BcdLibraryIntegerList_BadMemoryList = 0x1700000a,
    BcdLibraryBoolean_AllowBadMemoryAccess = 0x1600000b,
    BcdLibraryInteger_FirstMegabytePolicy = 0x1500000c,
    BcdLibraryBoolean_DebuggerEnabled = 0x16000010,
    BcdLibraryInteger_DebuggerType = 0x15000011,
    BcdLibraryInteger_SerialDebuggerPortAddress = 0x15000012,
    BcdLibraryInteger_SerialDebuggerPort = 0x15000013,
    BcdLibraryInteger_SerialDebuggerBaudRate = 0x15000014,
    BcdLibraryInteger_1394DebuggerChannel = 0x15000015,
    BcdLibraryString_UsbDebuggerTargetName = 0x12000016,
    BcdLibraryBoolean_DebuggerIgnoreUsermodeExceptions = 0x16000017,
    BcdLibraryInteger_DebuggerStartPolicy = 0x15000018,
    BcdLibraryBoolean_EmsEnabled = 0x16000020,
    BcdLibraryInteger_EmsPort = 0x15000022,
    BcdLibraryInteger_EmsBaudRate = 0x15000023,
    BcdLibraryString_LoadOptionsString = 0x12000030,
    BcdLibraryBoolean_DisplayAdvancedOptions = 0x16000040,
    BcdLibraryBoolean_DisplayOptionsEdit = 0x16000041,
    BcdLibraryBoolean_GraphicsModeDisabled = 0x16000046,
    BcdLibraryInteger_ConfigAccessPolicy = 0x15000047,
    BcdLibraryBoolean_AllowPrereleaseSignatures = 0x16000049
} BcdLibraryElementTypes;

typedef enum BcdOSLoaderElementTypes {
    BcdOSLoaderDevice_OSDevice = 0x21000001,
    BcdOSLoaderString_SystemRoot = 0x22000002,
    BcdOSLoaderObject_AssociatedResumeObject = 0x23000003,
    BcdOSLoaderBoolean_DetectKernelAndHal = 0x26000010,
    BcdOSLoaderString_KernelPath = 0x22000011,
    BcdOSLoaderString_HalPath = 0x22000012,
    BcdOSLoaderString_DbgTransportPath = 0x22000013,
    BcdOSLoaderInteger_NxPolicy = 0x25000020,
    BcdOSLoaderInteger_PAEPolicy = 0x25000021,
    BcdOSLoaderBoolean_WinPEMode = 0x26000022,
    BcdOSLoaderBoolean_DisableCrashAutoReboot = 0x26000024,
    BcdOSLoaderBoolean_UseLastGoodSettings = 0x26000025,
    BcdOSLoaderBoolean_AllowPrereleaseSignatures = 0x26000027,
    BcdOSLoaderBoolean_NoLowMemory = 0x26000030,
    BcdOSLoaderInteger_RemoveMemory = 0x25000031,
    BcdOSLoaderInteger_IncreaseUserVa = 0x25000032,
    BcdOSLoaderBoolean_UseVgaDriver = 0x26000040,
    BcdOSLoaderBoolean_DisableBootDisplay = 0x26000041,
    BcdOSLoaderBoolean_DisableVesaBios = 0x26000042,
    BcdOSLoaderInteger_ClusterModeAddressing = 0x25000050,
    BcdOSLoaderBoolean_UsePhysicalDestination = 0x26000051,
    BcdOSLoaderInteger_RestrictApicCluster = 0x25000052,
    BcdOSLoaderBoolean_UseBootProcessorOnly = 0x26000060,
    BcdOSLoaderInteger_NumberOfProcessors = 0x25000061,
    BcdOSLoaderBoolean_ForceMaximumProcessors = 0x26000062,
    BcdOSLoaderBoolean_ProcessorConfigurationFlags = 0x25000063,
    BcdOSLoaderInteger_UseFirmwarePciSettings = 0x26000070,
    BcdOSLoaderInteger_MsiPolicy = 0x26000071,
    BcdOSLoaderInteger_SafeBoot = 0x25000080,
    BcdOSLoaderBoolean_SafeBootAlternateShell = 0x26000081,
    BcdOSLoaderBoolean_BootLogInitialization = 0x26000090,
    BcdOSLoaderBoolean_VerboseObjectLoadMode = 0x26000091,
    BcdOSLoaderBoolean_KernelDebuggerEnabled = 0x260000a0,
    BcdOSLoaderBoolean_DebuggerHalBreakpoint = 0x260000a1,
    BcdOSLoaderBoolean_EmsEnabled = 0x260000b0,
    BcdOSLoaderInteger_DriverLoadFailurePolicy = 0x250000c1,
    BcdOSLoaderInteger_BootStatusPolicy = 0x250000E0
} BcdOSLoaderElementTypes;

using namespace macho::windows;
using namespace macho;

bcd_edit::bcd_edit(std::wstring bcd_file_path){

    HRESULT hr = S_OK;
    wmi_object inputOpenStore;
    wmi_object outputOpenStore;
    DWORD dwReturn;
    bool bRtn = false;
    FUN_TRACE_HRESULT(hr);
    macho::windows::environment::set_token_privilege(SE_RESTORE_NAME, true);
    macho::windows::environment::set_token_privilege(SE_BACKUP_NAME, true);
    hr = _wmi.connect(L"WMI", L".");
    if (!SUCCEEDED(hr)){
        BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't connect the WMI namespace");
    }
    else{
        hr = _wmi.get_input_parameters(L"BCDStore", L"OpenStore", inputOpenStore);
        if (!SUCCEEDED(hr)){
            BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't get the input parameter object for OpenStore method.");
        }
        else{
            hr = inputOpenStore.set_parameter(L"File", bcd_file_path);
            if (!SUCCEEDED(hr)){
                BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, boost::str(boost::wformat(L"Can't set the File path (%s).") % bcd_file_path));
            }
            else{
                hr = _wmi.exec_method(L"BCDStore", L"OpenStore", inputOpenStore, outputOpenStore, dwReturn);
                if (!SUCCEEDED(hr)){
                    BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, boost::str(boost::wformat(L"Can't open the Store Object (%s)") % bcd_file_path));
                }
                else{
                    outputOpenStore.get_parameter(L"ReturnValue", bRtn);
                    if (!bRtn){
                        BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, boost::str(boost::wformat(L"The return value of the OpenStore(%s) method is failed.") % bcd_file_path));
                    }
                    else{
                        hr = outputOpenStore.get_parameter(L"Store", _bcd_store);
                        if (!SUCCEEDED(hr))
                            BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, boost::str(boost::wformat(L"Can't get the Store Object") % bcd_file_path));
                    }
                }
            }
        }
    }
}

macho::windows::wmi_object bcd_edit::_get_bcd_boot_manager(){
    HRESULT hr = S_OK;
    wmi_object boot_manager;
    wmi_object inputOpenObject;
    wmi_object outputOpenObject;
    bool       bRtn = false;
    FUN_TRACE_HRESULT(hr);
    hr = _bcd_store.get_input_parameters(L"OpenObject", inputOpenObject);
    if (SUCCEEDED(hr)){
        inputOpenObject.set_parameter(L"Id", WINDOWS_BOOT_MANAGER);
        hr = _bcd_store.exec_method(L"OpenObject", inputOpenObject, outputOpenObject, bRtn);
        if (SUCCEEDED(hr) && bRtn)
            hr = outputOpenObject.get_parameter(L"Object", boot_manager);
        else{
            BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't OpenObject WINDOWS_BOOT_MANAGER object.");
        }
    }
    else{
        BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't get the OpenObject intput parameters object.");
    }
    return boot_manager;
}

std::wstring bcd_edit::_get_bcd_default_entry(macho::windows::wmi_object& bcd_boot_manager){
    HRESULT hr = S_OK;
    std::wstring wzDefaultBootEntry;
    FUN_TRACE_HRESULT(hr);
    wmi_object inputGetElement;
    wmi_object outputGetElement;
    bool       bRtn = false;
    hr = bcd_boot_manager.get_input_parameters(L"GetElement", inputGetElement);
    if (!SUCCEEDED(hr)){
        BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't get the GetElement intput parameters object.");
    }
    else{
        inputGetElement.set_parameter(L"Type", (LONG)BcdBootMgrObject_DefaultObject);        
        try{
            hr = bcd_boot_manager.exec_method(L"GetElement", inputGetElement, outputGetElement, bRtn);
            if (SUCCEEDED(hr) && bRtn){
                hr = bcd_boot_manager.exec_method(L"GetElement", inputGetElement, outputGetElement, bRtn);
                if (SUCCEEDED(hr) && bRtn){
                    wmi_object bcdDefaultBootElem;
                    hr = outputGetElement.get_parameter(L"Element", bcdDefaultBootElem);
                    std::wstring wzDefaultBootEntry;
                    hr = bcdDefaultBootElem.get_parameter(L"Id", wzDefaultBootEntry);
                }
            }
        }
        catch (...){
        }
        if (0 == wzDefaultBootEntry.length()){
            try{
                inputGetElement.set_parameter(L"Type", (LONG)BcdBootMgrObjectList_DisplayOrder);
                hr = bcd_boot_manager.exec_method(L"GetElement", inputGetElement, outputGetElement, bRtn);
                if (SUCCEEDED(hr) && bRtn){
                    wmi_object bcdObjectListElement;
                    hr = outputGetElement.get_parameter(L"Element", bcdObjectListElement);
                    string_array_w ids;
                    hr = bcdObjectListElement.get_parameter(L"Ids", ids);
                    if (SUCCEEDED(hr) && ids.size())
                        wzDefaultBootEntry = ids[0];
                }
            }
            catch (...){
            }
        }
    }
    return wzDefaultBootEntry;
}

macho::windows::wmi_object bcd_edit::_get_bcd_default_boot_entry(macho::windows::wmi_object& bcd_boot_manager){
    HRESULT hr = S_OK;
    wmi_object default_boot_entry;
    wmi_object inputOpenObject;
    wmi_object outputOpenObject;
    wmi_object inputGetElement;
    wmi_object outputGetElement;
    bool       bRtn = false;
    FUN_TRACE_HRESULT(hr);
    std::wstring wzDefaultBootEntry = _get_bcd_default_entry(bcd_boot_manager);
    if (!wzDefaultBootEntry.length()){
        BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't get Default Boot Entry ID.");
    }
    else{
        inputOpenObject = _bcd_store.get_input_parameters(L"OpenObject");
        inputOpenObject.set_parameter(L"Id", wzDefaultBootEntry.c_str());
        hr = _bcd_store.exec_method(L"OpenObject", inputOpenObject, outputOpenObject, bRtn);
        if (SUCCEEDED(hr) && bRtn){
            hr = outputOpenObject.get_parameter(L"Object", default_boot_entry);
            if (!SUCCEEDED(hr)){
                BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't get the Default Boot Entry Object.");
            }
        }
        else{
            BOOST_THROW_EXCEPTION_BASE(bcd_edit::exception, hr, L"Can't Open the Default Boot Entry Object.");
        }
    }
    return default_boot_entry;
}

bool bcd_edit::set_safe_mode(__in bool is_enable, __in bool is_domain_controller){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    wmi_object boot_manager = _get_bcd_boot_manager();
    wmi_object default_boot_entry = _get_bcd_default_boot_entry(boot_manager);
    wmi_object inputOpenObject;
    wmi_object outputOpenObject;
    wmi_object inputIntegerElement;
    wmi_object outputIntegerElement;
    bool       bRtn = false;
    
    std::wstring     wszMethod = is_enable ? L"SetIntegerElement" : L"DeleteElement";
    hr = default_boot_entry.get_input_parameters(wszMethod.c_str(), inputIntegerElement);
    if (SUCCEEDED(hr)){
        inputIntegerElement.set_parameter(L"Type", (LONG)BcdOSLoaderInteger_SafeBoot);
        if (is_enable)
            inputIntegerElement.set_parameter(L"Integer", is_domain_controller ? (LONG)SafemodeDsRepair : (LONG)SafemodeNetwork);
        hr = default_boot_entry.exec_method(wszMethod.c_str(), inputIntegerElement, bRtn);
    }

    if (!bRtn)
        hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    return hr == S_OK;
}

bool bcd_edit::set_boot_status_policy_option(__in bool is_enable, BootStatusPolicy policy ){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    wmi_object boot_manager = _get_bcd_boot_manager();
    wmi_object default_boot_entry = _get_bcd_default_boot_entry(boot_manager);
    wmi_object inputOpenObject;
    wmi_object outputOpenObject;
    wmi_object inputIntegerElement;
    wmi_object outputIntegerElement;
    bool       bRtn = false;

    std::wstring     wszMethod = is_enable ? L"SetIntegerElement" : L"DeleteElement";
    hr = default_boot_entry.get_input_parameters(wszMethod.c_str(), inputIntegerElement);
    if (SUCCEEDED(hr)){
        inputIntegerElement.set_parameter(L"Type", (LONG)BcdOSLoaderInteger_BootStatusPolicy);
        if (is_enable)
            inputIntegerElement.set_parameter(L"Integer", (LONG)policy);
        hr = default_boot_entry.exec_method(wszMethod.c_str(), inputIntegerElement, bRtn);
    }

    if (!bRtn)
        hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    return hr == S_OK;
}

bool bcd_edit::set_debug_settings(__in bool is_debug, __in bool is_boot_debug ){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    wmi_object boot_manager = _get_bcd_boot_manager();
    wmi_object default_boot_entry = _get_bcd_default_boot_entry(boot_manager);
    wmi_object inputOpenObject;
    wmi_object outputOpenObject;
    wmi_object inputIntegerElement;
    wmi_object outputIntegerElement;
    bool       bRtn = false;
    if (is_boot_debug || is_debug){
        inputOpenObject.set_parameter(L"Id", GLOBAL_DEBUG_SETTINGS);
        hr = _bcd_store.exec_method(L"OpenObject", inputOpenObject, outputOpenObject, bRtn);
        if (SUCCEEDED(hr) && bRtn){
            wmi_object bcdGlobalDebugSettings;
            hr = outputOpenObject.get_parameter(L"Object", bcdGlobalDebugSettings);
            if (SUCCEEDED(hr)){
                hr = bcdGlobalDebugSettings.get_input_parameters(L"SetIntegerElement", inputIntegerElement);
                inputIntegerElement.set_parameter(L"Type", (LONG)BcdLibraryInteger_DebuggerType);
                inputIntegerElement.set_parameter(L"Integer", (LONG)DebuggerSerial);
                hr = bcdGlobalDebugSettings.exec_method(L"SetIntegerElement", inputIntegerElement, bRtn);
                if (SUCCEEDED(hr) && bRtn){
                    inputIntegerElement.set_parameter(L"Type", (LONG)BcdLibraryInteger_SerialDebuggerPort);
                    inputIntegerElement.set_parameter(L"Integer", (LONG)1);
                    hr = bcdGlobalDebugSettings.exec_method(L"SetIntegerElement", inputIntegerElement, bRtn);
                    if (SUCCEEDED(hr) && bRtn){
                        inputIntegerElement.set_parameter(L"Type", (LONG)BcdLibraryInteger_SerialDebuggerBaudRate);
                        inputIntegerElement.set_parameter(L"Integer", (LONG)115200);
                        hr = bcdGlobalDebugSettings.exec_method(L"SetIntegerElement", inputIntegerElement, bRtn);
                    }
                }
            }
        }
    }
    if (SUCCEEDED(hr) && bRtn ){
        wmi_object input_mgr, input_boot;
        boot_manager.get_input_parameters(L"SetBooleanElement", input_mgr);
        input_mgr.set_parameter(L"Type", (LONG)BcdLibraryBoolean_DebuggerEnabled);
        input_mgr.set_parameter(L"Boolean", is_boot_debug);
        hr = boot_manager.exec_method(L"SetBooleanElement", input_mgr, bRtn);
        if (SUCCEEDED(hr) && bRtn){
            default_boot_entry.get_input_parameters(L"SetBooleanElement", input_boot);
            input_boot.set_parameter(L"Type", (LONG)BcdLibraryBoolean_DebuggerEnabled);
            input_boot.set_parameter(L"Boolean", is_boot_debug);
            hr = default_boot_entry.exec_method(L"SetBooleanElement", input_boot, bRtn);
        }
    }
    if (SUCCEEDED(hr) && bRtn){
        wmi_object input_boot;
        default_boot_entry.get_input_parameters(L"SetBooleanElement", input_boot);
        input_boot.set_parameter(L"Type", (LONG)BcdOSLoaderBoolean_KernelDebuggerEnabled);
        input_boot.set_parameter(L"Boolean", is_debug);
        hr = default_boot_entry.exec_method(L"SetBooleanElement", input_boot, bRtn);
    }
    if (!bRtn)
        hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    return hr == S_OK;
}

bool bcd_edit::set_detect_hal_option(__in bool is_enable){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    wmi_object boot_manager = _get_bcd_boot_manager();
    wmi_object default_boot_entry = _get_bcd_default_boot_entry(boot_manager);
    wmi_object inputIntegerElement;
    bool       bRtn = false;
    
    std::wstring     wszMethod = L"SetBooleanElement";
    if (SUCCEEDED(hr)){
        hr = default_boot_entry.get_input_parameters(wszMethod.c_str(), inputIntegerElement);
        if (SUCCEEDED(hr)){
            inputIntegerElement.set_parameter(L"Type", (LONG)BcdOSLoaderBoolean_DetectKernelAndHal);
            inputIntegerElement.set_parameter(L"Boolean", is_enable);
            hr = default_boot_entry.exec_method(wszMethod.c_str(), inputIntegerElement, bRtn);
        }
    }
    if (!bRtn)
        hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    return hr == S_OK;
}

bool bcd_edit::set_boot_system_device(__in std::wstring system_volume_path, __in bool is_virtual_disk){
    HRESULT hr = S_OK;
    FUN_TRACE_HRESULT(hr);
    wmi_object boot_manager = _get_bcd_boot_manager();
    wmi_object default_boot_entry = _get_bcd_default_boot_entry(boot_manager);
    wmi_object inputDeleteElement;
    wmi_object inputSetDeviceElement;
    wmi_object outputSetDeviceElement;
    wmi_object inputGetElement;
    wmi_object outputGetElement;
    wmi_object inputOpenObject;
    wmi_object outputOpenObject;
    bool       bRtn = false;
    std::wstring szSystemDevice;
    TCHAR      szDeviceName[MAX_PATH];
    memset(szDeviceName, 0, sizeof(szDeviceName));

    if (!system_volume_path.length())
        return false;
    if (system_volume_path[0] == TEXT('\\') ||
        system_volume_path[1] == TEXT('\\') ||
        system_volume_path[2] == TEXT('?') ||
        system_volume_path[3] == TEXT('\\') ){
        DWORD  CharCount = 0;
        if (system_volume_path[system_volume_path.length() - 1] == TEXT('\\'))
            system_volume_path.erase(system_volume_path.length() - 1);
        if ( CharCount = QueryDosDevice(&system_volume_path[4], szDeviceName, ARRAYSIZE(szDeviceName)) )
            szSystemDevice = szDeviceName;
    }
    else{
        TCHAR szDriveLetter[3];
        /*
        pszDriveLetter could be in the format X: or X:\; DefineDosDevice
        needs X:
        */
        szDriveLetter[0] = system_volume_path[0];
        szDriveLetter[1] = _T(':');
        szDriveLetter[2] = _T('\0');
        if (QueryDosDevice(szDriveLetter, szDeviceName, MAX_PATH))
            szSystemDevice = szDeviceName;
    }
 
    hr = boot_manager.get_input_parameters(L"DeleteElement", inputDeleteElement);
    if (SUCCEEDED(hr)){
        inputDeleteElement.set_parameter(L"Type", (LONG)BcdLibraryString_ApplicationPath);
        hr = boot_manager.exec_method(L"DeleteElement", inputDeleteElement, bRtn);
        if (SUCCEEDED(hr)){
            hr = boot_manager.get_input_parameters(L"SetDeviceElement", inputSetDeviceElement);

            if (SUCCEEDED(hr)){
                inputSetDeviceElement.set_parameter(L"Type", (LONG)BcdLibraryDevice_ApplicationDevice); // BcdOSLoaderDevice_OSDevice
                inputSetDeviceElement.set_parameter(L"DeviceType", (LONG)BootDevice);
                inputSetDeviceElement.set_parameter(L"AdditionalOptions", _T(""));
                hr = boot_manager.exec_method(L"SetDeviceElement", inputSetDeviceElement, outputSetDeviceElement, bRtn);

                if (SUCCEEDED(hr) && bRtn){

                    wmi_object inputSetPartitionDeviceElement;
                    wmi_object outputSetPartitionDeviceElement;
                    std::wstring     wszMethod = L"SetPartitionDeviceElement";
                                   
                    hr = default_boot_entry.get_input_parameters(wszMethod.c_str(), inputSetPartitionDeviceElement);

                    if (SUCCEEDED(hr)){
                        if (!is_virtual_disk){
                            inputSetPartitionDeviceElement.set_parameter(L"Type", (LONG)BcdLibraryDevice_ApplicationDevice); // BcdOSLoaderDevice_OSDevice
                            inputSetPartitionDeviceElement.set_parameter(L"DeviceType", (LONG)PartitionDevice);
                            inputSetPartitionDeviceElement.set_parameter(L"AdditionalOptions", _T(""));
                            inputSetPartitionDeviceElement.set_parameter(L"Path", szSystemDevice.c_str());
                            hr = default_boot_entry.exec_method(wszMethod.c_str(), inputSetPartitionDeviceElement, outputSetPartitionDeviceElement, bRtn);
                        }
                        if (SUCCEEDED(hr) && bRtn){
                            if (!is_virtual_disk){
                                inputSetPartitionDeviceElement.set_parameter(L"Type", (LONG)BcdOSLoaderDevice_OSDevice);
                                hr = default_boot_entry.exec_method(wszMethod.c_str(), inputSetPartitionDeviceElement, outputSetPartitionDeviceElement, bRtn);
                            }
                            if (SUCCEEDED(hr) && bRtn){
                                wmi_object inputGetPathElement;
                                wmi_object inputSetPathElement;
                                wmi_object outputGetPathElement;
                                wmi_object outputSetPathElement;
                                hr = default_boot_entry.get_input_parameters(L"GetElement", inputGetPathElement);
                                inputGetPathElement.set_parameter(L"Type", (LONG)BcdLibraryString_ApplicationPath);
                                hr = default_boot_entry.exec_method(L"GetElement", inputGetPathElement, outputGetPathElement, bRtn);
                                if (SUCCEEDED(hr) && bRtn){
                                    std::wstring szPath;
                                    wmi_object bcdPathElement;
                                    hr = outputGetPathElement.get_parameter(L"Element", bcdPathElement);
                                    bcdPathElement.get_parameter(L"String", szPath);
                                    if (szPath.length() > 3 &&
                                        (szPath[szPath.length() - 1] == _T('i')) &&
                                        (szPath[szPath.length() - 2] == _T('f')) &&
                                        (szPath[szPath.length() - 3] == _T('e')))
                                    {
                                        szPath[szPath.length() - 1] = _T('e');
                                        szPath[szPath.length() - 2] = _T('x');
                                        hr = default_boot_entry.get_input_parameters(L"SetStringElement", inputSetPathElement);
                                        inputSetPathElement.set_parameter(L"Type", (LONG)BcdLibraryString_ApplicationPath);
                                        inputSetPathElement.set_parameter(L"String", szPath.c_str());
                                        hr = default_boot_entry.exec_method(L"SetStringElement", inputSetPathElement, outputSetPathElement, bRtn);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (!bRtn)
        hr = HRESULT_FROM_WIN32(ERROR_CANT_ACCESS_FILE);
    return hr == S_OK;
}

bcd_edit_cli::bcd_edit_cli(std::wstring bcd_file_path) : _bcd_file(bcd_file_path){

}

bool bcd_edit_cli::set_safe_mode(__in bool is_enable, __in bool is_domain_controller){
    std::wstring command = _bcd_file.empty() ? L"bcdedit " : boost::str(boost::wformat(L"bcdedit /store %s ") % _bcd_file);
    std::wstring ret;
    bool result = false;
    if (is_enable){
        command.append(boost::str(boost::wformat(L"/set {default} safeboot %s") % (is_domain_controller? L"dsrepair": L"network")));
    }
    else{
        command.append(L"/deletevalue {default} safeboot");
    }
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    return result;
}

bool bcd_edit_cli::set_boot_status_policy_option(__in bool is_enable, BootStatusPolicy policy){
    std::wstring command = _bcd_file.empty() ? L"bcdedit " : boost::str(boost::wformat(L"bcdedit /store %s ") % _bcd_file);
    std::wstring ret;
    bool result = false;
    if (is_enable){
        switch (policy){
        case BootStatusPolicy::BootStatusPolicyDisplayAllFailures:
            command.append(L"/set {default} bootstatuspolicy displayallfailures");
            break;
        case BootStatusPolicy::BootStatusPolicyIgnoreAllFailures:
            command.append(L"/set {default} bootstatuspolicy ignoreallfailures");
            break;
        case BootStatusPolicy::BootStatusPolicyIgnoreBootFailures:
            command.append(L"/set {default} bootstatuspolicy ignorebootfailures");
            break;
        case BootStatusPolicy::BootStatusPolicyIgnoreShutdownFailures:
            command.append(L"/set {default} bootstatuspolicy ignoreshutdownfailures");
            break;
        }
    }
    else{
        command.append(L"/deletevalue {default} bootstatuspolicy");
    }
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    return result;
}

bool bcd_edit_cli::set_detect_hal_option(__in bool is_enable){
    std::wstring command = _bcd_file.empty() ? L"bcdedit " : boost::str(boost::wformat(L"bcdedit /store %s ") % _bcd_file);
    std::wstring ret;
    bool result = false;
    if (is_enable){
        command.append(L"/set {default} detecthal yes");
    }
    else{
        command.append(L"/set {default} detecthal no");
    }
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    return result;
}

bool bcd_edit_cli::set_boot_system_device(__in std::wstring system_volume_path, __in bool is_virtual_disk){
    std::wstring szSystemDevice;
    TCHAR      szDeviceName[MAX_PATH];
    memset(szDeviceName, 0, sizeof(szDeviceName));
    if (!system_volume_path.length())
        return false;
    if (system_volume_path[0] == TEXT('\\') ||
        system_volume_path[1] == TEXT('\\') ||
        system_volume_path[2] == TEXT('?') ||
        system_volume_path[3] == TEXT('\\')){
        DWORD  CharCount = 0;
        if (system_volume_path[system_volume_path.length() - 1] == TEXT('\\'))
            system_volume_path.erase(system_volume_path.length() - 1);
        if (CharCount = QueryDosDevice(&system_volume_path[4], szDeviceName, ARRAYSIZE(szDeviceName)))
            szSystemDevice = szDeviceName;
    }
    else{
        TCHAR szDriveLetter[3];
        /*
        pszDriveLetter could be in the format X: or X:\; DefineDosDevice
        needs X:
        */
        szDriveLetter[0] = system_volume_path[0];
        szDriveLetter[1] = _T(':');
        szDriveLetter[2] = _T('\0');
        if (QueryDosDevice(szDriveLetter, szDeviceName, MAX_PATH))
            szSystemDevice = szDeviceName;
    }
    std::wstring bcd_store = _bcd_file.empty() ? L"" : boost::str(boost::wformat(L"/store %s") % _bcd_file);
    std::wstring command, ret;
    bool result = false;
    command = boost::str(boost::wformat(L"bcdedit %s /deletevalue {bootmgr} path") % bcd_store);
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    if (!result)
        return false;
        
    command = boost::str(boost::wformat(L"bcdedit %s /set {bootmgr} device boot") % bcd_store);
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    if (!result)
        return false;
    command = boost::str(boost::wformat(L"bcdedit %s /set {default} device %s=%s") % bcd_store % (is_virtual_disk ? L"hd_partition" : L"partition") % szSystemDevice);
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    if (!result)
        return false;
    if (!is_virtual_disk){
        command = boost::str(boost::wformat(L"bcdedit %s /set {default} osdevice partition=%s") % bcd_store % szSystemDevice);
        LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
        result = process::exec_console_application_with_timeout(command, ret, -1);
        LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
        if (!result)
            return false;
    }
#if 0
    command = boost::str(boost::wformat(L"bcdedit %s /enum {default}") % bcd_store);
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    if (!result)
        return false;
    string_array_w arr = stringutils::tokenize2(ret, L"\r\n");
    std::map<std::wstring, std::wstring, stringutils::no_case_string_less_w> entry_values;
    foreach(std::wstring &v, arr){
        string_array_w _arr = stringutils::tokenize2(v, L" ", 2, false);
        if (_arr.size() > 1){
            entry_values[_arr[0]] = _arr[1];
        }
    }
    if (entry_values.count(L"path")){
        std::wstring szPath = entry_values[L"path"];
        if (szPath.length() > 3 &&
            (szPath[szPath.length() - 1] == _T('i')) &&
            (szPath[szPath.length() - 2] == _T('f')) &&
            (szPath[szPath.length() - 3] == _T('e')))
        {
            szPath[szPath.length() - 1] = _T('e');
            szPath[szPath.length() - 2] = _T('x');
            command = boost::str(boost::wformat(L"bcdedit %s /set {default} path %s") % bcd_store %szPath);
            LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
            result = process::exec_console_application_with_timeout(command, ret, -1);
            LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    }
    }
#else
    std::wstring szPath = _get_default_entry_path();
    if (szPath.length() > 3 &&
        (szPath[szPath.length() - 1] == _T('i')) &&
        (szPath[szPath.length() - 2] == _T('f')) &&
        (szPath[szPath.length() - 3] == _T('e')))
    {
        szPath[szPath.length() - 1] = _T('e');
        szPath[szPath.length() - 2] = _T('x');
        command = boost::str(boost::wformat(L"bcdedit %s /set {default} path %s") % bcd_store %szPath);
        LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
        result = process::exec_console_application_with_timeout(command, ret, -1);
        LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    }
#endif
    return result;
}

bool bcd_edit_cli::set_debug_settings(__in bool is_debug, __in bool is_boot_debug){
    std::wstring bcd_cmd = _bcd_file.empty() ? L"bcdedit " : boost::str(boost::wformat(L"bcdedit /store %s ") % _bcd_file);
    std::wstring ret;
    bool result = false;
    std::wstring command;
    if (is_debug){
        command = bcd_cmd + (L"/debug {default} on");
    }
    else{
        command = bcd_cmd + (L"/debug {default} off");
    }
    LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
    result = process::exec_console_application_with_timeout(command, ret, -1);
    LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    if (result){
        if (is_boot_debug){
            command = bcd_cmd + (L"/bootdebug {bootmgr} on");
        }
        else{
            command = bcd_cmd + (L"/bootdebug {bootmgr} off");
        }
        LOG(LOG_LEVEL_RECORD, L"Command: %s", command.c_str());
        result = process::exec_console_application_with_timeout(command, ret, -1);
        LOG(LOG_LEVEL_RECORD, L"Result: %s\n%s", result ? L"true" : L"false", ret.c_str());
    }
    return result;
}

std::wstring bcd_edit_cli::_get_default_entry_path(){
    std::wstring path;
    std::wstring default_id;
    if (!_bcd_file.empty()){
        reg_hive_edit::ptr boot_hive_edit_ptr = reg_hive_edit::ptr(new reg_hive_edit(boost::filesystem::path(_bcd_file).parent_path()));
        macho::windows::registry reg(*boot_hive_edit_ptr.get(), macho::windows::REGISTRY_FLAGS_ENUM::REGISTRY_READONLY);
        if (reg.open(L"BCD\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\23000003")){
            if (reg[_T("Element")].exists() && reg[_T("Element")].is_string())
                default_id = reg[_T("Element")].wstring();
            reg.close();
        }
        else if (reg.open(L"BCD\\Objects\\{9dea862c-5cdd-4e70-acc1-f32b344d4795}\\Elements\\24000001")){
            if (reg[_T("Element")].exists() && reg[_T("Element")].is_multi_sz() && reg[_T("Element")].get_multi_count())
                default_id = reg[_T("Element")].get_multi_at(0);
            reg.close();
        }
        if (default_id.length()){
            if (reg.open(boost::str(boost::wformat(L"BCD\\Objects\\%s\\Elements") % default_id)) &&
                reg.subkey(_T("12000002"))[_T("Element")].exists()){
                path = reg.subkey(_T("12000002"))[_T("Element")].wstring();
                reg.close();
            }
        }
    }
    return path;
}
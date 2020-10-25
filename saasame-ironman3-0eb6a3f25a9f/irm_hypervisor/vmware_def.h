// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

namespace mwdc { namespace ironman { namespace hypervisor {

enum hv_server_type
{
    HV_SERVER_TYPE_UNKNOWN = 0,
    HV_SERVER_TYPE_VMWARE = 1,
    HV_SERVER_TYPE_HYPERV = 2,
    HV_SERVER_TYPE_HYPERV_V2 = 3
};

enum hv_connection_type
{
    HV_CONNECTION_TYPE_UNKNOWN = 0,
    HV_CONNECTION_TYPE_VCENTER = 1,
    HV_CONNECTION_TYPE_HOST = 2
};

enum hv_power_action
{
    HV_POWER_ACTION_ON = 0,
    HV_POWER_ACTION_OFF = 1,
    HV_POWER_ACTION_SUSPEND = 2,
    HV_POWER_ACTION_RESET = 3,
    HV_POWER_ACTION_SHUTDOWN_GUEST = 4,
    HV_POWER_ACTION_REBOOT_GUEST = 5
};

enum hv_host_power_action
{
    HV_HOST_POWER_ACTION_ON_FROM_STANDBY = 0,
    HV_HOST_POWER_ACTION_STANDBY = 1,
    HV_HOST_POWER_ACTION_REBOOT = 2,
    HV_HOST_POWER_ACTION_SHUTDOWN = 3
};

enum hv_controller_type
{
    HV_CTRL_ANY = -1,
    HV_CTRL_IDE = 0,
    HV_CTRL_PARA_VIRT_SCSI = 1,
    HV_CTRL_BUS_LOGIC = 2,
    HV_CTRL_SCSI = 3, // alias for HV_CTRL_LSI_LOGIC
    HV_CTRL_LSI_LOGIC = 3,
    HV_CTRL_LSI_LOGIC_SAS = 4
};

// Bus Sharing
enum hv_bus_sharing
{
    HV_BUS_SHARING_ANY = -1,
    HV_BUS_SHARING_NONE = 0,
    HV_BUS_SHARING_VIRTUAL = 1,
    HV_BUS_SHARING_PHYSICAL = 2
};

//VMTools Options
enum hv_vm_tools_option
{
    HV_VMTOOLS_MOUNT = 0,
    HV_VMTOOLS_UNMOUNT = 1
};

enum hv_vm_tools_status
{
    HV_VMTOOLS_UNKNOWN = 0x00000000L,
    HV_VMTOOLS_OK = 0x00000001L,
    HV_VMTOOLS_NOTINSTALLED = 0x00000002L,
    HV_VMTOOLS_OLD = 0x00000003L,
    HV_VMTOOLS_NOTRUNNING = 0x00000004L,
    HV_VMTOOLS_NEEDUPGRADE = 0x00000005L,
    HV_VMTOOLS_UNMANAGED = 0x00000006L,
    HV_VMTOOLS_NEW = 0x00000007L,
    HV_VMTOOLS_BLACKLISTED = 0x00000008L
};

enum hv_vm_power_state
{
    HV_VMPOWER_UNKNOWN = 0x00000000L,
    HV_VMPOWER_ON = 0x00000001L,
    HV_VMPOWER_OFF = 0x00000002L,
    HV_VMPOWER_SUSPENDED = 0x00000003L,
};

enum hv_vm_connection_state
{
    HV_VMCONNECT_UNKNOWN = 0x00000000L,
    HV_VMCONNECT_CONNECTED = 0x00000001L,
    HV_VMCONNECT_DISCONNECTED = 0x00000002L,
    HV_VMCONNECT_INACCESSIBLE = 0x00000003L,
    HV_VMCONNECT_INVALID = 0x00000004L,
    HV_VMCONNECT_ORPHANED = 0x00000005L
};

enum hv_host_power_state
{
    HV_HOSTPOWER_UNKNOWN = 0x00000000L,
    HV_HOSTPOWER_ON = 0x00000001L,
    HV_HOSTPOWER_OFF = 0x00000002L,
    HV_HOSTPOWER_STANDBY = 0x00000003L,
};

// Guest OS
enum hv_guest_os_type
{
    HV_OS_UNKNOWN = 0x00000000L,
    HV_OS_WINDOWS = 0x00000001L,
    HV_OS_LINUX = 0x00000002L
};

struct hv_guest_os
{
    enum type{
        HV_OS_UNKNOWN,
        HV_OS_WINDOWS_VISTA,
        HV_OS_WINDOWS_7,
        HV_OS_WINDOWS_8,
        HV_OS_WINDOWS_8_1,  
        HV_OS_WINDOWS_10,    
        HV_OS_WINDOWS_2003_WEB,
        HV_OS_WINDOWS_2003_STANDARD,
        HV_OS_WINDOWS_2003_ENTERPRISE,
        HV_OS_WINDOWS_2003_DATACENTER,
        HV_OS_WINDOWS_2003_BUSINESS,
        HV_OS_WINDOWS_2008,
        HV_OS_WINDOWS_2008R2,
        HV_OS_WINDOWS_2012,
        HV_OS_WINDOWS_2012R2,
        HV_OS_WINDOWS_2016,
        HV_OS_REDHAT_4,
        HV_OS_REDHAT_5,
        HV_OS_REDHAT_6,
        HV_OS_REDHAT_7,
        HV_OS_SUSE_10,
        HV_OS_SUSE_11,
        HV_OS_SUSE_12,
        HV_OS_CENTOS,
        HV_OS_UBUNTU,
        HV_OS_OPENSUSE,
        HV_OS_DEBIAN_6,
        HV_OS_DEBIAN_7,
        HV_OS_DEBIAN_8,
        FREE_BSD,
        ORACLE_LINUX
    };
    enum architecture{
        X86,
        X64
    };
};

// Boot Firmware
enum hv_vm_firmware
{
    HV_VM_FIRMWARE_BIOS = 0x00000000L,
    HV_VM_FIRMWARE_EFI = 0x00000001L
};

enum hv_drs_vm
{
    HV_DRSVM_DEFAULT = 0,
    HV_DRSVM_DISABLED = 1,
    HV_DRSVM_FULLY_AUTOMATED = 2,
    HV_DRSVM_PARTIALLY_AUTOMATED = 3,
    HV_DRSVM_MANUAL = 4
};

enum hv_lun_type
{
    HV_LUNTYPE_UNKNOWN = 0,
    HV_LUNTYPE_ISCSI = 1,
    HV_LUNTYPE_FC = 2,
    HV_LUNTYPE_PARALLEL = 3,
    HV_LUNTYPE_BLOCK = 4
};

enum hv_datastore_type
{
    HV_DSTYPE_UNKNOWN = 0,
    HV_DSTYPE_VMFS = 1,
    HV_DSTYPE_NFS = 2
};


// HRESULT Macros
//
#define FACILITY_HV 0x800

#define HV_HRESULT(code) MAKE_HRESULT( SEVERITY_ERROR, FACILITY_HV, code )
#define HV_HRESULT_WARNING(code) MAKE_HRESULT( SEVERITY_SUCCESS, FACILITY_HV, code )

// No error
//
#define HV_ERROR_SUCCESS                                 S_OK

// Logic errors, implies a bug
//
#define HV_ERROR_INVALID_ARGUMENT                        HV_HRESULT(0x1)
#define HV_ERROR_NOT_IMPLEMENTED                         HV_HRESULT(0x2)
#define HV_ERROR_INVALID_HYPERVISOR_INSTANCE             HV_HRESULT(0x3)

// Internal errors, should not be exposed
//
#define HV_ERROR_FAILED_TO_GET_PROPERTY                  HV_HRESULT(0x501)
#define HV_ERROR_FAILED_TO_SET_PROPERTY                  HV_HRESULT(0x502)
#define HV_ERROR_OPERATION_FAILED                        HV_HRESULT(0x503)

// Normal errors
//
#define HV_ERROR_NOT_SUPPORTED                           HV_HRESULT(0x1001)
#define HV_ERROR_LOGIN_FAILED                            HV_HRESULT(0x1002)
#define HV_ERROR_LOGOUT_FAILED                           HV_HRESULT(0x1003)
#define HV_ERROR_VIRTUALCENTER_GET_FAILED                HV_HRESULT(0x1004)
#define HV_ERROR_HOST_NOT_FOUND                          HV_HRESULT(0x1005)
#define HV_ERROR_HOST_GET_FAILED                         HV_HRESULT(0x1006)
#define HV_ERROR_RESCAN_STORAGEADAPTOR_FAILED            HV_HRESULT(0x1007)
#define HV_ERROR_RESIGNATURE_VOLUMES_FAILED              HV_HRESULT(0x1008)
#define HV_ERROR_LUN_GET_FAILED                          HV_HRESULT(0x1009)
#define HV_ERROR_LUN_NOT_FOUND                           HV_HRESULT(0x1010)
#define HV_ERROR_DATACENTER_GET_FAILED                   HV_HRESULT(0x1011)
#define HV_ERROR_VM_NOT_FOUND                            HV_HRESULT(0x1012)
#define HV_ERROR_VM_GET_FAILED                           HV_HRESULT(0x1013)
#define HV_ERROR_VM_CREATE_FAILED                        HV_HRESULT(0x1014)
#define HV_ERROR_VM_DELETE_FAILED                        HV_HRESULT(0x1015)
#define HV_ERROR_VM_POWER_ON_FAILED                      HV_HRESULT(0x1016)
#define HV_ERROR_VM_POWER_OFF_FAILED                     HV_HRESULT(0x1017)
#define HV_ERROR_VM_SUSPEND_FAILED                       HV_HRESULT(0x1018)
#define HV_ERROR_VM_RESET_FAILED                         HV_HRESULT(0x1019)
#define HV_ERROR_VM_GUEST_SHUTDOWN_FAILED                HV_HRESULT(0x1020)
#define HV_ERROR_VM_GUEST_REBOOT_FAILED                  HV_HRESULT(0x1021)
#define HV_ERROR_VM_MOUNTTOOLS_FAILED                    HV_HRESULT(0x1022)
#define HV_ERROR_VM_UNMOUNTTOOLS_FAILED                  HV_HRESULT(0x1023)
#define HV_ERROR_VDISK_GET_FAILED                        HV_HRESULT(0x1024)
#define HV_ERROR_VDISK_NOT_FOUND                         HV_HRESULT(0x1025)
#define HV_ERROR_VDISK_GET_MAPPING_FAILED                HV_HRESULT(0x1026)
#define HV_ERROR_VDISK_ADD_RDM_FAILED                    HV_HRESULT(0x1027)
#define HV_ERROR_VDISK_ADD_VMDK_FAILED                   HV_HRESULT(0x1028)
#define HV_ERROR_VDISK_ADD_SHARDED_RDM_FAILED            HV_HRESULT(0x1029)
#define HV_ERROR_VDISK_REMOVE_RDM_FAILED                 HV_HRESULT(0x1030)
#define HV_ERROR_VDISK_REMOVE_VMDK_FAILED                HV_HRESULT(0x1031)
#define HV_ERROR_VDISK_REMOVE_SHARDED_RDM_FAILED         HV_HRESULT(0x1032)
#define HV_ERROR_VDISK_CHANGE_UUID_FAILED                HV_HRESULT(0x1033)
#define HV_ERROR_VNIC_GET_FAILED                         HV_HRESULT(0x1034)
#define HV_ERROR_VNETWORK_ADD_FAILED                     HV_HRESULT(0x1035)
#define HV_ERROR_VNETWORK_DELETE_FAILED                  HV_HRESULT(0x1036)
#define HV_ERROR_CONTROLLER_FOUND_FAILED                 HV_HRESULT(0x1037)
#define HV_ERROR_RESIGNATURE_VOLUMES_NOT_FOUND           HV_HRESULT(0x1038)
#define HV_ERROR_FAILED_TO_CONNECT                       HV_HRESULT(0x1039)
#define HV_ERROR_INVALID_CREDENTIALS                     HV_HRESULT(0x1040)
#define HV_ERROR_NETWORK_GET_FAILED                      HV_HRESULT(0x1041)
#define HV_ERROR_NETWORK_NOT_FOUND                       HV_HRESULT(0x1042)
#define HV_ERROR_VDISK_UNIT_CTRL_NO                      HV_HRESULT(0x1043)
#define HV_ERROR_VM_UPDATE_FAILED                        HV_HRESULT(0x1044)
#define HV_ERROR_UNITNUMBER_INUSE                        HV_HRESULT(0x1045)
#define HV_ERROR_UNITNUMBER_FOUND_FAILED                 HV_HRESULT(0x1046)
#define HV_ERROR_CLUSTER_GET_FAILED                         HV_HRESULT(0x1047)
#define HV_ERROR_CLUSTER_NOT_FOUND                        HV_HRESULT(0x1048)
#define HV_ERROR_VM_SNAPSHOT_FAILED                      HV_HRESULT(0x1049)
#define HV_ERROR_VM_SNAPSHOT_REMOVE_ALL_FAILED           HV_HRESULT(0x1050)
#define HV_ERROR_DATASTORE_NOT_FOUND                     HV_HRESULT(0x1051)
#define HV_ERROR_VDISK_NOT_ASSIGNED_TO_VM                HV_HRESULT(0x1052)
#define HV_ERROR_TASKLIST_GET_FAILED                     HV_HRESULT(0x1053)
#define HV_ERROR_TASKLIST_RESCANNINGINPROGRESS           HV_HRESULT(0x1054)
#define HV_ERROR_DATASTORE_GET_FAILED                     HV_HRESULT(0x1055)
#define HV_ERROR_FILE_INVALID                            HV_HRESULT(0x1056)
#define HV_ERROR_FILE_GET_FAILED                         HV_HRESULT(0x1057)
#define HV_ERROR_FILE_NOT_FOUND                          HV_HRESULT(0x1058)
#define HV_ERROR_VMFOLDER_GET_FAILED                     HV_HRESULT(0x1059)
#define HV_ERROR_VMFOLDER_CREATE_FAIELD                  HV_HRESULT(0x1060)
#define HV_ERROR_VMFOLDER_DUPLICATE                      HV_HRESULT(0x1061)
#define HV_WARNING_VM_CREATE_DEFAULT_FOLDER              HV_HRESULT_WARNING(0x1062)
#define HV_ERROR_RESOURCEPOOL_GET_FAILED                 HV_HRESULT(0x1063)
#define HV_WARNING_VM_CREATE_DEFAULT_RESOURCEPOOL        HV_HRESULT_WARNING(0x1064)
#define HV_ERROR_VM_CHANGE_HOST_FAILED                   HV_HRESULT(0x1065)
#define HV_ERROR_VMFOLDER_CREATE_STANDALONE              HV_HRESULT(0x1066)
#define HV_ERROR_VM_CREATE_FAILED_DUPLICATE_NAME         HV_HRESULT(0x1067)
#define HV_ERROR_FILE_DELETE_FAILED                      HV_HRESULT(0x1068)


/**
* VMWare native errors.
*/

#define VMWARE_ERROR_SUCCESS                             10000
#define VMWARE_ERROR_AlreadyExistsFault                  10001
#define VMWARE_ERROR_AlreadyUpgraded                     10002
#define VMWARE_ERROR_AuthMinimumAdminPermission          10003
#define VMWARE_ERROR_CannotAccessFile                    10004
#define VMWARE_ERROR_CannotAccessLocalSource             10005
#define VMWARE_ERROR_ConcurrentAccess                    10006
#define VMWARE_ERROR_CustomizationFault                  10007
#define VMWARE_ERROR_DasConfigFault                      10008
#define VMWARE_ERROR_DatastoreNotWritableOnHost          10009
#define VMWARE_ERROR_DuplicateName                       10010
#define VMWARE_ERROR_FileFault                           10011
#define VMWARE_ERROR_HostConfigFault                     10012
#define VMWARE_ERROR_HostConnectFault                    10013
#define VMWARE_ERROR_HostPowerOpFailed                   10014
#define VMWARE_ERROR_InaccessibleDatastore               10015
#define VMWARE_ERROR_InsufficientResourcesFault          10016
#define VMWARE_ERROR_InvalidBundle                       10017
#define VMWARE_ERROR_InvalidCollectorVersion             10018
#define VMWARE_ERROR_InvalidDatastore                    10019
#define VMWARE_ERROR_InvalidEvent                        10020
#define VMWARE_ERROR_InvalidFolder                       10021
#define VMWARE_ERROR_InvalidLicense                      10022
#define VMWARE_ERROR_InvalidLocale                       10023
#define VMWARE_ERROR_InvalidLogin                        10024
#define VMWARE_ERROR_InvalidName                         10025
#define VMWARE_ERROR_InvalidPowerState                   10026
#define VMWARE_ERROR_InvalidPrivilege                    10027
#define VMWARE_ERROR_InvalidProperty                     10028
#define VMWARE_ERROR_InvalidState                        10029
#define VMWARE_ERROR_LicenseServerUnavailable            10030
#define VMWARE_ERROR_LogBundlingFailed                   10031
#define VMWARE_ERROR_MigrationFault                      10032
#define VMWARE_ERROR_MismatchedBundle                    10033
#define VMWARE_ERROR_NoDiskFoundFault                    10034
#define VMWARE_ERROR_NoDiskSpaceFault                    10035
#define VMWARE_ERROR_NotFoundFault                       10036
#define VMWARE_ERROR_NotSupportedFault                   10037
#define VMWARE_ERROR_OutOfBoundsFault                    10038
#define VMWARE_ERROR_PatchBinariesNotFoundFault          10039
#define VMWARE_ERROR_PatchInstallFailedFault             10040
#define VMWARE_ERROR_PatchMetadataInvalidFault           10041
#define VMWARE_ERROR_PatchNotApplicableFault             10042
#define VMWARE_ERROR_PlatformConfigFaultFault            10043
#define VMWARE_ERROR_RebootRequiredFault                 10044
#define VMWARE_ERROR_RemoveFailedFault                   10045
#define VMWARE_ERROR_RequestCanceledFault                10046
#define VMWARE_ERROR_ResourceInUseFault                  10047
#define VMWARE_ERROR_RuntimeFaultFault                   10048
#define VMWARE_ERROR_SSPIChallengeFault                  10049
#define VMWARE_ERROR_SnapshotFaultFault                  10050
#define VMWARE_ERROR_TaskInProgressFault                 10051
#define VMWARE_ERROR_TimedoutFault                       10052
#define VMWARE_ERROR_TooManyHostsFault                   10053
#define VMWARE_ERROR_ToolsUnavailableFault               10054
#define VMWARE_ERROR_UserNotFoundFault                   10055
#define VMWARE_ERROR_VimFaultFault                       10056
#define VMWARE_ERROR_VmConfigFaultFault                  10057
#define VMWARE_ERROR_VmToolsUpgradeFaultFault            10058
#define VMWARE_ERROR_GENERALERROR                        10059

}}}
//_VERSION  "1.0.803"

namespace cpp saasame.transport
namespace csharp saasame.transport
namespace java saasame.transport
namespace php saasame.transport

const string CONFIG_PATH                = "Software\\SaaSame\\Transport"
const string SCHEDULER_SERVICE          = "{6FC9C4B0-B61E-4745-A6AA-1D5F0A2DA08B}"
const string PHYSICAL_PACKER_SERVICE    = "{6FC9C4B1-B61E-4745-A6AA-1D5F0A2DA08B}"
const string VIRTUAL_PACKER_SERVICE     = "{6FC9C4B2-B61E-4745-A6AA-1D5F0A2DA08B}"
const string CARRIER_SERVICE            = "{6FC9C4B3-B61E-4745-A6AA-1D5F0A2DA08B}"
const string LOADER_SERVICE             = "{6FC9C4B4-B61E-4745-A6AA-1D5F0A2DA08B}"
const string LAUNCHER_SERVICE           = "{6FC9C4B5-B61E-4745-A6AA-1D5F0A2DA08B}"
const string LINUX_LAUNCHER_SERVICE     = "{6FC9C4B6-B61E-4745-A6AA-1D5F0A2DA08B}"
const string TRANSPORTER_SERVICE        = "{6FC9C4B7-B61E-4745-A6AA-1D5F0A2DA08B}"

const string TRANSPORTER_SERVICE_DISPLAY_NAME = "SaaSame Transporter Service"
const string TRANSPORTER_SERVICE_DESCRIPTION  = "SaaSame Transporter Service"

const string SCHEDULER_SERVICE_DISPLAY_NAME = "SaaSame Scheduler Service"
const string SCHEDULER_SERVICE_DESCRIPTION  = "SaaSame Scheduler Service"

const string PHYSICAL_PACKER_SERVICE_DISPLAY_NAME = "SaaSame Host Packer Service"
const string PHYSICAL_PACKER_SERVICE_DESCRIPTION  = "SaaSame Host Packer Service"

const string VIRTUAL_PACKER_SERVICE_DISPLAY_NAME = "SaaSame Virtual Packer Service"
const string VIRTUAL_PACKER_SERVICE_DESCRIPTION  = "SaaSame Virtual Packer Service"

const string CARRIER_SERVICE_DISPLAY_NAME = "SaaSame Carrier Service"
const string CARRIER_SERVICE_DESCRIPTION  = "SaaSame Carrier Service"

const string LOADER_SERVICE_DISPLAY_NAME = "SaaSame Loader Service"
const string LOADER_SERVICE_DESCRIPTION  = "SaaSame Loader Service"

const string LAUNCHER_SERVICE_DISPLAY_NAME = "SaaSame Launcher Service"
const string LAUNCHER_SERVICE_DESCRIPTION  = "SaaSame Launcher Service"

const i32   SCHEDULER_SERVICE_PORT          = 18888
const i32   PHYSICAL_PACKER_SERVICE_PORT    = 18889
const i32   VIRTUAL_PACKER_SERVICE_PORT     = 18890
const i32   CARRIER_SERVICE_PORT            = 18891
const i32   LOADER_SERVICE_PORT             = 18892
const i32   LAUNCHER_SERVICE_PORT           = 18893
const i32   LAUNCHER_EMULATOR_PORT          = 18894
const i32   TRANSPORT_SERVICE_PORT          = 18895
const i32   TRANSPORT_SERVICE_HTTP_PORT     = 18896
const i32   CARRIER_SERVICE_HTTP_PORT       = 18897
const i32   CARRIER_SERVICE_SSL_PORT        = 28891
const i32   MANAGEMENT_SERVICE_PORT         = 443
const string MANAGEMENT_SERVICE_PATH        = "/mgmt/default.php"
const string CARRIER_SERVICE_PATH           = "/carrier"
const string TRANSPORTER_SERVICE_PATH       = "/transporter"

enum enumerate_disk_filter_style {
  ALL_DISK                    = 0,
  UNINITIALIZED_DISK          = 1
}

enum machine_detail_filter {
  FULL          = 0,
  SIMPLE        = 1
}

enum partition_style {
  PARTITION_UNKNOWN    = 0,
  PARTITION_MBR        = 1,
  PARTITION_GPT        = 2
}

enum drive_type {
  DT_UNKNOWN       = 0,
  DT_NO_ROOT_PATH  = 1,
  DT_REMOVABLE     = 2,
  DT_FIXED         = 3,
  DT_REMOTE        = 4,
  DT_CDROM         = 5,
  DT_RAMDISK       = 6
}

enum bus_type{
    Unknown               = 0,  //The bus type is unknown.
    SCSI                  = 1,  //SCSI
    ATAPI                 = 2,  //ATAPI
    ATA                   = 3,  //ATA
    IEEE_1394             = 4,  //IEEE 1394
    SSA                   = 5,  //SSA
    Fibre_Channel         = 6,  // Fibre Channel
    USB                   = 7,  // USB
    RAID                  = 8,  // RAID
    iSCSI                 = 9,  //iSCSI
    SAS                   = 10, // Serial Attached SCSI (SAS)
    SATA                  = 11, //Serial ATA (SATA)
    SD                    = 12, //Secure Digital (SD)
    MMC                   = 13, //Multimedia Card (MMC)
    Virtual               = 14, //This value is reserved for system use.
    File_Backed_Virtual   = 15, //File-Backed Virtual
    Storage_Spaces        = 16, //Storage spaces
    NVMe                  = 17  //NVMe
}

struct disk_info
{
    1: optional bool                boot_from_disk         = false
    2: optional bus_type            bus_type               = 0
    3: optional string              cluster_owner          = ""
    4: optional i64                 cylinders              = 0
    5: optional string              friendly_name          = ""
    6: optional string              guid                   = ""
    7: optional i32                 tracks_per_cylinder    = 0
    8: optional bool                is_boot                = false
    9: optional bool                is_clustered           = false
   10: optional bool                is_offline             = false
   11: optional bool                is_readonly            = false
   12: optional bool                is_snapshot            = false
   13: optional bool                is_system              = false
   14: optional string              location               = ""
   15: optional i32                 logical_sector_size    = 0
   16: optional string              manufacturer           = ""
   17: optional string              model                  = ""
   18: optional i32                 number                 = -1
   19: optional i32                 number_of_partitions   = -1
   20: optional i16                 offline_reason         = 0
   21: optional partition_style     partition_style        = 0
   22: optional string              path                   = ""
   23: optional i32                 physical_sector_size   = 0
   24: optional i32                 sectors_per_track      = 0
   25: optional string              serial_number          = ""
   26: optional i32                 signature              = 0
   27: optional i64                 size                   = 0
   28: optional string              uri                    = ""
   29: optional i32                 scsi_bus               = 0
   30: optional i16                 scsi_logical_unit      = 0
   31: optional i16                 scsi_port              = 0
   32: optional i16                 scsi_target_id         = 0
   33: optional string              unique_id               = ""
   34: optional i16                 unique_id_format       = 0
   35: optional string              customized_id           = ""
}

// gpt_type
//                            value                                            mean
// System Partition     c12a7328-f81f-11d2-ba4b-00a0c93ec93b  An EFI system partition.
// Microsoft Reserved   e3c9e316-0b5c-4db8-817d-f92df00215ae  A Microsoft reserved partition.
// Basic data           ebd0a0a2-b9e5-4433-87c0-68b6b72699c7  A basic data partition. This is the data partition type that is created and recognized by Windows.
//                                                                              Only partitions of this type can be assigned drive letters, receive volume GUID paths, host mounted folders
//                                                                              (also called volume mount points) and be enumerated by calls to FindFirstVolume and FindNextVolume.
// LDM Metadata          5808c8aa-7e8f-42e0-85d2-e1e90434cfb3  A Logical Disk Manager (LDM) metadata partition on a dynamic disk.
// LDM Data              af9b60a0-1431-4f62-bc68-3311714a69ad  The partition is an LDM data partition on a dynamic disk.
// Microsoft Recovery    de94bba4-06d1-4d40-a16a-bfd50179d6ac  A Microsoft recovery partition.

// mbr_type
// FAT12 (1)
// FAT16 (4)
// Extended (5)
// Huge (6)
// IFS (7)
// FAT32 (12)

struct partition_info
{
    1: optional set<string> access_paths
    2: optional i32         disk_number               = -1
    3: optional string      drive_letter              = ""
    4: optional string      gpt_type                  = ""
    5: optional string      guid                      = ""
    6: optional bool        is_active                 = false
    7: optional bool        is_boot                   = false
    8: optional bool        is_hidden                 = false
    9: optional bool        is_offline                = false
   10: optional bool        is_readonly               = false
   11: optional bool        is_shadowcopy             = false
   12: optional bool        is_system                 = false
   13: optional i16         mbr_type                  = 0
   14: optional i64         offset                    = 0
   15: optional i32         partition_number          = -1
   16: optional i64         size                      = 0
}

struct volume_info
{
    1: optional set<string>    access_paths
    2: optional string         cluster_access_path        = ""
    3: optional string         drive_letter               = ""
    4: optional drive_type     drive_type                 = 0
    5: optional string         file_system                = ""
    6: optional string         file_system_catalogid      = ""
    7: optional string         file_system_label          = ""
    8: optional string         object_id                  = ""
    9: optional string         path                       = ""
   10: optional i64            size                       = 0
   11: optional i64            size_remaining             = 0
}

struct network_info
{
    1: optional string         adapter_name            = ""
    2: optional string         description             = ""
    3: optional list<string>   dnss
    4: optional list<string>   gateways
    5: optional list<string>   ip_addresses
    6: optional bool           is_dhcp_v4              = false
    7: optional bool           is_dhcp_v6              = false
    8: optional string         mac_address             = ""
    9: optional list<string>   subnet_masks
}

struct cluster_network
{
    1: optional string cluster_network_name              = ""
    2: optional string cluster_network_id                = ""
    3: optional string cluster_network_address           = ""
    4: optional string cluster_network_address_mask      = ""
    5: optional set<network_info> network_infos
}

struct cluster_group
{
    1: optional string               group_id                   = ""
    2: optional string               group_name                 = ""
    3: optional string               group_owner                = ""
    4: optional set<disk_info>       cluster_disks
    5: optional set<volume_info>     cluster_partitions
    6: optional set<cluster_network> cluster_network_infos
}

struct cluster_info
{
    1: optional string                cluster_name      = ""
    2: optional disk_info             quorum_disk
    3: optional set<string>           cluster_nodes
    4: optional set<string>           client_ids
    5: optional set<string>           machine_ids
    6: optional set<cluster_network>  cluster_network_infos
    7: optional set<cluster_group>    cluster_groups
}

struct os_version_info
{
    1: optional string csd_version              = ""
    2: optional i32 build_number                = -1
    3: optional i32 major_version               = -1
    4: optional i32 minor_version               = -1
    5: optional i32 platform_id                 = -1
    6: optional i16 product_type                = -1
    7: optional i32 servicepack_major           = -1
    8: optional i32 servicepack_minor           = -1
    9: optional i32 suite_mask                  = -1
}

struct snapshot
{
    1: optional string         snapshot_set_id             = ""
    2: optional string         snapshot_id                 = ""
    3: optional string         original_volume_name        = ""
    4: optional string         snapshot_device_object      = ""
    5: optional string         creation_time_stamp         = ""
    6: optional i32            snapshots_count             = 0
}

struct snapshot_result
{
    1: list<snapshot>    snapshots;
}

struct volume_bit_map{
    1: optional i32     cluster_size                     = 0
    2: optional i64     starting_lcn                     = 0
    3: optional i64     total_number_of_clusters         = 0
    4: optional binary  bit_map                          = ""
    5: optional bool    compressed                       = false
}

struct replication_result
{
    1: optional binary result                     = ""
    2: optional bool   compressed                 = false
}

struct delete_snapshot_result
{
    1: optional i32        code                        = 0
    2: optional i32        deleted_snapshots           = 0
    3: optional string     non_deleted_snapshot_id
}


struct physical_machine_info
{
   1: optional string              architecture            = ""
   2: optional string              client_id               = ""
   3: optional string              client_name             = ""
   4: optional string              domain                  = ""
   5: optional string              hal                     = ""
   6: optional string              initiator_name          = ""
   7: optional bool                is_oem                  = false
   8: optional i16                 logical_processors      = 0
   9: optional string              machine_id              = ""
  10: optional string              manufacturer            = ""
  11: optional string              os_name                 = ""
  12: optional i32                 os_type                 = 0
  13: optional string              os_system_info
  14: optional i64                 physical_memory         = 0
  15: optional i16                 processors              = 0
  16: optional i32                 role                    = 0
  17: optional string              system_model            = ""
  18: optional string              system_root             = ""
  19: optional string              workgroup               = ""
  20: optional os_version_info     os_version
  21: optional set<cluster_info>   cluster_infos
  22: optional set<disk_info>      disk_infos
  23: optional set<network_info>   network_infos
  24: optional set<partition_info> partition_infos
  25: optional set<volume_info>    volume_infos
  26: optional bool                is_vcbt_driver_installed = false
  27: optional bool                is_vcbt_enabled          = false
  28: optional string              current_vcbt_version
  29: optional string              installed_vcbt_version
  30: optional bool                is_winpe                    = false
  31: optional i16                 system_default_ui_language  = 1033
}

enum hv_vm_tools_status
{
     HV_VMTOOLS_UNKNOWN = 0,
     HV_VMTOOLS_OK = 1,
     HV_VMTOOLS_NOTINSTALLED = 2,
     HV_VMTOOLS_OLD = 3,
     HV_VMTOOLS_NOTRUNNING = 4,
     HV_VMTOOLS_NEEDUPGRADE = 5,
     HV_VMTOOLS_UNMANAGED = 6,
     HV_VMTOOLS_NEW = 7,
     HV_VMTOOLS_BLACKLISTED = 8
}

enum hv_vm_power_state
{
     HV_VMPOWER_UNKNOWN = 0,
     HV_VMPOWER_ON = 1,
     HV_VMPOWER_OFF = 2,
     HV_VMPOWER_SUSPENDED = 3,
}

enum hv_vm_connection_state
{
     HV_VMCONNECT_UNKNOWN = 0,
     HV_VMCONNECT_CONNECTED = 1,
     HV_VMCONNECT_DISCONNECTED = 2,
     HV_VMCONNECT_INACCESSIBLE = 3,
     HV_VMCONNECT_INVALID = 4,
     HV_VMCONNECT_ORPHANED = 5
}

enum hv_host_power_state
{
     HV_HOSTPOWER_UNKNOWN = 0,
     HV_HOSTPOWER_ON = 1,
     HV_HOSTPOWER_OFF = 2,
     HV_HOSTPOWER_STANDBY = 3,
}

enum hv_connection_type
{
    HV_CONNECTION_TYPE_UNKNOWN = 0,
    HV_CONNECTION_TYPE_VCENTER = 1,
    HV_CONNECTION_TYPE_HOST = 2
}

// Guest OS
enum hv_guest_os_type
{
     HV_OS_UNKNOWN = 0,
     HV_OS_WINDOWS = 1,
     HV_OS_LINUX = 2
}

enum hv_controller_type
{
    HV_CTRL_ANY = -1,
    HV_CTRL_IDE = 0,
    HV_CTRL_PARA_VIRT_SCSI = 1,
    HV_CTRL_BUS_LOGIC = 2,
    HV_CTRL_LSI_LOGIC = 3,
    HV_CTRL_LSI_LOGIC_SAS = 4
}

// Boot Firmware
enum hv_vm_firmware
{
     HV_VM_FIRMWARE_BIOS = 0,
     HV_VM_FIRMWARE_EFI = 1
}

struct virtual_host{
     1: optional string                name_ref                      = ""
     2: optional string                name                          = ""
     3: optional list<string>          ip_addresses
     4: optional string                ip_address                    = ""
     5: optional string                product_name                  = ""
     6: optional string                version                       = ""
     7: optional hv_host_power_state   power_state                   = 0
     8: optional string                state                         = ""
     9: optional bool                  in_maintenance_mode           = false
    10: optional map<string,string>    vms
    11: optional map<string,string>    datastores
    12: optional map<string,string>    networks
    13: optional string                datacenter_name               = ""
    14: optional string                domain_name                   = ""
    15: optional string                cluster_key                   = ""
    16: optional string                full_name                     = ""
    17: optional map<string, list<string>>      lic_features
    18: optional list<string>          name_list
    19: optional list<string>          domain_name_list
    20: optional hv_connection_type    connection_type                 = 0
    21: optional string                virtual_center_name
    22: optional string                virtual_center_version
    23: optional string                uuid
    24: optional i16				   number_of_cpu_cores            =  0
    25: optional i16                   number_of_cpu_packages         =  0
    26: optional i64                   size_of_memory                 =  0
}

struct virtual_network_adapter{
     1: optional i32                   key                                     = 0
     2: optional string                name                                    = ""
     3: optional string                mac_address                             = ""
     4: optional string                network                                 = ""
     5: optional string                port_group                              = ""
     6: optional string                type                                    = ""
     7: optional bool                  is_connected                            = false
     8: optional bool                  is_start_connected                      = false
     9: optional bool                  is_allow_guest_control                  = false
    10: optional string                address_type                            = ""
    11: optional list<string>          ip_addresses
}

struct virtual_machine_snapshots
{
     1: optional string                                             name                          = ""
     2: optional string                                             description                   = ""
     3: optional string                                             create_time                   = ""
     4: optional bool                                               quiesced                      = false
     5: optional i32                                                id                            = -1
     6: optional string                                             backup_manifest               = ""
     7: optional bool                                               replay_supported              = false
     8: optional list<virtual_machine_snapshots>                    child_snapshot_list
}

struct virtual_disk_info
{
     1: optional string             key                                 = ""
     2: optional string             name                                = ""
     3: optional string             id                                  = ""
     4: optional i64                size_kb                             = 0
     5: optional i64                size                                = 0
     6: optional hv_controller_type controller_type                     = -1
     7: optional bool               thin_provisioned                    = false
}

struct virtual_machine{
     1: optional string                                                uuid                          = ""
     2: optional string                                                name                          = ""
     3: optional string                                                host_key                      = ""
     4: optional string                                                host_ip                       = ""
     5: optional string                                                host_name                     = ""
     6: optional string                                                cluster_key                   = ""
     7: optional string                                                cluster_name                  = ""
     8: optional string                                                annotation                    = ""
     9: optional bool                                                  is_cpu_hot_add                = false
    10: optional bool                                                  is_cpu_hot_remove             = false
    11: optional i32                                                   memory_mb                     = 0
    12: optional i32                                                   number_of_cpu                 = 0
    13: optional bool                                                  is_template                   = false
    14: optional string                                                config_path                   = ""
    15: optional string                                                config_path_file              = ""
    16: optional i32                                                   version                       = 0
    17: optional hv_vm_power_state                                     power_state                   = 0
    18: optional hv_vm_connection_state                                connection_state              = 0
    19: optional hv_vm_tools_status                                    tools_status                  = 0
    20: optional hv_vm_firmware                                        firmware                      = 0
    21: optional hv_guest_os_type                                      guest_os_type                 = 0
    22: optional string                                                guest_id                      = ""
    23: optional string                                                guest_os_name                 = ""
    24: optional bool                                                  is_disk_uuid_enabled          = false
    25: optional string                                                folder_path                   = ""
    26: optional string                                                resource_pool_path            = ""
    27: optional list<virtual_disk_info>                               disks
    28: optional map<string,string>                                    networks
    29: optional string                                                datacenter_name               = ""
    30: optional list<virtual_network_adapter>                         network_adapters
    31: optional list<virtual_machine_snapshots>                       root_snapshot_list
    32: optional string                                                guest_host_name               = ""
    33: optional string                                                guest_ip                      = ""
    34: optional bool                                                  has_cdrom                     = false
}

enum job_type {
  physical_packer_job_type   = 1,
  virtual_packer_job_type    = 2,
  physical_transport_type    = 3,
  virtual_transport_type     = 4,
  loader_job_type            = 5,
  launcher_job_type          = 6,
  winpe_packer_job_type      = 7,
  winpe_transport_job_type   = 8
}

exception invalid_operation {
  1: i32                             what_op,
  2: string                          why,
  5: optional string                 format                    = ""
  6: optional list<string>           arguments
}

enum job_trigger_type{
    runonce_trigger  = 0,
    interval_trigger = 1
}

struct job_trigger{
    1: optional job_trigger_type      type          = 1
    2: optional string                start         = ""
    3: optional string                finish        = ""
    4: optional i32                   interval      = 15
    5: optional string                id            = ""
    6: optional i32                   duration      = 0   //minutes
}

enum job_state{
     job_state_none             = 0x1,
     job_state_initialed        = 0x2,
     job_state_replicating      = 0x4,
     job_state_replicated       = 0x8,
     job_state_converting       = 0x10,
     job_state_finished         = 0x20,
     job_state_sche_completed   = 0x40,
     job_state_recover          = 0x80,
     job_state_resizing         = 0x100,
     job_state_uploading        = 0x200,
     job_state_upload_completed = 0x400,
     job_state_discard          = 0x40000000
}

enum error_codes{
SAASAME_S_OK                           = 0x00000000,  // "%1."
SAASAME_NOERROR                        = 0x00000000,  // "%1."
SAASAME_E_FAIL                         = 0x00001000,  // "%1."
SAASAME_E_INITIAL_FAIL                 = 0x00001001,  //  "%1 initial failure."
SAASAME_E_DISK_FULL                    = 0x00001002,  //  "Disk space full."
SAASAME_E_INVALID_ARG                  = 0x00001003,  //  "Invalid arguments."
SAASAME_E_INVALID_AUTHENTICATION       = 0x00001004,  //  "Authenticate to '%1' failure."
SAASAME_E_INTERNAL_FAIL                = 0x00001005,  //  "Fatal error: '%1', errno: %2."
SAASAME_E_CANNOT_CONNECT_TO_HOST       = 0x00001006,  //  "Failed to connect to '%1'."
SAASAME_E_QUEUE_FULL                   = 0x00001007,  //  "The queue limitation reachs."
SAASAME_E_INVALID_LICENSE_KEY          = 0x00001008,  //  "Invalid license key."
SAASAME_E_INVALID_LICENSE              = 0x00001009,  //  "Invalid license."
SAASAME_E_JOB_CREATE_FAIL              = 0x00003000,  //  "Create job failure."
SAASAME_E_JOB_REMOVE_FAIL              = 0x00003001,  //  "Remove job '%1' failure."
SAASAME_E_JOB_NOTFOUND                 = 0x00003002,  //  "Job '%1' is not found."
SAASAME_E_JOB_CONFIG_NOTFOUND          = 0x00003003,  //  "Job '%1' configuration is not found."
SAASAME_E_JOB_STATUS_NOTFOUND          = 0x00003004,  //  "Job '%1' status is not found."
SAASAME_E_JOB_CANCELLED                = 0x00003005,  //  "Job '%1' is cancelled."
SAASAME_E_JOB_CONVERT_FAIL             = 0x00003006,  //  "Job '%1' failed at convert state, error: %2"
SAASAME_E_JOB_REPLICATE_FAIL           = 0x00003007,  //  "Job '%1' failed at replicate state, error: %2"
SAASAME_E_JOB_ID_DUPLICATED            = 0x00003008,  //  "Job '%1' is duplicated."
SAASAME_E_JOB_INTERRUPTED              = 0x00003009,  //  "Job '%1' is interrupted."
SAASAME_E_JOB_RUNNING                  = 0x0000300a,  //  "Job '%1' is running."
SAASAME_E_JOB_RESPONSE                 = 0x0000300b,  //  "Job '%1' error response."
SAASAME_E_PHYSICAL_CONFIG_FAILED       = 0x00004000,  //  "Failed to configure the host agent on machine %1"
SAASAME_E_VIRTUAL_VM_NOTFOUND          = 0x00005000,  //  "Virtual machine '%1' not found."
SAASAME_E_IMAGE_NOTFOUND               = 0x00006000,  //  "Image '%1' not found."
SAASAME_E_IMAGE_CREATE_FAIL            = 0x00006001,  //  "Failed to create '%1' image."
SAASAME_E_IMAGE_OPEN_FAIL              = 0x00006002,  //  "Failed to open '%1' image."
SAASAME_E_IMAGE_READ                   = 0x00006003,  //  "Image '%1' read failure."
SAASAME_E_IMAGE_WRITE                  = 0x00006004,  //  "Image '%1' write failure."
SAASAME_E_IMAGE_OUTOFRANGE             = 0x00006005,  //  "The offset '%1' is out of range of the '%2' image."
SAASAME_E_IMAGE_ATTACH_FAIL            = 0x00006006,  //  "Mount '%1' image failure."
SAASAME_E_IMAGE_DETACH_FAIL            = 0x00006007,  //  "Unmount '%1' image failure."
SAASAME_E_IMAGE_PROPERTY_FAIL          = 0x00006008,  //  "Failed to get '%1' image's properties."
SAASAME_E_SNAPSHOT_CREATE_FAIL         = 0x00007000,  //  "Create '%1' snapshot for '%2' failure."
SAASAME_E_SNAPSHOT_REMOVE_FAIL         = 0x00007001,  //  "Remove '%1' snapshot failure."
SAASAME_E_SNAPSHOT_NOTFOUND            = 0x00007002,  //  "'%1' snapshot not found."
SAASAME_E_SNAPSHOT_INVALID             = 0x00007003   //  "The specified '%1' snapshot is invalid."
}

struct job_history{
    1: optional string              time                      = ""
    2: optional job_state           state                     = 1
    3: optional i32                 error                     = 0
    4: optional string              description               = ""
    7: optional string              format                    = ""
    8: optional list<string>        arguments
    9: optional bool                is_display                = true
}

struct create_job_detail{
    1: optional job_type                type                        = 1
    2: optional list<job_trigger>       triggers
    3: optional string                  management_id               = ""
    4: optional set<string>             mgmt_addr
    5: optional i32                     mgmt_port                   = 80
    6: optional bool                    is_ssl                      = false
}

struct packer_disk_image{
     1: optional string                  name                       = ""
     2: optional string                  parent                     = ""
     3: optional string                  base                       = ""
}

struct virtual_partition_info
{
    1: optional i32                          partition_number                    = 0
    2: optional i64                          offset                              = 0
    3: optional i64                          size                                = 0
}

struct virtual_disk_info_ex
{
     1: optional string                       id                                  = ""
     2: optional i64                          size                                = 0
     3: optional partition_style              partition_style                     = 0
     4: optional string                       guid                                = ""
     5: optional i32                          signature                           = 0
     6: optional bool                         is_system                           = false
     7: optional set<virtual_partition_info>  partitions
}

struct virtual_create_packer_job_detail{
     1: optional set<string>                        disks
     2: optional string                             host                       = ""
     3: optional set<string>                        addr
     4: optional string                             username                   = ""
     5: optional string                             password                   = ""
     6: optional string                             virtual_machine_id         = ""
     9: optional string                             snapshot                   = ""
     10: optional map<string,packer_disk_image>     images;
     14: optional map<string, i64>                  backup_size
     15: optional map<string, i64>                  backup_progress
     16: optional map<string, i64>                  backup_image_offset
     17: optional map<string,string>                previous_change_ids
     18: optional map<string, list<io_changed_range>>   completed_blocks
}

struct physical_vcbt_journal{
     1: optional i64                                  id                     = 0
     2: optional i64                                  first_key              = 0
     3: optional i64                                  latest_key             = 0
     4: optional i64                                  lowest_valid_key       = 0
}

struct io_changed_range{
    1: i64 offset             = 0
    2: i64 start              = 0
    3: i64 length             = 0
}

struct physical_create_packer_job_detail{
     1: optional set<string>                    disks
     2: optional list<snapshot>                 snapshots
     3: optional map<i64,physical_vcbt_journal> previous_journals;
     4: optional map<string,packer_disk_image>  images;
     5: optional map<string, i64>               backup_size
     6: optional map<string, i64>               backup_progress
     7: optional map<string, i64>               backup_image_offset
     8: optional map<i64,physical_vcbt_journal> cdr_journals
     9: optional map<string, list<io_changed_range>> cdr_changed_ranges
    10: optional map<string, list<io_changed_range>> completed_blocks
    11: optional set<string>                         excluded_paths
    12: optional set<string>                         resync_paths
}

union _create_packer_job_detail{
    1: physical_create_packer_job_detail p
    2: virtual_create_packer_job_detail  v
}

struct create_packer_job_detail{
     1: job_type                                       type
     2: set<string>                                    connection_ids
     3: map<string,set<string>>                        carriers
     4: _create_packer_job_detail                      detail
     5: optional bool                                  checksum_verify             = true
     6: optional i32                                   timeout                     = 300
     7: optional bool                                  is_encrypted                = false
     8: optional i32                                   worker_thread_number        = 0
     9: optional bool                                  file_system_filter_enable   = true
    10: optional i32                                   min_transport_size          = 0        //KB 0 => default ( Range 64KB ~ 8MB)
    11: optional i32                                   full_min_transport_size     = 0        //KB 0 => default ( Range 64KB ~ 8MB)
    12: optional bool                                  is_compressed               = true
    13: optional bool                                  is_checksum                 = false
    14: optional map<string,string>                    priority_carrier
    15: optional bool                                  is_only_single_system_disk  = false
    16: optional bool                                  is_compressed_by_packer     = false
}

struct virtual_packer_job_detail{
     1: optional map<string, i64>                      original_size
     2: optional map<string, i64>                      backup_size
     3: optional map<string, i64>                      backup_progress
     4: optional map<string, i64>                      backup_image_offset
     5: optional map<string,string>                    change_ids
     6: optional hv_guest_os_type                      guest_os_type       = 0
     7: optional map<string, list<io_changed_range>>   completed_blocks
     8: optional list<virtual_disk_info_ex>            disk_infos
}

struct physical_packer_job_detail{
     1: optional map<string, i64>                      original_size
     2: optional map<string, i64>                      backup_size
     3: optional map<string, i64>                      backup_progress
     4: optional map<string, i64>                      backup_image_offset
     5: optional map<i64, physical_vcbt_journal>       vcbt_journals
     6: optional hv_guest_os_type                      guest_os_type       = 1
     7: optional map<string, list<io_changed_range>>   cdr_changed_ranges
     8: optional map<string, list<io_changed_range>>   completed_blocks
}

union _packer_job_detail{
    1: physical_packer_job_detail p
    2: virtual_packer_job_detail  v
}

struct packer_job_detail{
    1: optional string                                id                              = ""
    2: optional job_type                              type
    3: optional job_state                             state                           = 1
    4: optional string                                created_time                    = ""
    5: optional string                                updated_time                    = ""
    6: optional list<job_history>                     histories
    7: optional _packer_job_detail                    detail
    8: optional bool                                  is_error                        = false
    9: optional string                                boot_disk                       = ""
   10: optional list<string>                          system_disks
   11: optional map<string, list<io_changed_range>>   completed_blocks
}

struct replica_job_detail{
    1: optional string                                replica_id                      = ""
    2: optional string                                host                            = ""
    3: optional string                                id                              = ""
    4: optional job_type                              type
    5: optional job_state                             state                           = 1
    6: optional bool                                  is_error                        = false
    7: optional string                                created_time                    = ""
    8: optional string                                updated_time                    = ""
    9: optional string                                virtual_machine_id              = ""
   10: optional set<string>                           disks
   11: optional string                                connection_id
   12: optional map<string, i64>                      original_size
   13: optional map<string, i64>                      backup_progress
   14: optional map<string, string>                   snapshot_mapping
   15: optional map<string, i64>                      backup_size
   16: optional map<string, i64>                      backup_image_offset
   17: optional string                                cbt_info                        = ""
   18: optional list<job_history>                     histories
   19: optional string                                snapshot_time                   = ""
   20: optional string                                snapshot_info                   = ""
   21: optional string                                boot_disk                          = ""
   22: optional list<string>                          system_disks
   23: optional bool                                  is_pending_rerun                = false
   24: optional bool                                  is_cdr                          = false
   25: optional list<virtual_disk_info_ex>            virtual_disk_infos
   26: optional set<string>                           excluded_paths
}

struct service_info{
    1: string id
    2: string version
    3: string path
}

struct vmware_snapshot{
     1: optional string                           id                    = ""
     2: optional string                           name                  = ""
     3: optional string                           datetime              = ""
}

service common_service
{
    service_info                   ping();
    physical_machine_info          get_host_detail  ( 1: string session_id, 2: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    set<service_info>              get_service_list ( 1: string session_id ) throws (1:invalid_operation ouch);
    set<disk_info>                 enumerate_disks  ( 1: enumerate_disk_filter_style filter)  throws (1:invalid_operation ouch);
    bool                           verify_carrier ( 1: string carrier, 2: bool is_ssl );
    binary                         take_xray()    throws (1:invalid_operation ouch);
    binary                         take_xrays()    throws (1:invalid_operation ouch);
    bool                           create_mutex( 1: string session, 2: i16 timeout ) throws (1:invalid_operation ouch);
    bool                           delete_mutex( 1: string session ) throws (1:invalid_operation ouch);
}

service scheduler_service extends common_service
{
    physical_machine_info                    get_physical_machine_detail( 1: string session_id, 2: string host, 3: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    physical_machine_info                    get_physical_machine_detail2( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    virtual_host                             get_virtual_host_info( 1: string session_id, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    virtual_machine                          get_virtual_machine_detail ( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    replica_job_detail                       create_job_ex ( 1: string session_id, 2: string job_id, 3: create_job_detail create_job ) throws (1:invalid_operation ouch);
    replica_job_detail                       create_job ( 1: string session_id, 2: create_job_detail create_job ) throws (1:invalid_operation ouch);
    replica_job_detail                       get_job( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     interrupt_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     resume_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     remove_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    list<replica_job_detail>                 list_jobs( 1: string session_id ) throws (1:invalid_operation ouch);
    bool                                     update_job( 1: string session_id, 2: string job_id, 3: create_job_detail create_job ) throws (1:invalid_operation ouch);
    void                                     terminate( 1: string session_id );
    bool                                     running_job( 1: string session_id, 2: string job_id) throws (1:invalid_operation ouch);
    bool                                     verify_management ( 1: string management, 2: i32 port, 3: bool is_ssl );
    bool                                     verify_packer_to_carrier ( 1: string packer, 2: string carrier, 3: i32 port, 4: bool is_ssl);
    binary                                   take_packer_xray( 1: string session_id, 2: string host ) throws (1:invalid_operation ouch);
    service_info                             get_packer_service_info( 1: string session_id, 2: string host ) throws (1:invalid_operation ouch);
}

struct take_snapshots_parameters{
    1: set<string> disks
    2: string pre_script
    3: string post_script
    4: set<string> excluded_paths
}

service physical_packer_service extends common_service
{
    list<snapshot>                           take_snapshots      ( 1: string session_id , 2: set<string> disks ) throws (1:invalid_operation ouch);
    list<snapshot>                           take_snapshots_ex   ( 1: string session_id , 2: set<string> disks, 3: string pre_script, 4: string post_script) throws (1:invalid_operation ouch);
    list<snapshot>                           take_snapshots2     ( 1: string session_id , 2: take_snapshots_parameters parameters) throws (1:invalid_operation ouch);
    delete_snapshot_result                   delete_snapshot     ( 1: string session_id , 2: string snapshot_id ) throws (1:invalid_operation ouch);
    delete_snapshot_result                   delete_snapshot_set ( 1: string session_id , 2: string snapshot_set_id ) throws (1:invalid_operation ouch);
    map<string,list<snapshot>>               get_all_snapshots    ( 1: string session_id ) throws (1:invalid_operation ouch);
    packer_job_detail                        create_job_ex ( 1: string session_id, 2: string job_id, 3: create_packer_job_detail create_job ) throws (1:invalid_operation ouch);
    packer_job_detail                        create_job ( 1: string session_id, 2: create_packer_job_detail create_job ) throws (1:invalid_operation ouch);
    packer_job_detail                        get_job( 1: string session_id, 2: string job_id, 3: string previous_updated_time ) throws (1:invalid_operation ouch);
    bool                                     interrupt_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     resume_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     remove_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    list<packer_job_detail>                  list_jobs( 1: string session_id ) throws (1:invalid_operation ouch);
    void                                     terminate( 1: string session_id );
    bool                                     running_job( 1: string session_id, 2: string job_id) throws (1:invalid_operation ouch);
    bool                                     unregister(1: string session ) throws (1:invalid_operation ouch);
}

service virtual_packer_service extends common_service
{
    virtual_host                             get_virtual_host_info( 1: string session_id, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    list<virtual_host>                       get_virtual_hosts( 1: string session_id, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    virtual_machine                          get_virtual_machine_detail ( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                                     power_off_virtual_machine( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                                     remove_virtual_machine( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    list<vmware_snapshot>                    get_virtual_machine_snapshots( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                                     remove_virtual_machine_snapshot( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id, 6: string snapshot_id ) throws (1:invalid_operation ouch);
    list<string>                             get_datacenter_folder_list( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string datacenter ) throws (1:invalid_operation ouch);
    packer_job_detail                        create_job_ex ( 1: string session_id, 2: string job_id, 3: create_packer_job_detail create_job ) throws (1:invalid_operation ouch);
    packer_job_detail                        create_job ( 1: string session_id, 2: create_packer_job_detail create_job ) throws (1:invalid_operation ouch);
    packer_job_detail                        get_job( 1: string session_id, 2: string job_id, 3: string previous_updated_time  ) throws (1:invalid_operation ouch);
    bool                                     interrupt_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     resume_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                     remove_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    list<packer_job_detail>                  list_jobs( 1: string session_id ) throws (1:invalid_operation ouch);
    void                                     terminate( 1: string session_id );
    bool                                     running_job( 1: string session_id, 2: string job_id) throws (1:invalid_operation ouch);
}

enum connection_type {
  LOCAL_FOLDER          = 0,
  NFS_FOLDER            = 1,
  CIFS_FOLDER           = 1,
  WEBDAV                = 1,
  S3_BUCKET             = 2,
  WEBDAV_WITH_SSL       = 3,
  WEBDAV_EX             = 4,
  S3_BUCKET_EX          = 5,
  LOCAL_FOLDER_EX       = 6
}

enum aws_region{
    US_EAST_1      = 0,
    US_WEST_1      = 1,
    US_WEST_2      = 2,
    EU_WEST_1      = 3,
    EU_CENTRAL_1   = 4,
    AP_SOUTHEAST_1 = 5,
    AP_SOUTHEAST_2 = 6,
    AP_NORTHEAST_1 = 7,
    AP_NORTHEAST_2 = 8,
    SA_EAST_1      = 9
}

struct local_folder{
    1: optional string                           path     = ""
}

struct network_folder{
    1: optional string                          path                 = ""
    2: optional string                          username            = ""
    3: optional string                          password             = ""
    4: optional i32                             port                 = 0
    5: optional string                          proxy_host         = ""
    6: optional i32                             proxy_port         = 0
    7: optional string                          proxy_username    = ""
    8: optional string                          proxy_password     = ""
    9: optional aws_region                      s3_region            = 0
   10: optional i32                             timeout           = 300
}

struct _detail{
    1: local_folder    local
    2: network_folder  remote
}

struct connection{
    1: optional connection_type               type             = 0
    2: optional string                        id               = ""
    3: optional map<string,string>            options
    4: optional bool                          compressed       = true
    5: optional bool                          checksum         = false
    6: optional bool                          encrypted        = false
    7: optional _detail                       detail
}

struct image_map_info
{
    1: optional string                        image            = ""
    2: optional string                        base_image       = ""
    3: optional set<string>                   connection_ids
}

enum create_image_option
{
    VERSION_1      = 0,
    VERSION_2      = 1
}

struct create_image_info
{
    1: optional string                        name            = ""
    2: optional string                        base            = ""
    3: optional string                        parent          = ""
    4: optional set<string>                   connection_ids
    5: optional i64                           size            = 0
    6: optional i32                           block_size      = 0
    7: optional bool                          checksum_verify = true
    8: optional string                        comment         = ""
    9: optional create_image_option           version         = 0           // VERSION_2 will ignore the compressed and checksum optionals, control by connection settings.
   10: optional bool                          compressed      = true
   11: optional bool                          checksum        = false
   12: optional bool                          cdr             = false
   13: optional i8                            mode            = 2
}

service common_connection_service extends common_service
{
    bool test_connection   ( 1: string session_id , 2: connection conn) throws (1:invalid_operation ouch);
    bool add_connection    ( 1: string session_id , 2: connection conn) throws (1:invalid_operation ouch);
    bool remove_connection ( 1: string session_id , 2: string connection_id) throws (1:invalid_operation ouch);
    bool modify_connection ( 1: string session_id , 2: connection conn) throws (1:invalid_operation ouch);
    list<connection> enumerate_connections( 1: string session_id ) throws (1:invalid_operation ouch);
    connection get_connection( 1: string session_id , 2: string connection_id ) throws (1:invalid_operation ouch);
    i64 get_available_bytes   ( 1: string session_id , 2: string connection_id) throws (1:invalid_operation ouch);
}

service carrier_service extends common_connection_service
{
    string create( 1: string session_id, 2: create_image_info image ) throws (1:invalid_operation ouch);
    string create_ex( 1: string session_id, 2: set<string> connection_ids, 3: string base_name, 4: string name, 5: i64 size, 6: i32 block_size, 7: string parent, 8: bool checksum_verify ) throws (1:invalid_operation ouch);
    string open( 1: string session_id, 2: set<string> connection_ids, 3: string base_name, 4: string name ) throws (1:invalid_operation ouch);
    binary read( 1: string session_id, 2: string image_id, 3: i64 start, 4: i32 number_of_bytes_to_read ) throws (1:invalid_operation ouch);
    i32    write( 1: string session_id, 2: string image_id, 3: i64 start, 4: binary buffer, 5: i32 number_of_bytes_to_write ) throws (1:invalid_operation ouch);
    i32    write_ex( 1: string session_id, 2: string image_id, 3: i64 start, 4: binary buffer, 5: i32 number_of_bytes_to_write, 6: bool is_compressed ) throws (1:invalid_operation ouch);
    bool   close( 1: string session_id, 2: string image_id, 3: bool is_cancel ) throws (1:invalid_operation ouch);
    bool   remove_base_image(1: string session_id, 2: set<string> base_images ) throws (1:invalid_operation ouch);
    // base image name = disk uuid
    bool   remove_snapshot_image(1: string session_id, 2: map<string,image_map_info> images ) throws (1:invalid_operation ouch);
    bool   verify_management ( 1: string management, 2: i32 port, 3: bool is_ssl );
    bool   set_buffer_size(1: string session_id, 2: i32 size ); // Size in GB
    bool   is_buffer_free( 1: string session_id, 2: string image_id ) throws (1:invalid_operation ouch);
    bool   is_image_replicated( 1: string session_id, 2: set<string> connection_ids, 3: string image_name) throws (1:invalid_operation ouch);
}

struct loader_job_detail
{
    1: optional string                          replica_id                  = ""
    2: optional string                          id                          = ""
    3: optional job_state                       state                       = 1
    4: optional string                          created_time                = ""
    5: optional string                          updated_time                = ""
    6: optional map<string,i64>                 progress
    7: optional list<job_history>               histories
    8: optional string                          connection_id
    9: optional map<string,i64>                 data
   10: optional string                          snapshot_id                 = ""
   11: optional map<string,i64>                 duplicated_data
   12: optional map<string,i64>                 transport_data
}

service loader_service extends common_connection_service
{
    loader_job_detail                             create_job_ex ( 1: string session_id, 2: string job_id, 3: create_job_detail create_job ) throws (1:invalid_operation ouch);
    loader_job_detail                             create_job ( 1: string session_id, 2: create_job_detail create_job ) throws (1:invalid_operation ouch);
    loader_job_detail                             get_job( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                          interrupt_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                          resume_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                          remove_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    list<loader_job_detail>                       list_jobs( 1: string session_id ) throws (1:invalid_operation ouch);
    bool                                          update_job( 1: string session_id, 2: string job_id, 3: create_job_detail job ) throws (1:invalid_operation ouch);
    void                                          terminate( 1: string session_id );
    bool                                          remove_snapshot_image(1: string session_id, 2: map<string,image_map_info> images ) throws (1:invalid_operation ouch);
    bool                                          running_job( 1: string session_id, 2: string job_id) throws (1:invalid_operation ouch);
    bool                                          verify_management ( 1: string management, 2: i32 port, 3: bool is_ssl );
    bool                                          set_customized_id ( 1: string session_id, 2: string disk_addr, 3: string disk_id ) throws (1:invalid_operation ouch);

    string                                        create_vhd_disk_from_snapshot(1: string connection_string, 2: string container, 3: string original_disk_name, 4: string target_disk_name, 5: string snapshot ) throws (1:invalid_operation ouch);
    bool                                          is_snapshot_vhd_disk_ready( 1: string task_id )  throws (1:invalid_operation ouch);
    bool                                          delete_vhd_disk(1: string connection_string, 2: string container, 3: string disk_name) throws (1:invalid_operation ouch);
    bool                                          delete_vhd_disk_snapshot(1: string connection_string, 2: string container, 3: string disk_name, 4: string snapshot) throws (1:invalid_operation ouch);
    list<vhd_snapshot>                            get_vhd_disk_snapshots(1: string connection_string, 2: string container, 3: string disk_name) throws (1:invalid_operation ouch);
    bool                                          verify_connection_string(1: string connection_string) throws (1:invalid_operation ouch);
}

struct upload_progress
{
   1: optional i64                               size                        = 0 //GB
   2: optional i64                               progress                    = 0
   3: optional i64                               vhd_size                    = 0
   4: optional string                            upload_id                   = ""
   5: optional bool                              completed                   = false
}

struct launcher_job_detail
{
    1: optional string                          replica_id                  = ""
    2: optional string                          id                          = ""
    3: optional job_state                       state                       = 1
    4: optional string                          created_time                = ""
    5: optional string                          updated_time                = ""
    6: optional string                          boot_disk                   = ""
    7: optional list<job_history>               histories
    8: optional bool                            is_error                    = false
    9: optional bool                            is_windows_update           = false
   10: optional string                          platform                    = ""
   11: optional string                          architecture                = ""
   12: optional i64                             size                        = 0 //GB
   13: optional i64                             progress                    = 0
   14: optional i64                             vhd_size                    = 0
   15: optional string                          upload_id                   = ""
   16: optional string                          host_name                   = ""
   17: optional map<string,upload_progress>     vhd_upload_progress
   18: optional string                          virtual_machine_id          = ""
}

union job_detail{
    1: replica_job_detail  scheduler
    2: launcher_job_detail launcher
    3: loader_job_detail   loader
}

service physical_packer_service_proxy extends common_service{
    service_info                             packer_ping_p( 1: string session_id , 2: string addr);
    list<snapshot>                           take_snapshots_p      ( 1: string session_id , 2: string addr, 3: set<string> disks ) throws (1:invalid_operation ouch);
    list<snapshot>                           take_snapshots_ex_p   ( 1: string session_id , 2: string addr, 3: set<string> disks, 4: string pre_script, 5: string post_script) throws (1:invalid_operation ouch);
    list<snapshot>                           take_snapshots2_p     ( 1: string session_id , 2: string addr, 3: take_snapshots_parameters parameters) throws (1:invalid_operation ouch);
    delete_snapshot_result                   delete_snapshot_p     ( 1: string session_id , 2: string addr, 3: string snapshot_id ) throws (1:invalid_operation ouch);
    delete_snapshot_result                   delete_snapshot_set_p ( 1: string session_id , 2: string addr, 3: string snapshot_set_id ) throws (1:invalid_operation ouch);
    map<string,list<snapshot>>               get_all_snapshots_p    ( 1: string session_id, 2: string addr ) throws (1:invalid_operation ouch);
    packer_job_detail                        create_packer_job_ex_p ( 1: string session_id, 2: string addr, 3: string job_id, 4: create_packer_job_detail create_job ) throws (1:invalid_operation ouch);
    packer_job_detail                        get_packer_job_p( 1: string session_id, 2: string addr, 3: string job_id, 4: string previous_updated_time ) throws (1:invalid_operation ouch);
    bool                                     interrupt_packer_job_p ( 1: string session_id, 2: string addr, 3: string job_id ) throws (1:invalid_operation ouch);
    bool                                     resume_packer_job_p ( 1: string session_id, 2: string addr, 3: string job_id ) throws (1:invalid_operation ouch);
    bool                                     remove_packer_job_p ( 1: string session_id, 2: string addr, 3: string job_id ) throws (1:invalid_operation ouch);
    bool                                     running_packer_job_p( 1: string session_id, 2: string addr, 3: string job_id) throws (1:invalid_operation ouch);
    set<disk_info>                           enumerate_packer_disks_p  ( 1: string session_id, 2: string addr, 3: enumerate_disk_filter_style filter)  throws (1:invalid_operation ouch);
    bool                                     verify_packer_carrier_p ( 1: string session_id, 2: string addr, 3: string carrier, 4: bool is_ssl ) throws (1:invalid_operation ouch);
    physical_machine_info                    get_packer_host_detail_p( 1: string session_id, 2: string addr, 3: machine_detail_filter filter ) throws (1:invalid_operation ouch);
}

service service_proxy extends physical_packer_service_proxy{
    job_detail                     create_job_ex_p ( 1: string session_id, 2: string job_id, 3: create_job_detail create_job, 4: string service_type ) throws (1:invalid_operation ouch);
    job_detail                     get_job_p( 1: string session_id, 2: string job_id, 3: string service_type ) throws (1:invalid_operation ouch);
    bool                           interrupt_job_p ( 1: string session_id, 2: string job_id, 3: string service_type ) throws (1:invalid_operation ouch);
    bool                           resume_job_p ( 1: string session_id, 2: string job_id, 3: string service_type ) throws (1:invalid_operation ouch);
    bool                           remove_job_p ( 1: string session_id, 2: string job_id , 3: string service_type) throws (1:invalid_operation ouch);
    bool                           running_job_p( 1: string session_id, 2: string job_id, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           update_job_p( 1: string session_id, 2: string job_id, 3: create_job_detail create_job, 4: string service_type ) throws (1:invalid_operation ouch);
    bool                           remove_snapshot_image_p(1: string session_id, 2: map<string,image_map_info> images , 3: string service_type) throws (1:invalid_operation ouch);
    bool                           test_connection_p   ( 1: string session_id , 2: connection conn, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           add_connection_p    ( 1: string session_id , 2: connection conn, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           remove_connection_p ( 1: string session_id , 2: string connection_id, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           modify_connection_p ( 1: string session_id , 2: connection conn, 3: string service_type) throws (1:invalid_operation ouch);
    list<connection>               enumerate_connections_p( 1: string session_id, 2: string service_type ) throws (1:invalid_operation ouch);
    connection                     get_connection_p( 1: string session_id , 2: string connection_id, 3: string service_type ) throws (1:invalid_operation ouch);

    virtual_host                   get_virtual_host_info_p( 1: string session_id, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    list<virtual_host>             get_virtual_hosts_p( 1: string session_id, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    virtual_machine                get_virtual_machine_detail_p ( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                           power_off_virtual_machine_p( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                           remove_virtual_machine_p( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    list<vmware_snapshot>          get_virtual_machine_snapshots_p( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                           remove_virtual_machine_snapshot_p( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string machine_id, 6: string snapshot_id ) throws (1:invalid_operation ouch);
    physical_machine_info          get_physical_machine_detail_p( 1: string session_id, 2: string host, 3: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    list<string>                   get_datacenter_folder_list_p( 1: string session_id, 2: string host, 3: string username, 4: string password, 5: string datacenter ) throws (1:invalid_operation ouch);
    bool                           verify_packer_to_carrier_p ( 1: string packer, 2: string carrier, 3: i32 port, 4: bool is_ssl);
    binary                         take_packer_xray_p( 1: string session_id, 2: string host ) throws (1:invalid_operation ouch);
    service_info                   get_packer_service_info_p( 1: string session_id, 2: string host ) throws (1:invalid_operation ouch);
    bool                           set_customized_id_p ( 1: string session_id, 2: string disk_addr, 3: string disk_id ) throws (1:invalid_operation ouch);

}

service launcher_service extends service_proxy
{
    launcher_job_detail                     create_job_ex ( 1: string session_id, 2: string job_id, 3: create_job_detail create_job ) throws (1:invalid_operation ouch);
    launcher_job_detail                     create_job ( 1: string session_id, 2: create_job_detail create_job ) throws (1:invalid_operation ouch);
    launcher_job_detail                     get_job( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                    interrupt_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                    resume_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                    update_job( 1: string session_id, 2: string job_id, 3: create_job_detail job ) throws (1:invalid_operation ouch);
    bool                                    remove_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    list<launcher_job_detail>               list_jobs( 1: string session_id ) throws (1:invalid_operation ouch);
    void                                    terminate( 1: string session_id );
    bool                                    running_job( 1: string session_id, 2: string job_id) throws (1:invalid_operation ouch);
    bool                                    verify_management ( 1: string management, 2: i32 port, 3: bool is_ssl );
    bool                                    unregister(1: string session ) throws (1:invalid_operation ouch);
}

struct replica_job_create_detail{
    1: optional string                              host                        = ""
    2: optional set<string>                         addr
    3: optional string                              username                    = ""
    4: optional string                              password                    = ""
    5: optional job_type                            type
    6: optional string                              virtual_machine_id          = ""
    7: optional set<string>                         disks
    8: optional map<string,string>                  targets  // <connection_id, replica_id>
    9: optional map<string,set<string>>             carriers // <connection_id, carries>
   10: optional set<string>                         full_replicas
   11: optional map<string, string>                 disk_ids
   12: optional string                              cbt_info                    = ""
   13: optional string                              snapshot_info               = ""
   14: optional bool                                checksum_verify             = true
   15: optional bool                                always_retry                = false
   16: optional i32                                 timeout                     = 300
   17: optional bool                                is_encrypted                = false
   18: optional bool                                is_paused                   = false
   19: optional i32                                 worker_thread_number        = 0
   20: optional bool                                block_mode_enable           = false
   21: optional bool                                file_system_filter_enable   = true
   22: optional i32                                 min_transport_size          = 0        //KB 0 => default
   23: optional i32                                 full_min_transport_size     = 0        //KB 0 => default
   24: optional bool                                is_full_replica             = false
   25: optional i32                                 buffer_size                 = 0     //GB  0 => unlimited
   26: optional bool                                is_compressed               = true
   27: optional bool                                is_checksum                 = false
   28: optional string                              time                        = ""
   29: optional map<string,string>                  priority_carrier
   30: optional bool                                is_only_single_system_disk  = false
   31: optional bool                                is_continuous_data_replication = false
   32: optional string                              pre_snapshot_script
   33: optional string                              post_snapshot_script
   34: optional bool                                is_compressed_by_packer     = false
   35: optional set<string>                         excluded_paths
   36: optional set<string>                         previous_excluded_paths
}

enum disk_detect_type{
    SCSI_ADDRESS      = 0,
    LINUX_DEVICE_PATH = 0,
    SERIAL_NUMBER     = 1,
    EXPORT_IMAGE      = 2,
    UNIQUE_ID         = 3,
    CUSTOMIZED_ID     = 4,
    AZURE_BLOB        = 5,
    VMWARE_VADP       = 6
}

enum virtual_disk_type{
    VHD              = 0,
    VHDX             = 1
}

enum conversion_type{
     ANY_TO_ANY = 0,
     OPENSTACK  = 1,
     XEN        = 2,
     VMWARE     = 3,
     HYPERV     = 4,
     AUTO       = -1
}

enum recovery_type{
     TEST_RECOVERY       = 0,
     DISASTER_RECOVERY   = 1,
     MIGRATION_RECOVERY  = 2
}

struct vmware_connection_info{
    1: optional string                              host                        = ""
    2: optional string                              username                    = ""
    3: optional string                              password                    = ""
    4: optional string                              esx                         = ""
    5: optional string                              datastore                   = ""
    6: optional string                              folder_path                 = ""
}

struct vmware_options{
    1: optional vmware_connection_info               connection
    2: optional string                               virtual_machine_id                 = ""
    3: optional string                               virtual_machine_snapshot           = ""
    4: optional i32                                  number_of_cpus                     = 1
    5: optional i32                                  number_of_memory_in_mb             = 1024
    6: optional string                               vm_name
    7: optional list<string>                         network_connections
    8: optional list<string>                         network_adapters
    9: optional map<hv_controller_type,list<string>> scsi_adapters
   10: optional string                               guest_id
   11: optional hv_vm_firmware                       firmware                           = 0
   12: optional bool                                 install_vm_tools                   = true
   13: optional list<string>                         mac_addresses
}

struct aliyun_options{
   1: optional string                         access_key                               = ""
   2: optional string                         secret_key                               = ""
   3: optional string                         objectname                               = ""
   4: optional string                         bucketname                               = ""
   5: optional string                         region                                   = ""
   6: optional i32                            max_size                                 = 500 //GB
   7: optional bool                           file_system_filter_enable                = true
   8: optional i16                            number_of_upload_threads                 = 0 //AUTO
   9: optional map<string,string>             disks_object_name_mapping
}

struct tencent_options{
   1: optional string                         access_key                               = ""
   2: optional string                         secret_key                               = ""
   3: optional string                         objectname                               = ""
   4: optional string                         bucketname                               = ""
   5: optional string                         region                                   = ""
   6: optional i32                            max_size                                 = 500 //GB
   7: optional bool                           file_system_filter_enable                = true
   8: optional i16                            number_of_upload_threads                 = 0 //AUTO
   9: optional map<string,string>             disks_object_name_mapping
}

union extra_options{
    1: aliyun_options  aliyun
    2: tencent_options tencent
}

enum extra_options_type{
    UNKNOWN       = 0,
    ALIYUN        = 1,
    TENCENT       = 2
}

struct vhd_snapshot{
     1: optional string                           id                    = ""
     2: optional string                           datetime              = ""
     3: optional string                           name                  = ""
}

struct cascading{
    1: optional i32                               level                         = 0
    2: optional string                            machine_id                    = ""
    3: optional connection                        connection_info
    4: optional map<string,cascading>             branches
}

struct loader_job_create_detail{
    1: optional string                            replica_id                    = ""
    2: optional map<string,string>                disks_lun_mapping
    3: optional list<string>                      snapshots
    4: optional map<string,map<string,string>>    disks_snapshot_mapping
    5: optional string                            connection_id
    6: optional bool                              block_mode_enable              = false
    7: optional bool                              purge_data                     = true
    8: optional bool                              remap                          = false
    9: optional disk_detect_type                  detect_type                    = 0
   10: optional i32                               worker_thread_number           = 0
   11: optional string                            host_name                      = ""
   12: optional virtual_disk_type                 export_disk_type               = 0
   13: optional string                            export_path                    = ""
   14: optional map<string,i64>                   disks_size_mapping
   15: optional bool                              keep_alive                       = true
   16: optional string                            time                             = ""
   17: optional bool                              is_continuous_data_replication   = false
   18: optional string                            azure_storage_connection_string  = ""
   19: optional vmware_connection_info            vmware_connection
   20: optional bool                              thin_provisioned                 = true
   21: optional bool                              is_paused                        = false
   22: optional cascading                         cascadings
}

struct launcher_job_create_detail{
    1: optional string                        replica_id                              = ""
    2: optional map<string,string>            disks_lun_mapping
    3: optional bool                          is_sysvol_authoritative_restore         = false
    4: optional bool                          is_enable_debug                         = false
    5: optional bool                          is_disable_machine_password_change      = false
    6: optional bool                          is_force_normal_boot                    = false
    7: optional set<network_info>             network_infos
    8: optional string                        config
    9: optional bool                          gpt_to_mbr                              = true
   10: optional disk_detect_type              detect_type                             = 0
   11: optional bool                          skip_system_injection                   = false
   12: optional bool                          reboot_winpe                            = false
   13: optional set<string>                   callbacks
   14: optional i32                           callback_timeout                        = 30     //seconds
   15: optional string                        host_name                               = ""
   16: optional virtual_disk_type             export_disk_type                        = 0
   17: optional string                        export_path                             = ""
   18: optional conversion_type               target_type                             = -1
   19: optional hv_guest_os_type              os_type                                 = 1
   20: optional bool                          is_update_ex                            = false
   21: optional extra_options_type            options_type                            = 0
   22: optional extra_options                 options
   23: optional set<string>                   pre_scripts
   24: optional set<string>                   post_scripts
   25: optional vmware_options                vmware
   26: optional recovery_type                 mode                                    = 0
}

struct register_service_info{
    1: optional string                        mgmt_addr                          = ""
    2: optional string                        username                           = ""
    3: optional string                        password                           = ""
    4: optional set<string>                   service_types
    5: optional string                        version                            = ""
    6: optional string                        path                               = ""
}

struct register_physical_packer_info{
    1: optional string                        mgmt_addr                           = ""
    2: optional string                        username                            = ""
    3: optional string                        password                            = ""
    4: optional string                        packer_addr                         = ""
    5: optional string                        version                             = ""
    6: optional string                        path                                = ""
}

exception command_empty{
}

exception invalid_session{
}

struct transport_message{
    1: i64  id                = 0,
    2: binary message         = ""
}

struct register_return{
    1: optional string                        message                     = ""
    2: optional string                        session                     = ""
}

service management_service{

    replica_job_create_detail             get_replica_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    void                                  update_replica_job_state(  1: string session_id, 2: replica_job_detail state ) throws (1:invalid_operation ouch);
    bool                                  is_replica_job_alive ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);

    loader_job_create_detail              get_loader_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    void                                  update_loader_job_state(  1: string session_id, 2: loader_job_detail state ) throws (1:invalid_operation ouch);

    bool                                  take_snapshots( 1: string session_id, 2: string snapshot_id ) throws (1:invalid_operation ouch);
    bool                                  check_snapshots( 1: string session_id, 2: string snapshots_id ) throws (1:invalid_operation ouch);

    launcher_job_create_detail            get_launcher_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    void                                  update_launcher_job_state(  1: string session_id, 2: launcher_job_detail state ) throws (1:invalid_operation ouch);
    void                                  update_launcher_job_state_ex(  1: string session_id, 2: launcher_job_detail state ) throws (1:invalid_operation ouch);
    bool                                  is_launcher_job_image_ready( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);

    bool                                  is_loader_job_devices_ready( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                  mount_loader_job_devices( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                  dismount_loader_job_devices( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                  discard_snapshots( 1: string session_id, 2: string snapshots_id ) throws (1:invalid_operation ouch);

    register_return                       register_service( 1: string session_id, 2: register_service_info register_info, 3: physical_machine_info machine_info ) throws (1:invalid_operation ouch);
    register_return                       register_physical_packer( 1: string session_id, 2: register_physical_packer_info packer_info, 3: physical_machine_info machine_info ) throws (1:invalid_operation ouch);

    bool                                  check_running_task( 1: string task_id, 2: string parameters ) throws (1:invalid_operation ouch);

    service_info                          ping();
}

struct license_info{
    1: optional string                        key                          = ""
    2: optional string                        activated                    = ""
    3: optional i32                           count                        = 0
    4: optional string                        expired_date                 = ""
    5: optional i32                           consumed                     = 0
    6: optional bool                          is_active                    = false
    7: optional string                        name                         = ""
    8: optional string                        email                        = ""
    9: optional string                        status                       = ""
}

struct workload_history{
    1: optional string                         machine_id                   = ""
    2: optional string                         name                         = ""
    3: optional string                         type                         = ""
    4: optional list<i32>                      histories
}

struct license_infos{
    1: optional list<license_info>           licenses
    2: optional list<workload_history>       histories
}

struct running_task{
    1: optional string                  id
    2: optional list<job_trigger>       triggers
    3: optional string                  mgmt_addr
    4: optional i32                     mgmt_port                    = 80
    5: optional bool                    is_ssl                       = false
    6: optional string                  parameters
}

service reverse_transport
{
    service_info               ping();
    string                     generate_session( 1: string addr ) throws (1:invalid_operation ouch);
    transport_message          receive( 1: string session, 2: string addr, 3: string name ) throws (1:invalid_operation ouch, 2: command_empty empty, 3:invalid_session invalid );
    bool                       response( 1: string session, 2: string addr, 3: transport_message response ) throws (1:invalid_operation ouch, 2: command_empty empty, 3:invalid_session invalid );
}

service transport_service extends physical_packer_service_proxy{

    string                         generate_session( 1: string addr ) throws (1:invalid_operation ouch);

    //license
    string                         get_package_info( 1: string email, 2: string name, 3: string key )throws (1:invalid_operation ouch);
    bool                           active_license( 1: string email, 2: string name, 3: string key )throws (1:invalid_operation ouch);
    bool                           add_license( 1: string license )throws (1:invalid_operation ouch);
    bool                           add_license_with_key( 1: string key, 2: string license )throws (1:invalid_operation ouch);
    license_infos                  get_licenses();
    bool                           check_license_expiration(1: i8 days );
    bool                           is_license_valid( 1: string job_id );
    bool                           is_license_valid_ex( 1: string job_id, 2: bool is_recovery );
    bool                           remove_license( 1: string key );
    string                         query_package_info( 1: string key ) throws (1:invalid_operation ouch);
    bool                           create_task( 1: running_task task ) throws (1:invalid_operation ouch);
    bool                           remove_task( 1: string task_id ) throws (1:invalid_operation ouch);

    //common_service
    service_info                   ping_p(1: string addr);
    physical_machine_info          get_host_detail_p  ( 1: string addr, 2: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    set<service_info>              get_service_list_p ( 1: string addr ) throws (1:invalid_operation ouch);
    set<disk_info>                 enumerate_disks_p  ( 1: string addr, 2: enumerate_disk_filter_style filter)  throws (1:invalid_operation ouch);
    bool                           verify_carrier_p ( 1: string addr, 2: string carrier, 3: bool is_ssl ) throws (1:invalid_operation ouch);
    binary                         take_xray_p( 1: string addr )    throws (1:invalid_operation ouch);
    binary                         take_xrays_p( 1: string addr )    throws (1:invalid_operation ouch);
    bool                           create_mutex_p( 1: string addr, 2: string session, 3: i16 timeout ) throws (1:invalid_operation ouch);
    bool                           delete_mutex_p( 1: string addr, 2: string session ) throws (1:invalid_operation ouch);

    //service_proxy
    job_detail                     create_job_ex_p ( 1: string addr, 2: string job_id, 3: create_job_detail create_job, 4: string service_type ) throws (1:invalid_operation ouch);
    job_detail                     get_job_p( 1: string addr, 2: string job_id, 3: string service_type ) throws (1:invalid_operation ouch);
    bool                           interrupt_job_p ( 1: string addr, 2: string job_id, 3: string service_type ) throws (1:invalid_operation ouch);
    bool                           resume_job_p ( 1: string addr, 2: string job_id, 3: string service_type ) throws (1:invalid_operation ouch);
    bool                           remove_job_p ( 1: string addr, 2: string job_id , 3: string service_type) throws (1:invalid_operation ouch);
    bool                           running_job_p( 1: string addr, 2: string job_id, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           update_job_p( 1: string addr, 2: string job_id, 3: create_job_detail create_job, 4: string service_type ) throws (1:invalid_operation ouch);
    bool                           remove_snapshot_image_p(1: string addr, 2: map<string,image_map_info> images , 3: string service_type) throws (1:invalid_operation ouch);
    bool                           test_connection_p   ( 1: string addr , 2: connection conn, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           add_connection_p    ( 1: string addr , 2: connection conn, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           remove_connection_p ( 1: string addr , 2: string connection_id, 3: string service_type) throws (1:invalid_operation ouch);
    bool                           modify_connection_p ( 1: string addr , 2: connection conn, 3: string service_type) throws (1:invalid_operation ouch);
    list<connection>               enumerate_connections_p( 1: string addr, 2: string service_type ) throws (1:invalid_operation ouch);
    connection                     get_connection_p( 1: string addr , 2: string connection_id, 3: string service_type ) throws (1:invalid_operation ouch);

    virtual_host                   get_virtual_host_info_p( 1: string addr, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    list<virtual_host>             get_virtual_hosts_p( 1: string addr, 2: string host, 3: string username, 4: string password ) throws (1:invalid_operation ouch);
    virtual_machine                get_virtual_machine_detail_p ( 1: string addr, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    physical_machine_info          get_physical_machine_detail_p( 1: string addr, 2: string host, 3: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    bool                           power_off_virtual_machine_p( 1: string addr, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                           remove_virtual_machine_p( 1: string addr, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    list<vmware_snapshot>          get_virtual_machine_snapshots_p( 1: string addr, 2: string host, 3: string username, 4: string password, 5: string machine_id ) throws (1:invalid_operation ouch);
    bool                           remove_virtual_machine_snapshot_p( 1: string addr, 2: string host, 3: string username, 4: string password, 5: string machine_id, 6: string snapshot_id ) throws (1:invalid_operation ouch);
    list<string>                   get_datacenter_folder_list_p( 1: string addr, 2: string host, 3: string username, 4: string password, 5: string datacenter ) throws (1:invalid_operation ouch);
    binary                         take_packer_xray_p( 1: string addr, 2: string host ) throws (1:invalid_operation ouch);
    service_info                   get_packer_service_info_p( 1: string addr, 2: string host ) throws (1:invalid_operation ouch);

    bool                           verify_management_p ( 1: string addr, 2: string management, 3: i32 port, 4: bool is_ssl ) throws (1:invalid_operation ouch);
    bool                           verify_packer_to_carrier_p ( 1: string addr, 2: string packer, 3: string carrier, 4: i32 port, 5: bool is_ssl) throws (1:invalid_operation ouch);

    replica_job_create_detail      get_replica_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    loader_job_create_detail       get_loader_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    launcher_job_create_detail     get_launcher_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    void                           terminate( 1: string session_id );

    bool                           set_customized_id_p ( 1: string addr, 2: string disk_addr, 3: string disk_id ) throws (1:invalid_operation ouch);

    bool                           unregister_packer_p(1: string addr ) throws (1:invalid_operation ouch);
    bool                           unregister_server_p(1: string addr ) throws (1:invalid_operation ouch);

    //
    string                         create_vhd_disk_from_snapshot(1: string connection_string, 2: string container, 3: string original_disk_name, 4: string target_disk_name, 5: string snapshot ) throws (1:invalid_operation ouch);
    bool                           is_snapshot_vhd_disk_ready( 1: string task_id )  throws (1:invalid_operation ouch);
    bool                           delete_vhd_disk(1: string connection_string, 2: string container, 3: string disk_name) throws (1:invalid_operation ouch);
    bool                           delete_vhd_disk_snapshot(1: string connection_string, 2: string container, 3: string disk_name, 4: string snapshot) throws (1:invalid_operation ouch);
    list<vhd_snapshot>             get_vhd_disk_snapshots(1: string connection_string, 2: string container, 3: string disk_name) throws (1:invalid_operation ouch);
    bool                           verify_connection_string(1: string connection_string) throws (1:invalid_operation ouch);
}
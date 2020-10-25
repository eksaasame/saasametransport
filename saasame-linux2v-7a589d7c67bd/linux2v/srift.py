from io import StringIO


def load_content():
    return StringIO(r"""//_VERSION  "1.0.233"

namespace cpp saasame.transport
namespace csharp saasame.transport
namespace java saasame.transport
namespace php saasame.transport

const string CONFIG_PATH                = "Software\\SaaSame\\Transport"

const string LINUX_LAUNCHER_SERVICE     = "{6FC9C4B6-B61E-4745-A6AA-1D5F0A2DA08B}"

const string LAUNCHER_SERVICE_DISPLAY_NAME = "SaaSame Launcher Service"
const string LAUNCHER_SERVICE_DESCRIPTION  = "SaaSame Launcher Service"


const i32   LAUNCHER_SERVICE_PORT           = 18893
const i32   MANAGEMENT_SERVICE_PORT         = 443
const string MANAGEMENT_SERVICE_PATH        = "/mgmt/default.php"

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
   33: optional string				unique_id			   = ""
   34: optional i16                 unique_id_format       = 0
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
  28: optional string			   current_vcbt_version
  29: optional string              installed_vcbt_version
  30: optional bool                is_winpe                 = false
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
  1: i32 							what_op,
  2: string 						why,
  5: optional string             	format                    = ""
  6: optional list<string>	        arguments
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

// Guest OS
enum hv_guest_os_type
{
     HV_OS_UNKNOWN = 0,
     HV_OS_WINDOWS = 1,
     HV_OS_LINUX = 2
}

struct job_history{
    1: optional string          	time                      = ""
    2: optional job_state       	state                     = 1
    3: optional i32             	error                     = 0
    4: optional string          	description               = ""
	7: optional string             	format                    = ""
	8: optional list<string>	    arguments
}

struct create_job_detail{
    1: optional job_type                type                        = 1
    2: optional list<job_trigger>       triggers
    3: optional string                  management_id               = ""
    4: optional set<string>             mgmt_addr
	5: optional i32 					mgmt_port					= 80
	6: optional bool 					is_ssl					    = false
}

struct service_info{
    1: string id
    2: string version
	3: string path
}

service common_service
{
    service_info                   ping();
    physical_machine_info          get_host_detail  ( 1: string session_id, 2: machine_detail_filter filter ) throws (1:invalid_operation ouch);
    set<service_info>              get_service_list ( 1: string session_id ) throws (1:invalid_operation ouch);
    set<disk_info>                 enumerate_disks  ( 1: enumerate_disk_filter_style filter)  throws (1:invalid_operation ouch);
	bool   						   verify_carrier ( 1: string carrier, 2: bool is_ssl );
	binary  					   take_xray()	throws (1:invalid_operation ouch);
	binary						   take_xrays()	throws (1:invalid_operation ouch);
    bool						   create_mutex( 1: string session, 2: i16 timeout ) throws (1:invalid_operation ouch);
	bool						   delete_mutex( 1: string session ) throws (1:invalid_operation ouch);
}


struct launcher_job_detail{
    1: optional string                          replica_id                  = ""
    2: optional string                          id                          = ""
    3: optional job_state                       state                       = 1
    4: optional string                          created_time                = ""
    5: optional string                          updated_time                = ""
    6: optional string                          boot_disk                   = ""
    7: optional list<job_history>               histories
	8: optional bool                            is_error                    = false
    9: optional bool                            is_windows_update           = false
}

service launcher_service extends common_service
{
    launcher_job_detail                     create_job_ex ( 1: string session_id, 2: string job_id, 3: create_job_detail create_job ) throws (1:invalid_operation ouch);
    launcher_job_detail                     create_job ( 1: string session_id, 2: create_job_detail create_job ) throws (1:invalid_operation ouch);
    launcher_job_detail                     get_job( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                    interrupt_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                    resume_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    bool                                    remove_job ( 1: string session_id, 2: string job_id ) throws (1:invalid_operation ouch);
    list<launcher_job_detail>               list_jobs( 1: string session_id ) throws (1:invalid_operation ouch);
    void                                    terminate( 1: string session_id );
    bool                                    running_job( 1: string session_id, 2: string job_id) throws (1:invalid_operation ouch);
	bool						            verify_management ( 1: string management, 2: i32 port, 3: bool is_ssl );
}


enum disk_detect_type{
    SCSI_ADDRESS      = 0,
	LINUX_DEVICE_PATH = 0,
    SERIAL_NUMBER     = 1
}

struct launcher_job_create_detail{
    1: optional string                        replica_id                             = ""
    2: optional map<string,string>            disks_lun_mapping
    3: optional bool                          is_sysvol_authoritative_restore        = false
    4: optional bool                          is_enable_debug                        = false
    5: optional bool                          is_disable_machine_password_change     = false
    6: optional bool                          is_force_normal_boot                   = false
    7: optional set<network_info>             network_infos
    8: optional string                        config
    9: optional bool                          gpt_to_mbr                             = true
   10: optional disk_detect_type              detect_type                            = 0
   11: optional bool                          skip_system_injection                  = false
   12: optional bool                          reboot_winpe                           = false
   13: optional set<string>				      callbacks
   14: optional i32						      callback_timeout						 = 30     //seconds
}

service management_service{
    launcher_job_create_detail            get_launcher_job_create_detail ( 1: string session_id, 2: string job_id  ) throws (1:invalid_operation ouch);
    bool                                  check_snapshots( 1: string session_id, 2: string snapshots_id ) throws (1:invalid_operation ouch);
    void                                  update_launcher_job_state(  1: string session_id, 2: launcher_job_detail state ) throws (1:invalid_operation ouch);
}
""")  # noqa

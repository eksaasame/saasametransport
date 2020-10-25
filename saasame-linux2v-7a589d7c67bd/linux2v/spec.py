import uuid
import platform

from pathlib import Path
import psutil
import thriftpy
# from ..lib import log

GUID_LENGTH_STRING = "00000000-0000-0000-0000-000000000000"


try:
    from . import srift
except ImportError:
    HERE = Path(__file__).parent
    thrift = thriftpy.load(str(HERE / "saasame.thrift"),
                           module_name="s_thrift")
else:
    with srift.load_content() as fp:
        thrift = thriftpy.load_fp(fp, module_name="s_thrift")

thrift.TIME_FORMAT = '%Y-%b-%d %H:%M:%S.%f'


def _cook_specs_set():
    specs_set = set()
    attrs = (attr for attr in dir(thrift) if not attr.startswith("_"))
    for attr_name in attrs:
        attr = getattr(thrift, attr_name)
        if attr and attr not in (None, 11):
            specs_set.add(attr)
    return specs_set
thrift.specs_set = _cook_specs_set()


def diff(o1, o2, type_only=False):
    print(">>> DIFF <<<")
    for key in o1._tspec.keys():
        attr1 = getattr(o1, key)
        attr2 = getattr(o2, key)
        if type_only:
            if type(attr1) != type(attr2):
                print("{}: {!r} != {!r}".format(key, attr1, attr2))
        else:
            if attr1 != attr2:
                print("{}: {!r} != {!r}".format(key, attr1, attr2))
            else:
                print("{} is the same.".format(key))
thrift.diff = diff


# [[ Enums ]]


class OsType(thrift.hv_guest_os_type):
    UNKNOWN = thrift.hv_guest_os_type.HV_OS_UNKNOWN
    WINDOWS = thrift.hv_guest_os_type.HV_OS_WINDOWS
    LINUX = thrift.hv_guest_os_type.HV_OS_LINUX


class JobState(thrift.job_state):
    none = thrift.job_state.job_state_none
    initialized = thrift.job_state.job_state_initialed
    replicating = thrift.job_state.job_state_replicating
    replicated = thrift.job_state.job_state_replicated
    converting = thrift.job_state.job_state_converting
    finished = thrift.job_state.job_state_finished
    sche_completed = thrift.job_state.job_state_sche_completed
    recovered = thrift.job_state.job_state_recover
    discarded = thrift.job_state.job_state_discard
    # error = thrift.job_state.job_state_error
    # By gen-cpp/job.h:17: job_state_error = 0x80000000.


class JobType(thrift.job_type):
    physical_packer = thrift.job_type.physical_packer_job_type
    virtual_packer = thrift.job_type.virtual_packer_job_type
    physical_transport = thrift.job_type.physical_transport_type
    virtual_transport = thrift.job_type.virtual_transport_type
    loader = thrift.job_type.loader_job_type
    launcher = thrift.job_type.launcher_job_type


class JobTriggerType(thrift.job_trigger_type):
    runonce = thrift.job_trigger_type.runonce_trigger
    interval = thrift.job_trigger_type.interval_trigger


class PartitionStyle(thrift.partition_style):
    UNKNOWN = thrift.partition_style.PARTITION_UNKNOWN
    MBR = thrift.partition_style.PARTITION_MBR
    GPT = thrift.partition_style.PARTITION_GPT


class EnumerateDiskFilter(thrift.enumerate_disk_filter_style):
    ALL = thrift.enumerate_disk_filter_style.ALL_DISK
    UNINITIALIZED = thrift.enumerate_disk_filter_style.UNINITIALIZED_DISK


def obj_to_dict(enum_obj, child_method_name=None, child_method_kwargs={}):
    d = {}
    for key in enum_obj._tspec.keys():
        attr = getattr(enum_obj, key)
        attr_type = type(attr)
        if attr_type in thrift.specs_set:  # recursive dump thrift objs tree
            attr = obj_to_dict(attr)
        elif attr_type == uuid.UUID:
            attr = str(attr)
        else:
            if attr_type in (tuple, list, set):
                new_attr = []
                for a in attr:
                    child_method = (child_method_name and
                                    getattr(a, child_method_name, None))
                    if child_method:
                        new_attr.append(child_method(**child_method_kwargs))
                    else:
                        new_attr.append(a)
                attr = new_attr
            else:
                child_method = (child_method_name and
                                getattr(attr, child_method_name, None))
                if child_method:
                    attr = child_method(**child_method_kwargs)
        d[key] = attr
    return d

thrift.obj_to_dict = obj_to_dict


def enum_to_dict(enum_obj):
    return enum_obj._NAMES_TO_VALUES

thrift.enum_to_dict = enum_to_dict


class OsVersionInfo(thrift.os_version_info):
    def __init__(self, version):
        super().__init__()
        if version:
            major_version, minor_version, patch_version = \
                map(lambda x: int(x) if x else -1, version.split("."))
            self.major_version = major_version
            self.minor_version = minor_version
            self.build_number = patch_version


class PhysicalMachineInfo(thrift.physical_machine_info):
    """
    Current only working for Linux and Mac.
    """
    def __init__(self):
        super().__init__()
        # Unsupported fields are discarded
        self.architecture = platform.machine()
        self.client_id = ""
        self.client_name = ""
        self.domain = ""
        self.processors = psutil.cpu_count(logical=False)
        self.logical_processors = psutil.cpu_count(logical=True)
        self.machine_id = ""
        self.manufacturer = ""
        self.os_name = ""
        self.os_type = OsType.UNKNOWN
        self.os_version = OsVersionInfo("")
        self.os_system_info = platform.platform()
        self.physical_memory = psutil.virtual_memory().total
        self.cluster_infos = []
        self.disk_infos = []
        self.network_infos = []
        self.partition_infos = []
        self.volume_infos = []


class NetworkInfo(thrift.network_info):
    def __init__(self):
        super().__init__()
        self.adapter_name = ""
        self.description = ""
        self.dnss = []
        self.gateways = []
        self.ip_addresses = []
        self.is_dhcp_v4 = False
        self.is_dhcp_v6 = False
        self.mac_address = ""
        self.subnet_masks = []


class PartitionInfo(thrift.partition_info):
    def __init__(self):
        super().__init__()
        self.access_paths = []
        self.disk_number = -1
        self.drive_letter = ""
        self.gpt_type = ""
        self.guid = ""
        self.is_active = False
        self.is_boot = False
        self.is_hidden = False
        self.is_offline = False
        self.is_readonly = False
        self.is_shadowcopy = False
        self.is_system = False
        self.mbr_type = 0
        self.offset = 0
        self.partition_number = -1
        self.size = 0


class VolumnInfo(thrift.volume_info):
    def __init__(self):
        super().__init__()
        self.access_paths = []
        self.cluster_access_path = ""
        self.drive_letter = ""
        self.drive_type = 0
        self.file_system = ""
        self.file_system_catalogid = ""
        self.file_system_label = ""
        self.object_id = ""
        self.path = ""
        self.size = 0
        self.size_remaining = 0

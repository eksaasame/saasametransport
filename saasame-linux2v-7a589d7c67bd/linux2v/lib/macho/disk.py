import time
from pathlib import Path
from contextlib import contextmanager

import psutil

from ...spec import (
    thrift,
    PartitionStyle
)
from .. import log
from .. import cli

def get_dev_mount_point(dev, dry=False):
    cmd = ('lsblk -P -p -o NAME,MOUNTPOINT {}'.format(dev))
    output = cli.run(cmd, dry=dry)
    for line in output.split("\n"):
        dev_name = None
        for keyvar in line.split(" "):
            name, var = keyvar.partition("=")[::2]
            if name=="NAME":
                dev_name=var.replace("\"","")
            if dev_name == dev:
                if name=="MOUNTPOINT":
                    mount_point = var.replace("\"","")
                    return mount_point
    return ""
         
def get_partition_style(devpath):
    if not cli.os_disk_operable:
        from ...fixtures import mac
        return mac.get_partition_style()
    cli.run("parted {} print Fix Fix".format(devpath), quiet_log=True)
    output = cli.run("parted {} print".format(devpath), quiet_log=True)
    is_msdos_in_output = bool("Partition Table: msdos" in output)
    return PartitionStyle.MBR if is_msdos_in_output else PartitionStyle.GPT

def mount(device: str, filesys: Path, waitting_for_finished=True, timeout=100,
          dry=False):
    cmd = "mount {} {}".format(device, filesys)
    res = cli.run(cmd, dry=dry)
    if waitting_for_finished and not dry:
        waitting_time = 0
        interval = 0.5
        while waitting_time < timeout:
            time.sleep(interval)
            if list(filesys.iterdir()):
                break
            if len(get_dev_mount_point(device, dry)):
                break
            waitting_time += interval
        else:
            log.error("Timeout at {} seconds".format(waitting_time))

    return res

def umount(device: str, filesys: Path, waitting_for_finished=True, timeout=100, quiet=False,
           dry=False):
    cmd = "umount {}".format(filesys)
    try:
        res = cli.run(cmd, dry=dry)
    except cli.run.exception:
        if quiet:
            return
        else:
            raise

    if waitting_for_finished and not dry:
        waitting_time = 0
        interval = 0.5
        while waitting_time < timeout:
            time.sleep(interval)
            if not list(filesys.iterdir()):
                break
            if "" == get_dev_mount_point(device, dry):
                break
            waitting_time += interval
        else:
            log.error("Timeout at {} seconds".format(waitting_time))

    return res

@contextmanager
def MountContext(device, filesys, dry=False):
    try:
        res = mount(device, filesys, dry=dry)
        yield res
        umount(device, filesys, dry=dry)
    except Exception:
        log.error("Error inside mounted {}".format(device))
        umount(device, filesys, quiet=True, dry=dry)
        time.sleep(1) if not dry else None
        raise

class Disk:
    scsi_address_attrs = (
        "scsi_port",
        "scsi_bus",
        "scsi_target_id",
        "scsi_logical_unita"
    )

    def __init__(self,
                 guid=None,
                 device=None,
                 partition=None,
                 scsi=None,
                 partition_style=None):
        self.guid = guid
        self.device = device
        # Not using 'if partition:' due to None could be given.
        self.partition = partition
        self.mountpoint = partition
        self.scsi = scsi
        self.partition_style = get_partition_style(device)

    def __str__(self):
        return "{} {} (guid:{}...{} scsi:[{}] mount:{})".format(
            self.__class__.__name__,
            self.device,
            self.guid[:4] if self.guid else None,
            self.guid[-4:] if self.guid else "",
            self.scsi['address'] if self.scsi else '----',
            self.mountpoint
        )

    def bus_type(self):
        return thrift.bus_type.SCSI  # XXX

    def has_boot_flag(self):
        return False

    def cylinders(self):  # for physical
        return 0  # XX

    def tracks_per_cylinder(self):  # for physical
        return 0  # XX

    def sectors_per_track(self):  # for physical
        return 0  # XX

    def size(self):
        if not self.mountpoint:  # Disk not mounted.
            return 0
        try:
            usage = psutil.disk_usage(self.mountpoint)
            return usage.total / 1024 / 1024 / 1024
        except Exception as exc:
            log.exception(exc) # Disk not ready.
            return 0

    def find_serial_number(self):  # XXX d->serial_number()
        """ TODO: parse one of them
        hdparm -I /dev/sda
        smartctl -i /dev/sda
        It could be None in VM Disk format (QEMU or VMWare disk)
        """
        return self.guid

    def count_partitions(self):
        return 1  # XXX

    def export_to_thrift(self):
        mountpoint = self.mountpoint
        info = thrift.disk_info()
        info.size = self.size()
        info.bus_type = self.bus_type()
        info.cylinders = self.cylinders()
        info.tracks_per_cylinder = self.tracks_per_cylinder()
        info.sectors_per_track = self.sectors_per_track()
        info.mountpoint = mountpoint
        info.boot_from_disk = False  # XXX
        info.friendly_name = str(self)
        info.guid = str(self.guid)
        info.is_boot = self.has_boot_flag()
        info.is_clustered = False
        info.is_offline = True
        info.is_readonly = False
        info.is_system = bool(mountpoint == "/")
        info.location = self.device
        info.model = "?"  # XXX d->model()
        info.number = 1  # XXX
        info.number_of_partitions = self.count_partitions()
        info.offline_reason = 0
        info.partition_style = self.partition_style
        info.path = self.device
        info.logical_sector_size = 512  # XXX fdisk -l
        info.physical_sector_size = 512  # XXX
        info.serial_number = self.find_serial_number()
        info.signature = 0  # XXX d->signature()

        if self.scsi:
            info.manufacturer = self.scsi['brand']
            info.model = self.scsi['description']
            for attr, value in zip(self.scsi_address_attrs,
                                   self.scsi['address']):
                setattr(info, attr, value)

        # Need to add code for following info
        info.cluster_owner = ''
        info.is_snapshot = False
        info.uri = ''
        return info

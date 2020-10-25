import alog

from linux2v.lib.macho import storage
from linux2v.fixtures import mac

from .base import BaseMachoTest

print = alog.debug


class TestStorage(BaseMachoTest):

    def test_blks_dict(self):
        blks = storage.blks_dict()
        print(alog.pformat(blks))

    def test_filter_lvm_devices(self):
        found_devices = list(storage.filter_lvm_devices(['/dev/vdh']))
        assert found_devices
        not_found_devices = list(storage.filter_lvm_devices(['/dev/vdj']))
        assert not_found_devices == []

    def test_cleanup_all_lvm_devices(self):
        storage.cleanup_all_lvm_devices()

    def test_find_disks(self):
        disks = list(storage.find_disks())
        for disk in disks:
            print(disk)

    def test_find_disk_partitions(self):
        disk_partitions = storage.find_disk_partitions()
        print(alog.pformat(disk_partitions))

    def test_find_disk_partitions_with_prefixes(self):
        device_prefix = '/dev/vdh'
        disk_partitions = storage.find_disk_partitions([device_prefix])
        print(alog.pformat(disk_partitions))
        assert len(disk_partitions.keys()) == 1
        assert disk_partitions[device_prefix] == [
            '/dev/mapper/vdh1',
            '/dev/mapper/centos-swap',
            '/dev/mapper/centos-root',
        ]

    def test_find_all_vgnames(self):
        vgname = 'centos-root'
        found_vgnames = storage.find_all_vgnames()
        assert(found_vgnames)
        print("Found vgnames: {}".format(found_vgnames))
        assert vgname in found_vgnames['/dev/vdh']
        assert not found_vgnames['/dev/vdi']

    def test_find_lvm_pv_vm_mapping(self):
        print(storage.find_lvm_pv_vm_mapping())

    def test_get_virtio_device_by_disk(self):
        guid = "e1de1a6d-cc55-477a-9148-c8bf403c5602"
        print(storage.get_virtio_device_by_disk(guid))

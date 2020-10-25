from pathlib import Path

import alog

from linux2v.lib.macho.host import sort_versions
from linux2v.lib import cli
from linux2v.lib.macho import storage
from linux2v.launcher.converter import SystemDiskConverter
from linux2v.launcher.converter import GrubConverter
from linux2v.launcher.networkconverter import NetworkConverter

from .base import BaseLauncherTest

print = alog.debug


class TestSystemDiskConverter(BaseLauncherTest):
    def setup(self):
        self.disks_lun_mapping = {"60874b4b-4652-4a0d-a4ca-f0daec064429":
                                  "/dev/vdb"}
        self.converter = SystemDiskConverter("testing_job")
        self.root_mnt_path = Path("tmp_testing")
        self.boot_mnt_path = self.root_mnt_path / "boot"
        self.converter.os_root_path = Path("tests/data/centos7etc/")
        for folder in self.converter.part_folders['root']:
            cli.mkdir(self.root_mnt_path / folder, quiet=True)
        cli.mkdir(self.boot_mnt_path / "grub", quiet=True)
        cli.copy(Path("tests/data/centos6etc/etc/") / "redhat-release",
                 self.root_mnt_path / "etc")
        for file in ["grub/grub.cfg", "vmlinuz-2.6.32-431.el6.x86_64"]:
            cli.copy(Path("tests/data/bootroot/boot/") / file,
                     self.boot_mnt_path / file)
        self.rclocal = self.root_mnt_path / "etc" / "rc.local"
        cli.run("touch {}".format(self.rclocal))

    def teardown(self):
        cli.run("rm -rf {}".format(self.root_mnt_path), dry=False)

    def test_convert(self):
        assert self.converter.convert(self.disks_lun_mapping,
                                      root_mnt_path=self.root_mnt_path,
                                      boot_mnt_path=self.boot_mnt_path)

    def test_sort_versions(self):
        folder_path = Path("tests/data/boot")
        kernels = self.converter.find_installed_kernels(folder_path)
        print(alog.pformat(kernels))
        versions = self.converter.extract_versions_from_kernels(kernels)
        print(alog.pformat(versions))
        sorted_versions = sort_versions(versions)
        print(alog.pformat(sorted_versions))

    def test_find_convertable_grub_config(self):
        assert self.converter.find_convertable_grub_config(self.boot_mnt_path)

    def test_find_boot_partition(self):
        assert self.converter.find_boot_partition(self.disks_lun_mapping,
                                                  self.boot_mnt_path)

    def test_chrootdracut(self):
        kernel_versions = ['3.10.0-327.18.2.el7.x86_64',
                           '3.10.0-327.el7.x86_64']
        self.converter.chrootdracut(self.root_mnt_path,
                                    kernel_versions=kernel_versions)

    def test__convert_kernel(self):
        self.converter._convert_kernel(self.root_mnt_path,
                                       os_family="Red",
                                       os_version="3.10.0-327.el7.x86_64")

    def test__cook_device_prefixes_to_find(self):
        device_prefixes = self.converter \
            ._cook_device_prefixes_to_find(self.disks_lun_mapping)
        print("device_prefixes: {}".format(device_prefixes))
        disk_partitions = storage.find_disk_partitions(device_prefixes)
        print(alog.pformat(disk_partitions))

    def test_inject_runonced_folder(self):
        self.converter.inject_runonced(self.root_mnt_path / "etc")
        assert (self.root_mnt_path / "etc/runonce.d/bin/runonce.sh").exists()
        with self.rclocal.open("rb") as f:
            content = f.read()
            assert self.converter.rclocal_runonce in content

    def test_write_runonced_script(self):
        sh_content = """#!/bin/sh\necho "Hello"\necho "World"."""
        script_name = "testing_write.sh"
        cli.mkdir(self.root_mnt_path / "etc/runonce.d/run", quiet=True)
        self.converter.write_runonced_script(self.root_mnt_path / "etc",
                                             script_name,
                                             sh_content)
        script_filepath = \
            self.root_mnt_path / "etc/runonce.d/run/" / script_name
        assert script_filepath.exists()
        with script_filepath.open("r") as f:
            content = f.read()
            assert content == sh_content

    def test_inject_runonced_scripts(self):
        os_family = "Red"
        os_version = "3.10.0-327.el7.x86_64"
        networkconverter = NetworkConverter(
            os_family=os_family,
            os_version=os_version,
            os_root_path=self.converter.os_root_path,
            backup_folder=self.converter.os_root_path / 'backup_folder'
        )
        self.converter.inject_runonced(self.root_mnt_path / "etc")
        scripts = list(networkconverter.generate_runonce_scripts())

        host_py_content = self.converter.HOST_PY_TEMPLATE.format(
            os_version=os_version, os_family=os_family)
        self.converter.inject_runonced_script(self.root_mnt_path / "etc",
                                              scripts, host_py_content)
        script_name = Path(scripts[-1]).name
        assert (
            self.root_mnt_path / "etc/runonce.d/run" / script_name
        ).exists()


class TestGrubConverter(BaseLauncherTest):
    def setup(self):
        self.converter = GrubConverter()

    def test_inject_tty_console_to_boot_ubuntu1204(self):
        root_mnt_path = Path("tests/data/ubuntu1204boot")
        grub_config = {'bootcfg': "grub/grub.cfg"}
        output_path = root_mnt_path / "boot/grub/grub.new.cfg"

        self.converter.inject_tty_console_to_boot(
            root_mnt_path, grub_config, output_path=output_path)
        with output_path.open("r") as f:
            assert "console=ttyS0,115200n8 console=tty0" in f.read()

    def test_inject_tty_console_to_boot_ubuntu1204_noLVM(self):
        root_mnt_path = Path("tests/data/ubuntu1204etc")
        grub_config = {'bootcfg': "grub/grub.cfg"}
        output_path = root_mnt_path / "boot/grub/grub.new.cfg"

        self.converter.inject_tty_console_to_boot(root_mnt_path,
                                                  grub_config,
                                                  output_path=output_path)
        with output_path.open("r") as f:
            assert "console=ttyS0,115200n8 console=tty0" in f.read()

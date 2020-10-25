from pathlib import Path

from linux2v.lib.macho import host

from .base import BaseMachoTest


class TestHost(BaseMachoTest):
    def test_detect_centos7_in_etc_folder(self):
        distribution, version = host.detect_os_version_in_etc_folder(
            Path("tests/data/centos7etc/etc/")
        )
        assert distribution == "CentOS"
        assert print(version) or version.startswith("7")

    def test_detect_ubuntu1404_in_etc_folder(self):
        distribution, version = host.detect_os_version_in_etc_folder(
            Path("tests/data/ubuntu1404etc/etc/")
        )
        assert distribution == "Ubuntu"
        assert print(version) or version.startswith("14.04")

    def test_detect_ubuntu1204_in_etc_folder(self):
        distribution, version = host.detect_os_version_in_etc_folder(
            Path("tests/data/ubuntu1204etc/etc/")
        )
        assert distribution == "Ubuntu"
        assert print(version) or version.startswith("12.04")

    def test_detect_ubuntu1604_in_etc_folder(self):
        distribution, version = host.detect_os_version_in_etc_folder(
            Path("tests/data/ubuntu1604/etc/")
        )
        assert distribution == "Ubuntu"
        assert print(version) or version.startswith("16.04")

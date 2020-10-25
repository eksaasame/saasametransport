from pprint import pprint

from pathlib import Path
from linux2v.launcher.networkconverter import NetworkConverter

from .base import BaseLauncherTest


class TestNetworkConverter(BaseLauncherTest):

    def test__get_template(self):
        self.converter = NetworkConverter(
            os_version="6.7",
            os_family="CentOS",
            os_root_path=Path("tests/data/centos7etc/"),
            backup_folder=Path("tests/data/backup2v")
        )
        eth = self.converter._get_template("eth")
        pprint(eth)

    def test__get_template_for_rhel6(self):
        self.converter = NetworkConverter(
            os_version="6.7",
            os_family="Red",
            os_root_path=Path("tests/data/centos7etc/"),
            backup_folder=Path("tests/data/backup2v")
        )
        eth = self.converter._get_template("eth")
        pprint(eth)

    def test__get_template_for_ubuntu1204(self):
        self.converter = NetworkConverter(
            os_version="12.04.5",
            os_family="Ubuntu",
            os_root_path=Path("tests/data/ubuntu1204etc/"),
            backup_folder=Path("tests/data/backup2v")
        )
        eth = self.converter._get_template("eth")
        pprint(eth)

    def test__get_template_for_ubuntu1404(self):
        self.converter = NetworkConverter(
            os_version="12.04.5",
            os_family="Ubuntu",
            os_root_path=Path("tests/data/ubuntu1404etc/"),
            backup_folder=Path("tests/data/backup2v")
        )
        eth = self.converter._get_template("eth")
        pprint(eth)

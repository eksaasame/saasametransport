from pathlib import Path
from pprint import pprint

import pytest

from linux2v.launcher import thirdparty
from linux2v.lib import cli


simple_driver_path = Path("data/thirdparty") / "GenericDriver"


@pytest.mark.skipif(not simple_driver_path.exists(),
                    reason="Not existed {}".format(simple_driver_path))
class TestGenericDriver:
    def setup(self):
        self.root_mnt = Path("tmp_testing")
        self.driver = thirdparty.GenericDriver(simple_driver_path)

    def teardown(self):
        cli.run("rm -rf {}".format(self.root_mnt), dry=False)

    @pytest.mark.skipif(not (simple_driver_path
                             / "copyfiles/CentOS/6/x86_64").exists(),
                        reason="Not existed {}".format(simple_driver_path))
    def test_customize_centos_6_x86_64(self):
        res = self.driver.customize(self.root_mnt, "CentOS", "6.5", 'x86_64')
        pprint(res)
        print(res['runonce_custom']['sh_content'])

    @pytest.mark.skipif(not (simple_driver_path
                             / "copyfiles/CentOS/7/x86_64").exists(),
                        reason="Not existed {}".format(simple_driver_path))
    def test_customize_centos_7_x86_64(self):
        res = self.driver.customize(self.root_mnt, "CentOS", "7.2", 'x86_64')
        pprint(res)
        print(res['runonce_custom']['sh_content'])


def test_find_existed_parties():
    res = list(thirdparty.find_existed_parties())
    pprint(res)

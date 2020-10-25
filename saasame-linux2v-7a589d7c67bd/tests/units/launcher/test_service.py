from linux2v.launcher.service import Launcher

from .base import BaseLauncherTest


class TestLauncher(BaseLauncherTest):
    def setup(self):
        self.launcher = Launcher(environ=self.test_environ)

    def test_service(self):
        print(self.launcher.service)

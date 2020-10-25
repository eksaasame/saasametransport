from .base import BaseLibTest
from linux2v.lib import cli


class TestCli(BaseLibTest):

    def setup(self):
        self.test_file_name = "tmptestfile"
        self.test_file = "scripts/runonced/" + self.test_file_name
        self.target_folder = "tmp"
        self.dirname = self.target_folder + "/" + "testdir"
        cli.mkdir(self.dirname, quiet=True)

    def teardown(self):
        cli.rmdir(self.dirname)

    def test_copy_src_inside_linux2v(self):
        cli.run("touch " + "linux2v/" + self.test_file)
        cli.copy(self.test_file, self.target_folder, src_inside_linux2v=True)
        cli.run("rm " + "linux2v/" + self.test_file)
        cli.run("rm " + self.target_folder + "/" + self.test_file_name)

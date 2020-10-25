from pathlib import Path

import arrow

from ..lib import cli
from ..lib import log
from .templates import get_template


class NetworkConverter:
    def __init__(self, os_version, os_family,
                 os_root_path: Path,
                 backup_folder: Path):
        self.os_version = os_version
        self.os_family = os_family
        # os_root_path is the "/a/b/c" part of "/a/b/c/etc".
        self.os_root_path = os_root_path
        self.backup_folder = backup_folder

    def move_to_backup(self, file):
        backup_filename = "{}.{}".format(Path(file).name, arrow.utcnow())
        filepath = self.mountedpath(file)
        cli.move(filepath, self.backup_folder / backup_filename)

    def rescue_from_backup(self, file, filename):
        backup_file = self.backup_folder / filename
        mountedpath = self.mountedpath(file)
        cli.copy(backup_file, mountedpath)

    def mountedpath(self, given_path):
        if not str(given_path).startswith(str(self.os_root_path)):
            if str(given_path).startswith("/"):
                return self.os_root_path / str(given_path)[1:]
            return self.os_root_path / given_path
        return Path(given_path)

    def reset_sysconfig_network(self):
        self._reset_setting_content('network')

    def reset_resolvconf(self):
        self._reset_setting_content('resolvconf')

    def _reset_setting_content(self, setiing):
        item = self._get_template(setiing)
        target = self.mountedpath(item['target'])
        self.move_to_backup(target)
        cmd = "echo {} > {}".format(item.get('content', ''), target)
        cli.run(cmd)

    def _get_template(self, setting):
        log.warning("get_template {}-{}-{}".format(setting,self.os_family,self.os_version ))
        return get_template(setting, self.os_family, self.os_version,
                            none_if_no_matched=True)

    # def reset_network_manager(self):
    #   cli.run("service network-manager stop")
    #   self.move_to_backup("/var/lib/NetworkManager/NetworkManager.state")

    # def reset_dhclient(self):
    #   self.move_to_backup("/var/lib/dhclient/dhclient.leases")

    def _remove_template_files(self, tpl):
        log.warning("Remove template files {}{}".format(tpl['folder'],
                                                        tpl['glob']))
        mountedpath = self.mountedpath(tpl['folder'])
        if not mountedpath.exists():
            log.warning("Path not exists: {}".format(tpl['folder']))
            return

        for file in mountedpath.glob(tpl['glob']):
            self.move_to_backup(file)

    def reset_network_interfaces(self):
        log.warning("reset_network_interfaces")
        eth_template = self._get_template('eth')
        self._remove_template_files(eth_template)
        networkrules_template = self._get_template('networkrules')
        self._remove_template_files(networkrules_template)
        bootup_network_interfaces = \
            self._get_template('bootup_network_interfaces')
        if bootup_network_interfaces:
            filepath = self.os_root_path / bootup_network_interfaces['file']
            with filepath.open(mode="wb") as f:
                f.write(bootup_network_interfaces['content'].encode('utf-8'))

    def reset_all(self):
        log.warning("Remove all system networking.")
        # self.reset_resolvconf()
        # /var self.reset_dhclient()
        # /var self.reset_network_manager()  # if network manager is running
        # self.reset_sysconfig_network()
        self.reset_network_interfaces()

    def generate_runonce_scripts(self):
        # Here we can generate static addresss for failback mode.
        log.warning("Generate runonce scripts.")
        scripts_folder = "scripts/runonced/"
        script_names = [
            '6_x2v_firstboot.sh',
            '_python.sh',
            '_6_x2v_firstboot.py',
            '_reset_redhat_network.py',
            '_reset_ubuntu_network.py',
            '_2vsettings_redhat.py',
            '_2vsettings_ubuntu.py',
            '_templates.py',
            '_utils.py'
        ]
        for script_name in script_names:
            yield scripts_folder + script_name

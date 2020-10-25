from pathlib import Path
from collections import (
    OrderedDict,
    Mapping
)

from pyramid.decorator import reify

from ..lib import log
from ..lib import cli
from .templates import get_template


def deep_dict_update(d, u):
    for k, v in u.items():
        if isinstance(v, Mapping):
            r = deep_dict_update(d.get(k, {}), v)
            d[k] = r
        else:
            d[k] = u[k]
    return d


def drivers_root_path():
    return Path("/etc/linux2v/thirdparty")


def _is_folder_exists_and_not_empty(path):
    return path.exists() and path.is_dir() and any(path.iterdir())


def find_existed_parties():
    if not _is_folder_exists_and_not_empty(drivers_root_path()):
        return []

    possible_parties = OrderedDict([('GenericDriver', GenericDriver)])
    root_path = drivers_root_path()
    for folder_name, driver_class in possible_parties.items():
        path = root_path / folder_name
        if _is_folder_exists_and_not_empty(path):
            yield driver_class(path)


class BaseThirdParty:
    """
    Example path of source copyfiles:
      /etc/linux2v/thirdparty/GenericDriver/copyfiles/CentOS/6/x86_64/*
    """

    default_copyfiles_folder = 'etc/runonce.d/copyfiles'

    def _copy_file(self, filepath, target_folder_path=None):
        to_path = target_folder_path or self.default_copyfiles_folder
        cli.copy(src=filepath, dst=to_path, quiet_log=True)

    def _mkdir(self, root_mnt_path, target_folder):
        target_folder_path = root_mnt_path / target_folder
        if not target_folder_path.exists():
            cli.mkdir(target_folder_path, quiet=True)
        return target_folder_path

    def copy_targets_files(self, root_mnt_path, targets,
                           os_family, os_version_tuple, arch):
        files_copied = []
        for target in targets:
            target_folder_path = self._mkdir(root_mnt_path, target['folder'])
            src_path = (self.path / "copyfiles" / os_family /
                        target.get('src_folder').format(
                            os_main_version=os_version_tuple[0], arch=arch))
            for file in target['files']:
                self._copy_file(src_path / file, target_folder_path)
                files_copied.append(target['folder'] + '/' + file)
        log.info("Files copied: {}".format(files_copied))

    def copyfiles(self, root_mnt_path, os_family, os_version, arch, templates):
        copyfiles_targets = get_template('copyfiles', os_family, os_version,
                                         given_templates=templates)
        if copyfiles_targets:
            log.info("Found copyfiles targets: {}".format(copyfiles_targets))
            self.copy_targets_files(root_mnt_path, copyfiles_targets,
                                    os_family, os_version.split("."), arch)
        else:
            log.warning("No copyfiles targets found.")


class RPMMixin:

    target_rpm_folder = 'etc/runonce.d/rpms'

    def _inject_rpm(self, filepath, target_folder_path=None):
        log.info("Inject RPM: {}".format(filepath.name))
        self._copy_file(filepath, target_folder_path)
        sh_content = "\nrpm -ivh --replacepkgs /{!s}/{!s}\n".format(
            self.target_rpm_folder, filepath.name)
        return sh_content

    def _inject_all_rpm_in(self, rpm_src_path, target_folder_path=None):
        sh_content = ""
        filepaths = list(rpm_src_path.iterdir())
        log.info("Found RPMs: {}".format([f.name for f in filepaths]))
        for filepath in filepaths:
            if not filepath.name.endswith('.rpm'):
                continue

            sh_content += self._inject_rpm(filepath, target_folder_path)
        return sh_content

    def inject_rpm_targets(self, root_mnt_path, rpm_src_path, targets,
                           os_version, arch):
        to_path = self._mkdir(root_mnt_path, self.target_rpm_folder)
        log.info("Inject {} target(s) to {!s}".format(len(targets), to_path))
        for target in targets:
            src_path = rpm_src_path / target.get('src_folder').format(
                os_main_version=os_version.split(".")[0], arch=arch)
            return self._inject_all_rpm_in(src_path, to_path)


class GenericDriver(RPMMixin, BaseThirdParty):

    _default_templates = {}

    def __init__(self, path):
        self.path = path

    @reify
    def templates(self):
        if (self.path / "generic_templates.py").exists():
            import sys
            sys.path.append("{!s}".format(self.path.resolve()))
            print("{!s}".format(self.path.resolve()))
            from generic_templates import templates
        else:
            templates = {}
        return deep_dict_update(self._default_templates.copy(), templates)

    def customize(self, root_mnt_path, os_family, os_version, arch) -> dict:
        self.copyfiles(root_mnt_path, os_family, os_version, arch,
                       self.templates)
        runonce_sh_content = "#!/bin/sh\n"
        rpm_src_path = self.path / 'rpms' / os_family
        if not (rpm_src_path.exists() and arch):
            log.warning("Not existed {!s} with arch:{}".format(rpm_src_path,
                                                               arch))
            return {}

        log.info("Found {!s} with arch:{}".format(rpm_src_path, arch))
        rpm_targets = get_template('rpms', os_family, os_version,
                                   given_templates=self.templates)
        if rpm_targets:
            log.info("Found RPM targets: {}".format(rpm_targets))
            rpm_sh_conent = self.inject_rpm_targets(
                root_mnt_path, rpm_src_path, rpm_targets, os_version, arch)
            runonce_sh_content += rpm_sh_conent
        else:
            log.warning("No RPM targets found.")

        postsh_targets = get_template('postsh', os_family, os_version,
                                      given_templates=self.templates)
        for target in postsh_targets:
            if 'cmds' in target:
                for cmd in target['cmds']:
                    runonce_sh_content += "\n" + cmd + "\n"

        followup_actions = {
            'runonce_custom': {
                'sh_name': "chinac",
                'sh_content': runonce_sh_content,
                'root_mnt_path': root_mnt_path
            }
        }
        return followup_actions

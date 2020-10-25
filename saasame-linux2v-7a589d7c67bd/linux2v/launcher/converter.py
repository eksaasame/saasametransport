import re
import os
import shutil
import json

from pathlib import Path

from functional import seq

from ..exceptions import (
    DiskPartitionNotFound,
    OSUnknown
)
from ..lib import log
from ..lib import cli
from ..lib.macho.storage import (
    disk,
    part,
    volume,
    storage,
    find_disk_partitions,
    find_lvm_pv_vm_mapping,
    find_bootable_partitions,
)
from ..lib.macho.disk import (
    Disk,
    MountContext,
)
from ..lib.macho.host import (
    sort_versions,
    get_hardware_info,
    detect_os_version_in_etc_folder
)
from .networkconverter import NetworkConverter
from .templates import get_template, get_family, get_dist_display, get_drivers_injection, get_drivers_verify
from . import thirdparty

MIN_VERSION_CONTAINS_VIRTIO = '2.6.9-5'
REGEX = {
    'vmlinuz_version': re.compile('vmlinuz-(\d.\d+.\d+-\d*)(.|-)*'),
    'indent': re.compile('(?P<indent>^\s*)'),
    'grub_linux_root':
    re.compile('\s*[a-zA-Z]+\s*\S*/vmlinuz-\S+\s+\S*\s*root=+'),
    'grub_cmdline_linux': re.compile(
        '^(?P<setting>\s*GRUB_CMDLINE_LINUX\s*=\s*)"(?P<value>.*)"'),
    'grub_root_console': re.compile(
        '(?P<tty0>console=tty0,?\S*)|(?P<ttyS0>console=ttyS0,?\S*)'
    )
}
JOBS_FOLDER = "/var/run/linux2v/jobs" \
    if cli.os_disk_operable else "/tmp/linux2v.jobs"


class GrubConverter:

    @classmethod
    def improve_grub_for_double_boot(cls, bootcfg_path):
        with bootcfg_path.open('r',encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_content = []
        for line in content.split('\n'):
            line = line.replace('linuxefi ','linux ').replace('initrdefi ','initrd ').replace('linux16 ','linux ').replace('initrd16 ','initrd ')
            new_content.append(line)
        log.info("Write new content to {}".format(bootcfg_path))
        with bootcfg_path.open('wb') as f:
            f.write("\n".join(new_content).encode('utf-8'))

    @classmethod
    def set_numa_off_for_grub(cls, root_mnt_path, output_path=None):
        bootcfg_path = root_mnt_path / 'boot/grub/grub.conf'
        if not bootcfg_path.exists():
            return
        with bootcfg_path.open('r',encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_content = []
        for line in content.split('\n'):
            if REGEX['grub_linux_root'].match(line):
                numas= re.compile('\s*numa=[a-zA-Z]+\s*').findall(line)        
                for numa in numas:
                    line= line.replace(numa,'')
                line += " numa=off"
            new_content.append(line)
        output_path = output_path or bootcfg_path
        log.info("Write new content to {}".format(output_path))
        with output_path.open('wb') as f:
            f.write("\n".join(new_content).encode('utf-8'))

    @classmethod
    def inject_tty_console_to_boot(cls, root_mnt_path, grub_config,
                                   output_path=None):
        bootcfg_path = root_mnt_path / 'boot' / grub_config['bootcfg']
        log.info("Inject tty to {}".format(bootcfg_path))
        with bootcfg_path.open('r',encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_content = []
        for line in content.split('\n'):
            if REGEX['grub_linux_root'].match(line):
                line = cls._reassemble_grub_root(line)
            new_content.append(line)
        output_path = output_path or bootcfg_path
        log.info("Write new content to {}".format(output_path))
        with output_path.open('wb') as f:
            f.write("\n".join(new_content).encode('utf-8'))

    @classmethod
    def inject_tty_console_to_etc_default_grub(cls, root_mnt_path,
                                               output_path=None):
        etcdefault_path = root_mnt_path / 'etc/default/grub'
        if not etcdefault_path.exists():
            log.warning("Not found {}".format(etcdefault_path))
            return

        log.info("Inject tty to {}".format(etcdefault_path))
        with etcdefault_path.open('r',encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_content = []
        default_prefix = 'GRUB_CMDLINE_LINUX='
        found = False
        for line in content.split('\n'):
            matched = REGEX['grub_cmdline_linux'].match(line)
            if matched:
                line = '{}"{}"'.format(
                    matched.group('setting') or default_prefix,
                    cls._reassemble_grub_cmdline_linux(matched.group('value'))
                )
                found = True
            new_content.append(line)
        if not found:
            line = '{}"{}"'.format(
                default_prefix, "console=ttyS0,115200n8 console=tty0")
            new_content.append(line)
        output_path = output_path or etcdefault_path
        log.info("Write new content to {}".format(output_path))
        with output_path.open('wb') as f:
            f.write("\n".join(new_content).encode('utf-8'))

    @staticmethod
    def _reassemble_grub_root(line):
        LOG_BOOT_CONSOLE = {
            'tty0': "console=tty0",
            'ttyS0': "console=ttyS0,115200n8"
        }
        terms = line.split()
        new_line = []
        found_ttys = {}

        for term in terms:
            matched = REGEX['grub_root_console'].match(term)
            if not matched:
                new_line.append(term)
            else:
                for key, value in matched.groupdict().items():
                    if value:
                        found_ttys[key] = value
        for tty in ('ttyS0', 'tty0'):  # order does master
            if tty in found_ttys:
                new_line.append(found_ttys[tty])
            else:
                new_line.append(LOG_BOOT_CONSOLE[tty])
        indent = REGEX['indent'].match(line).group('indent')
        return indent + " ".join(new_line)

    @staticmethod
    def _reassemble_grub_cmdline_linux(line):
        LOG_BOOT_CONSOLE = {
            'tty0': "console=tty0",
            'ttyS0': "console=ttyS0,115200n8"
        }
        terms = line.split()
        new_line = []
        found_ttys = {}

        for term in terms:
            matched = REGEX['grub_root_console'].match(term)
            if not matched:
                new_line.append(term)
            else:
                for key, value in matched.groupdict().items():
                    if value:
                        found_ttys[key] = value
        for tty in ('ttyS0', 'tty0'):  # order does master
            if tty in found_ttys:
                new_line.append(found_ttys[tty])
            else:
                new_line.append(LOG_BOOT_CONSOLE[tty])
        return " ".join(new_line)


class SystemDiskConverter:
    """
    Convert the system settings and services.
    """
    rclocal_runonce = b"\n/etc/runonce.d/bin/runonce.sh\nexit 0\n"
    part_folders = {
        'root': ['boot', 'home', 'etc', 'lib'],
        'usr': ['share', 'lib', 'local'],
        'var': ['log', 'run', 'spool'],
    }
    HOST_PY_TEMPLATE = ('\nos_version = "{os_version}"'
                        '\nos_family = "{os_family}"'
                        '\n')

    def __init__(self, job_id, callbacks, timeout, backup_folder_postfix='', type='', nocompress = False, selinux = True, boot='bios', shrink = 0):
        backup_folder = "backup2v{}".format(
            ("-" + backup_folder_postfix) if backup_folder_postfix else "")
        self.backup_folder = backup_folder
        self.nocompress = nocompress;
        self.os_root_path = None  # Defined in _stage2.
        jobs_folder_path = Path("/var/run/linux2v/jobs"
                                if cli.os_disk_operable else
                                "/tmp/linux2v.jobs")
        self.workspace_path = jobs_folder_path / job_id
        self.root_mnt_path = self.workspace_path / "root"
        self.boot_mnt_path = self.workspace_path / "boot"
        self.efi_mnt_path = self.boot_mnt_path / "efi"
        self.callbacks = callbacks
        self.timeout = timeout
        self.type = type
        self.selinux = selinux
        self.boot = boot
        self.platform = ""
        self.version = ""
        self.architecture =""
        self.hostname =""
        self.can_rebuild = False
        self.shrink = shrink
        if not self.workspace_path.exists():
            cli.mkdir(self.workspace_path, quiet=True)
        else:
            log.warning("{} existed.".format(self.workspace_path))
        cli.mkdir(self.root_mnt_path, quiet=True, quiet_log=True)
        cli.mkdir(self.boot_mnt_path, quiet=True, quiet_log=True)

    def cleanup(self):
        cli.rmdir(self.root_mnt_path, quiet=True, quiet_log=True)
        if self.boot_mnt_path.exists():
            cli.rmdir(self.boot_mnt_path, quiet=True, quiet_log=True)

    def run_on_mounts(self, final_call, mnt_parts, root_mnt_path, to_mounts):
        if not to_mounts:
            return final_call()

        to_mount = to_mounts.pop(0)
        if to_mount in mnt_parts:
            with MountContext(mnt_parts[to_mount], root_mnt_path / to_mount):
                self.run_on_mounts(
                    final_call, mnt_parts, root_mnt_path, to_mounts)
        else:
            self.run_on_mounts(final_call, mnt_parts, root_mnt_path, to_mounts)
    
    def _get_host_name(self, family, root_mnt_path):
        name = ""
        if family == "RedHat":
            sysconfig_network = root_mnt_path / 'etc/sysconfig/network'
            log.info("read {}".format(sysconfig_network))
            content = []
            with sysconfig_network.open('r',encoding='utf-8', errors='ignore') as f:
                content = f.read()
            for line in content.split('\n'):
                print( "{}".format(line))
                if line.startswith("HOSTNAME="):
                    arr = line.split('=')
                    if len(arr) > 1:
                        name = arr[1]
        elif family == "Ubuntu":
            etc_hostname = root_mnt_path / 'etc/hostname'
            log.info("read {}".format(etc_hostname))
            content = []
            with etc_hostname.open('r',encoding='utf-8', errors='ignore') as f:
                content = f.read()
            for line in content.split('\n'):
                name = line
                break
        elif family == "SuSE":
            etc_hostname = root_mnt_path / 'etc/HOSTNAME'
            log.info("read {}".format(etc_hostname))
            content = []
            with etc_hostname.open('r',encoding='utf-8', errors='ignore') as f:
                content = f.read()
            for line in content.split('\n'):
                name = line
                break
        return name

    def _convert_boot_partition(self, root_mnt_path, mnt_parts):
        grub_config = mnt_parts['boot']['grub_config']
        try:
            dist, os_version = \
                detect_os_version_in_etc_folder(root_mnt_path / "etc")
        except ValueError as error:
            raise OSUnknown("Error on parsing: {}".format(error))
        if not (dist and os_version):
            raise OSUnknown("None found in Distribution:{}, OS version:{}"
                            .format(dist, os_version))

        log.info("Found dist: {}, os_version: {}, to root_mnt_path {}"
                 .format(dist, os_version, root_mnt_path))
        self._convert_network_settings(root_mnt_path, dist, os_version)
        
        if not self.selinux:
            self._disable_selinux(root_mnt_path)
        """
        remove linux packer settings
        """
        linux_packer_settings_path = root_mnt_path / "usr/local/linux_packer/.hostConfig"
        if linux_packer_settings_path.exists():
            os.remove( linux_packer_settings_path )
        
        linux_packer_settings_path = root_mnt_path / "usr/local/linux_packer/.snapshot_config"
        if linux_packer_settings_path.exists():
            os.remove( linux_packer_settings_path )

        dkms = root_mnt_path/"etc/systemd/system/multi-user.target.wants/dkms.service"
        if dkms.exists():
            os.remove( dkms )

        family = get_family(dist)
        lib64 = root_mnt_path / "lib64"
        if lib64.exists():
            self.architecture = "amd64"
        else:
            self.architecture = "x86"
        versions = os_version.split(".")
        major_version = int(versions[0])
        if len(versions) > 1 :
            minor_version = int(versions[1])
        else:
            minor_version = 0
        
        self.platform = get_dist_display(dist)
        self.version = "{}.{}".format(major_version,minor_version)
        self.hostname = self._get_host_name(family, root_mnt_path)

        if family == "RedHat" and major_version >= 4:
            if major_version >= 7 and minor_version > 0:
                self.can_rebuild = True;
            if self.type == "Microsoft" :
                if major_version < 6 :
                    GrubConverter.set_numa_off_for_grub(root_mnt_path)
                elif major_version == 6 and minor_version < 6 :
                    GrubConverter.set_numa_off_for_grub(root_mnt_path)
                GrubConverter.inject_tty_console_to_boot(root_mnt_path, grub_config)
                GrubConverter.inject_tty_console_to_etc_default_grub(root_mnt_path)
            log.info("Need to convert kernel")
            def final_call():         
                self._convert_kernel(root_mnt_path, dist, os_version, root_mnt_path / 'boot' / grub_config['bootcfg'])
                if self.boot == 'bios' :
                    self.grub_install(root_mnt_path, mnt_parts['boot'])
            to_mounts = ['usr', 'var']
            self.run_on_mounts(final_call, mnt_parts, root_mnt_path, to_mounts)
            if self.boot == 'bios' : 
                bootcfg_path = root_mnt_path / 'boot' / grub_config['bootcfg']
                if grub_config['bootcfg'].startswith('efi/EFI') :
                    shutil.move( bootcfg_path, root_mnt_path / ('boot/{}/grub.cfg'.format(grub_config['version'])))
                    bootcfg_path = root_mnt_path / ('boot/{}/grub.cfg'.format(grub_config['version']))
                GrubConverter.improve_grub_for_double_boot(bootcfg_path)
        elif family == "SuSE" and major_version >= 12:
            self.can_rebuild = True;
            if self.type == "Microsoft" :
                GrubConverter.inject_tty_console_to_boot(root_mnt_path, grub_config)
                GrubConverter.inject_tty_console_to_etc_default_grub(root_mnt_path)
            log.info("Need to convert kernel")
            def final_call():         
                self._convert_kernel(root_mnt_path, dist, os_version, root_mnt_path / 'boot' / grub_config['bootcfg'])
                if self.boot == 'bios' :
                    self.grub_install(root_mnt_path, mnt_parts['boot'])
            to_mounts = ['usr', 'var']
            self.run_on_mounts(final_call, mnt_parts, root_mnt_path, to_mounts)
            if self.boot == 'bios' : 
                bootcfg_path = root_mnt_path / 'boot' / grub_config['bootcfg']
                if grub_config['bootcfg'].startswith('efi/EFI') :
                    shutil.move( bootcfg_path, root_mnt_path / ('boot/{}/grub.cfg'.format(grub_config['version'])))
                    bootcfg_path = root_mnt_path / ('boot/{}/grub.cfg'.format(grub_config['version']))
                GrubConverter.improve_grub_for_double_boot(bootcfg_path)
        elif family == "Ubuntu" or family == "SuSE":
            if self.boot == 'bios' :
                def final_call():         
                    self.grub_install(root_mnt_path, mnt_parts['boot'])
                to_mounts = ['usr', 'var']
                self.run_on_mounts(final_call, mnt_parts, root_mnt_path, to_mounts)
            GrubConverter.inject_tty_console_to_boot(root_mnt_path,
                                                     grub_config)
            GrubConverter.inject_tty_console_to_etc_default_grub(root_mnt_path)

    def _stage2(self, mnt_parts, root_mnt_path):
        boot_partition = mnt_parts['boot']
        self.os_root_path = self.os_root_path or root_mnt_path
        if mnt_parts['root'] != boot_partition['device']:
            boot_mnt_path = root_mnt_path / "boot"
            with MountContext(boot_partition['device'], boot_mnt_path,
                              dry=not cli.os_disk_operable):
                log.info("boot '{}' partition is different from root.".format(boot_mnt_path))
                if 'efi' in boot_partition:
                    efi_mnt_path = boot_mnt_path / "efi"
                    log.info("efi partition: '{}'".format(efi_mnt_path))
                    with MountContext(boot_partition['efi'], efi_mnt_path,
                                      dry=not cli.os_disk_operable):
                        self._convert_boot_partition(root_mnt_path, mnt_parts)
                else:
                    self._convert_boot_partition(root_mnt_path, mnt_parts)
        else:
            boot_mnt_path = root_mnt_path
            log.info("boot '{}' partition is the same with root.".format(boot_mnt_path))
            if 'efi' in boot_partition:
                efi_mnt_path = boot_mnt_path / "boot/efi"
                log.info("efi partition: '{}'".format(efi_mnt_path))
                with MountContext(boot_partition['efi'], efi_mnt_path,
                                    dry=not cli.os_disk_operable):
                    self._convert_boot_partition(root_mnt_path, mnt_parts)
            else:
                self._convert_boot_partition(root_mnt_path, mnt_parts)

        return Disk(guid=boot_partition['guid'],
                    device=boot_partition['device'])

    @staticmethod
    def _cook_device_prefixes_to_find(disk_devices):
        lvm_pv_vg_mapping = find_lvm_pv_vm_mapping()
        log.info("lvm_pv_vg_mapping: \n {}\n".format(lvm_pv_vg_mapping));
        device_prefixes = set()
        for disk_guid, device in disk_devices.items():
            device_prefixes.add(device)
            for pv, vgname in lvm_pv_vg_mapping.items():
                dev = pv.rstrip('1234567890')
                if dev == device:
                    device_prefixes.add(
                    '/dev/mapper/' + vgname.replace("-", "--"))     
        return device_prefixes

    def convert(self, disk_devices,
                root_mnt_path=None, boot_mnt_path=None):
        """
        "root_mnt_path" and "boot_mnt_path" is given in different setting while
        running in testing.
        """
        root_mnt_path = root_mnt_path or self.root_mnt_path
        boot_mnt_path = boot_mnt_path or self.boot_mnt_path
        log.info("Start convert disk_devices:{}"
                 .format(log.pformat(disk_devices)))
        boot_partition = self.find_boot_partition(disk_devices,
                                                  boot_mnt_path)
        device_prefixes = self._cook_device_prefixes_to_find(disk_devices)
        disk_partitions = find_disk_partitions(device_prefixes=device_prefixes)
        log.info("Found disk partitions: {!r} on prefixes {!r}".format(
            disk_partitions, device_prefixes))

        mnt_parts = {'boot': boot_partition}
        if 'device' in boot_partition:
            log.info("partition_device: {}, root_mnt_path: {}".format(boot_partition['device'],root_mnt_path))
            with MountContext(boot_partition['device'], root_mnt_path,dry=not cli.os_disk_operable):
                for part, folders_to_find in self.part_folders.items():
                    if self.has_folders_in(root_mnt_path, folders_to_find):
                        mnt_parts[part] = boot_partition['device']
                        break

        if not 'root' in mnt_parts:
            for disk_device in disk_devices.values():
                for partition_device in disk_partitions[disk_device]:
                    if 'device' in boot_partition and partition_device == boot_partition['device'] :
                        continue
                    log.debug(partition_device)
                    log.info("partition_device: {}, root_mnt_path: {}".format(partition_device,root_mnt_path))
                    with MountContext(partition_device, root_mnt_path,dry=not cli.os_disk_operable):
                        for part, folders_to_find in self.part_folders.items():
                            if self.has_folders_in(root_mnt_path, folders_to_find):
                                mnt_parts[part] = partition_device
                                break

        log.info('Found mnt_parts:' + log.pformat(mnt_parts))
        if self.boot == 'bios' and 'gpt_disk' in boot_partition:           
            stg = storage()
            stg.prepare_bios_grub_boot_on_gpt_disk({boot_partition['gpt_disk']})
        if 'root' in mnt_parts:
            with MountContext(mnt_parts['root'], root_mnt_path,
                                dry=not cli.os_disk_operable):
                return self._stage2(mnt_parts, root_mnt_path)

        raise DiskPartitionNotFound("Root disk not found.")

    @staticmethod
    def has_folders_in(folder_path, folder_names):
        return all(
            any((folder_name == folder.name)
                for folder in folder_path.iterdir())
            for folder_name in folder_names
        )

    @staticmethod
    def _disable_selinux(root_mnt_path):
        selinux_config_path = root_mnt_path / "etc/selinux/config"
        if selinux_config_path.exists():
            log.info("Found selinux config: {}"
                 .format(selinux_config_path))
            with (selinux_config_path).open(mode="r") as f:
                selinux_output = f.read()
            out=""
            for line in selinux_output.split("\n"):
                if line.startswith('SELINUX='):
                    log.info("Changed from \"{}\" to \"{}\"".format(line,"SELINUX=disabled"))
                    out+=("SELINUX=Disabled\n")
                else:
                    out+=("{}\n").format(line);
            with (selinux_config_path).open(mode="wb") as f:
                f.write(out.encode("utf-8"))

    @staticmethod
    def copytree(src, dst, symlinks=False, ignore=None):
        for item in os.listdir(src):
            s = os.path.join(src, item)
            d = os.path.join(dst, item)
            if os.path.isdir(s):
                shutil.copytree(s, d, symlinks, ignore)
            else:
                shutil.copy2(s, d)

    @staticmethod
    def grub_install(root_mnt_path, boot_partition ):
        if 'gpt_disk' in boot_partition:
            grub_lib= root_mnt_path / "usr/lib/grub"
            if os.path.exists(grub_lib):
                if not os.path.exists(grub_lib / "i386-pc"):
                    os.makedirs(str(grub_lib / "i386-pc"), exist_ok=True)
                    SystemDiskConverter.copytree("/usr/lib/grub/i386-pc", grub_lib / "i386-pc")
                if not os.path.exists(grub_lib/ "x86_64-efi"):
                    os.makedirs(str(grub_lib / "x86_64-efi"), exist_ok=True)
                    SystemDiskConverter.copytree(Path("/usr/lib/grub/x86_64-efi"), grub_lib/ "x86_64-efi" )

            log.info("Generate grub install script")
            script_name = "chrootgrub.sh"
            target_folder = root_mnt_path / "linux2vjob"
            tpl_file_path = (Path(__file__).resolve().parent.parent /
                             "scripts/chrootdracut.template")
            grub_commands = []   
            grub_commands.append("t=$({}-install --help | grep '\-\-target=' | wc -l)".format(boot_partition['grub_config']['version']))
            grub_commands.append("if [ $t -gt 0 ]; then")
            grub_commands.append( "{}-install {} --target=x86_64-efi".format(boot_partition['grub_config']['version'], boot_partition['gpt_disk']))
            grub_commands.append( "{}-install {} --target=i386-pc".format(boot_partition['grub_config']['version'], boot_partition['gpt_disk']))
            grub_commands.append("else")
            grub_commands.append( "{}-install {}".format(boot_partition['grub_config']['version'], boot_partition['gpt_disk']))
            grub_commands.append("fi")
            with (tpl_file_path).open(mode="r") as f:
                dracut_script_template = f.read()
                dracut_script_content = dracut_script_template.format(
                    dracut_commands="\n".join(grub_commands))

            log.info("grub_install_script_content : {}".format(dracut_script_content))

            cli.mkdir(target_folder, quiet=True)
            with (target_folder / script_name).open(mode="wb") as f:
                log.info("Write chroot grub install script")
                f.write(dracut_script_content.encode("utf-8"))

            log.info("Chroot to run grub install script")
            grub_script = "chroot {} /bin/sh {}".format(
                root_mnt_path, target_folder.name + "/" + script_name)

            log.info("grub_script : {}".format(grub_script))
            cli.run(grub_script,dry=not cli.os_disk_operable, quiet_log=True )

            os.remove(target_folder / script_name)
            shutil.rmtree( target_folder, True )

    @staticmethod
    def chrootdracut(root_mnt_path,
                     initramfs_str="initramfs",
                     initramfs_type="initramfs",
                     kernel_versions=[], drivers_injection="", drivers_verify="", drivers_count = 0, nocompress =False, img_extension='.img'):
        if drivers_injection=="":
            log.info("Skip drivers injection")
            return
        log.info("Generate chroot dracut script")
        script_name = "chrootdracut.sh"
        target_folder = root_mnt_path / "linux2vjob"

        tpl_file_path = (Path(__file__).resolve().parent.parent /
                         "scripts/chrootdracut.template")

        dracut_commands = []
        if initramfs_type == "initramfs":
            if len(drivers_verify) == 0 :
                for kernel_version in kernel_versions:               
                    command = "dracut {drivers_injection} -f " \
                    "/boot/{initramfs_str}-{kernel_version}{img_extension} {kernel_version}" \
                    .format(drivers_injection=drivers_injection, initramfs_str=initramfs_str,
                            img_extension=img_extension, kernel_version=kernel_version)
                    if nocompress :
                        command += " --no-compress"
                    dracut_commands.append(command)
            else:
                for kernel_version in kernel_versions:
                    dracut_commands.append("d=$(find /lib/modules/{kernel_version} -iname \'*.ko*\' | egrep -i \'{drivers_verify}\' -o | sort -u | wc -l)"
                                           .format(drivers_verify=drivers_verify,kernel_version=kernel_version))
                    dracut_commands.append("r=$(lsinitrd /boot/{initramfs_str}-{kernel_version}{img_extension} | egrep -i \'{drivers_verify}\' -o | sort -u | wc -l)"
                                           .format(drivers_verify=drivers_verify,initramfs_str=initramfs_str,kernel_version=kernel_version, img_extension=img_extension))
                    dracut_commands.append("if [ $d -ge {drivers_count} ] && [ $r -ne {drivers_count} ]; then".format(drivers_count=drivers_count) )
                    command = "dracut {drivers_injection} -f " \
                    "/boot/{initramfs_str}-{kernel_version}{img_extension} {kernel_version}" \
                    .format(drivers_injection=drivers_injection, initramfs_str=initramfs_str,
                            img_extension=img_extension, kernel_version=kernel_version)
                    if nocompress :
                        command += " --no-compress"
                    dracut_commands.append(command)
                    dracut_commands.append("fi")

        elif initramfs_type == "initrd":
            if len(drivers_verify) == 0 :
                dracut_commands.append("if [ -f /etc/lvm/lvm.conf ]; then\n    export root_lvm=1\nfi")
                for kernel_version in kernel_versions:
                    command = "mkinitrd {drivers_injection} -f " \
                        "/boot/{initramfs_str}-{kernel_version}{img_extension} {kernel_version}" \
                        .format(drivers_injection=drivers_injection, initramfs_str=initramfs_str,
                                kernel_version=kernel_version,img_extension=img_extension)
                    if nocompress :
                        command += " --nocompress"
                    dracut_commands.append(command)
            else:
                dracut_commands.append("if [ -f /etc/lvm/lvm.conf ]; then\n    export root_lvm=1\nfi")
                for kernel_version in kernel_versions:
                    dracut_commands.append("d=$(find /lib/modules/{kernel_version} -iname \'*.ko*\' | egrep -i \'{drivers_verify}\' -o | sort -u | wc -l)"
                                           .format(drivers_verify=drivers_verify,kernel_version=kernel_version))
                    dracut_commands.append("r=$(zcat /boot/{initramfs_str}-{kernel_version}{img_extension} | cpio -itv | egrep -i \'{drivers_verify}\' -o | sort -u | wc -l)"
                                           .format(drivers_verify=drivers_verify,initramfs_str=initramfs_str,kernel_version=kernel_version,img_extension=img_extension))
                    dracut_commands.append("s=$(cat /boot/{initramfs_str}-{kernel_version}{img_extension} | cpio -itv | egrep -i \'{drivers_verify}\' -o | sort -u | wc -l)"
                                           .format(drivers_verify=drivers_verify,initramfs_str=initramfs_str,kernel_version=kernel_version,img_extension=img_extension))
                    dracut_commands.append("if [ $d -ge {drivers_count} ] && [ $r -ne {drivers_count} ] && [ $s -ne {drivers_count} ]; then".format(drivers_count=drivers_count) )
                    command = "mkinitrd {drivers_injection} -f " \
                        "/boot/{initramfs_str}-{kernel_version}{img_extension} {kernel_version}" \
                        .format(drivers_injection=drivers_injection, initramfs_str=initramfs_str,
                                kernel_version=kernel_version,img_extension=img_extension)
                    if nocompress :
                        command += " --nocompress"
                    dracut_commands.append(command)
                    dracut_commands.append("fi")

        with (tpl_file_path).open(mode="r") as f:
            dracut_script_template = f.read()
            dracut_script_content = dracut_script_template.format(
                dracut_commands="\n".join(dracut_commands))

        log.info("dracut_script_content : {}".format(dracut_script_content))

        cli.mkdir(target_folder, quiet=True)
        with (target_folder / script_name).open(mode="wb") as f:
            log.info("Write chroot dracut script")
            f.write(dracut_script_content.encode("utf-8"))

        log.info("Chroot to run dracut script")
        dracut_script = "chroot {} /bin/sh {}".format(
            root_mnt_path, target_folder.name + "/" + script_name)

        log.info("dracut_script : {}".format(dracut_script))
        cli.run(dracut_script,dry=not cli.os_disk_operable, quiet_log=True )

        os.remove(target_folder / script_name)
        shutil.rmtree( target_folder, True )

    @staticmethod
    def inject_virtio_drivers(etc_folder):
        dracut_with_virtio = (
            b'# additional kernel modules to the default\n'
            b'add_drivers+="virtio virtio_blk virtio_net virtio_pci"\n'
        )
        with (etc_folder / "dracut.conf").open(mode="ab") as f:
            f.write(dracut_with_virtio)

    def mountedpath(self, given_path):
        if not str(given_path).startswith(str(self.os_root_path)):
            if str(given_path).startswith("/"):
                return self.os_root_path / str(given_path)[1:]
            return self.os_root_path / given_path
        return Path(given_path)

    def _find_first_entry(self, bootcfg_path, initramfs_str, kernel_versions):
        with bootcfg_path.open('r',encoding='utf-8', errors='ignore') as f:
            content = f.read()
        new_kernel_versions = []
        for line in content.split('\n'):
            if len(new_kernel_versions) > 0: 
                break
            for kernel_version in kernel_versions:
                 if line.find("/{initramfs_str}-{kernel_version}".format( initramfs_str= initramfs_str, kernel_version=kernel_version)) > 0: 
                     log.info("found first entry : {} in {}".format(kernel_version, line))
                     new_kernel_versions.append(kernel_version)
                     break
        return new_kernel_versions

    def _convert_kernel(self, root_mnt_path, os_family, os_version, bootcfg_path):
        mounted_backup_folder = self.mountedpath(self.backup_folder)
        cli.mkdir(mounted_backup_folder, quiet=True)
        log.info("os_family : {}, os_version: {}".format(os_family, os_version))
        tpl = get_template('kernel_version', os_family, os_version)
        log.info("kernel_version templete : initramfs_str: {}, kmodules_folder: {}, filter_str: {} ".format(str(tpl['initramfs_str']), str(tpl['kmodules_folder']), str(tpl['filter_str'])))
        drivers_injection_tpl = dict()
        drivers_verify_tpl = dict()
        if self.type == '' :
            manufacturer, product = get_hardware_info()
            drivers_injection_tpl = get_drivers_injection(manufacturer)
            drivers_verify_tpl = get_drivers_verify(manufacturer)
            log.info("manufacturer : {}, product: {}, drivers_injection: {}".format(manufacturer,product,str(drivers_injection_tpl[str(tpl['initramfs_str'])])))
        else:
            drivers_injection_tpl = get_drivers_injection(self.type)
            drivers_verify_tpl = get_drivers_verify(self.type)
            log.info("conversion type : {}, drivers_injection: {}".format(self.type,str(drivers_injection_tpl[str(tpl['initramfs_str'])])))

        kernel_versions = self._find_buildable_kernel_versions(
            self.mountedpath(self.os_root_path) / tpl['kmodules_folder'],
            filter_str=tpl['filter_str']
        )
        log.info("Found kernel_versions : {}".format(kernel_versions))
        self.chrootdracut(root_mnt_path
                , initramfs_str=tpl['initramfs_str']
                , initramfs_type=tpl['initramfs_type']
                , kernel_versions= self._find_first_entry(bootcfg_path, tpl['initramfs_str'], kernel_versions)
                , drivers_injection=str(drivers_injection_tpl[str(tpl['injection_type'])])
                , drivers_verify = str(drivers_verify_tpl['{}_drivers'.format(str(tpl['initramfs_type']))])
                , drivers_count = int(drivers_verify_tpl['{}_count'.format(str(tpl['initramfs_type']))]) 
                , nocompress = self.nocompress
                , img_extension = tpl['extension']
                )
        

    @staticmethod
    def _find_buildable_kernel_versions(folder_path, filter_str=""):
        kernel_versions = seq(folder_path.iterdir()) \
            .filter(lambda k: k.is_dir()) \
            .map(lambda d: d.name) \
            .filter(lambda kname: (not filter_str) or (filter_str in kname))
        return kernel_versions

    def _convert_network_settings(self,
                                  root_mnt_path, distribution, os_version):
        """
        2. Inject runonce.d and setup next boot converter.
        """
        mounted_backup_folder = self.mountedpath(self.backup_folder)
        cli.mkdir(mounted_backup_folder, quiet=True)
        self.inject_runonced(root_mnt_path / "etc", distribution, os_version)
        self.networkconverter = NetworkConverter(
            os_version=os_version,
            os_family=distribution,
            os_root_path=root_mnt_path,
            backup_folder=mounted_backup_folder
        )
        self.networkconverter.reset_network_interfaces()
        #runonce_scripts = self.networkconverter.generate_runonce_scripts()
        #host_py_content = self.HOST_PY_TEMPLATE.format(os_version=os_version,
        #                                               os_family=distribution)
        #self.inject_runonced_script(
        #    root_mnt_path / "etc", runonce_scripts, host_py_content)
        for order, party in enumerate(thirdparty.find_existed_parties()):
            arch = self.find_host_arch_from_etc(root_mnt_path / "etc")
            actions = party.customize(
                root_mnt_path, distribution, os_version, arch)
            self.follow_up(actions, order)

    @staticmethod
    def find_host_arch_from_etc(mnt_etc):
        ld_so_conf_d_path = mnt_etc / "ld.so.conf.d"
        if not ld_so_conf_d_path.exists():
            return None

        for file in ld_so_conf_d_path.iterdir():
            if 'x86_64' in file.name:
                return 'x86_64'
        return 'i686'

    @classmethod
    def follow_up(cls, actions, order):
        if 'runonce_custom' in actions:
            action = actions['runonce_custom']
            sh_name = action.get('sh_name', '')
            sh_content = action['sh_content']
            mnt_etc_path = action['root_mnt_path'] / "etc"
            cls.write_runonced_script(
                mnt_etc_path,
                "9_{}_custom_{}.sh".format(order, sh_name),
                sh_content
            )

    @staticmethod
    def write_runonced_script(mnt_etc, script_name, content):
        log.info("Write runonced script: {}".format(script_name))
        filepath = mnt_etc / "runonce.d" / "run" / script_name
        with filepath.open("wb") as f:
            f.write(content.encode('utf-8'))
        cli.run("chmod +x {}".format(filepath), quiet_log=True)

    @staticmethod
    def inject_runonced_script(mnt_etc, scripts, host_py_content):
        log.info("Inject runonced network reset scripts")
        runonce_run_folder = mnt_etc / "runonce.d" / "run"
        for script in scripts:
            cli.copy(src=script,
                     dst=runonce_run_folder,
                     src_inside_linux2v=True)
            filename = Path(script).name
            if not filename.startswith('_'):
                cli.run("chmod +x {}".format(runonce_run_folder / filename))
        host_py_path = runonce_run_folder = runonce_run_folder / "_host.py"
        with host_py_path.open(mode="wb") as f:
            f.write(host_py_content.encode("utf-8"))

    @staticmethod
    def _read_file_contect(file_path):
        with(Path(file_path)).open(mode="r") as f:
            return f.read()

    def inject_runonced(self, etc_folder, distribution, os_version):
        log.info("Inject runonced")
        for folder_name in ("run", "ran", "bin"):
            folder = etc_folder / "runonce.d" / folder_name
            cli.mkdir(folder, quiet=True)

        runonce_bin_folder = etc_folder / "runonce.d" / "bin"
        runonce_run_folder = etc_folder / "runonce.d" / "run"

        cli.copy(src="scripts/runonced/runonce.sh",
                 dst=runonce_bin_folder,
                 src_inside_linux2v=True)
        os_family = get_family(distribution)
        if os_family == 'SuSE':
            rclocal_filepath = etc_folder / "init.d" / "boot.local"
        else:
            rclocal_filepath = etc_folder / "rc.local"

        new_rclocal = False
        if os.path.exists(rclocal_filepath):
            cli.copy( src= rclocal_filepath,  # backup /etc/rc.local
                        dst=self.mountedpath(self.backup_folder), quiet_log=False)
        else:
            new_rclocal = True
            log.info("new {}.".format(rclocal_filepath))
            cli.run("touch {}".format(rclocal_filepath))
            cli.run("chmod +x {}".format(rclocal_filepath))
            #cli.run("ln -sf {} {}/rc1.d/S999rc.local".format(rclocal_filepath, etc_folder))
            #cli.run("ln -sf {} {}/rc2.d/S999rc.local".format(rclocal_filepath, etc_folder))
            #cli.run("ln -sf {} {}/rc3.d/S999rc.local".format(rclocal_filepath, etc_folder))
            #cli.run("ln -sf {} {}/rc4.d/S999rc.local".format(rclocal_filepath, etc_folder))
            #cli.run("ln -sf {} {}/rc5.d/S999rc.local".format(rclocal_filepath, etc_folder))
            #cli.run("ln -sf {} {}/rc6.d/S999rc.local".format(rclocal_filepath, etc_folder))

        with rclocal_filepath.open(mode="rb") as f:
            exit_0_in_rclocal = b'exit 0' in f.read()
        if exit_0_in_rclocal:
            log.info("'exit 0' inside rc.local script.")
            cli.run("sed -i '/\/etc\/runonce.d\/bin\/runonce.sh/d' {}".format(
                rclocal_filepath))
            cli.run("sed -i '/^$/d' {}".format(rclocal_filepath))    
            runsh = ("\\n\\1\/etc\/runonce.d\/bin\/runonce.sh"
                        "\\n\\1exit 0\\n")
            cli.run("sed -i 's/^\\(\\s*\\)exit 0/{}/g' {}".format(
                runsh, rclocal_filepath))
        else:
            log.info("Append runonce.sh to rc.local script.")
            with rclocal_filepath.open(mode="ab") as f:
                if new_rclocal:
                    f.write(b"#!/bin/bash\n")
                f.write(self.rclocal_runonce)

        tpl_file_path = (Path(__file__).resolve().parent.parent /
                            "scripts/callback.template")

        with (tpl_file_path).open(mode="r") as f:
            callback_script_template = f.read()
        
        callback_script_content=("#!/bin/bash\ndeclare -a urls=(")

        if self.callbacks and len(self.callbacks) > 0 :    
            for callback in self.callbacks:
                callback_script_content+=( "\"{}\" ".format(callback))
        callback_script_content+=( ")\n")
        callback_script_content+=("timeout={}\n").format(self.timeout)
        mac_addresses=[]
        ip_addresses=[]
        submasks=[]
        submasks2=[]
        gateways=[]
        dns1=[]
        dns2=[]
        if Path("/root/network_infos.json").exists():
            net = json.loads(self._read_file_contect("/root/network_infos.json"))
            for _network in net["network_infos"] :
                mac_addresses.append(_network["mac_address"])
                if len(_network["ip_addresses"]) > 0 :
                    ip_addresses.append(_network["ip_addresses"][0])
                    submasks.append(_network["subnet_masks"][0])
                    submasks2.append(sum([bin(int(x)).count("1") for x in _network["subnet_masks"][0].split(".")]))
                else:
                    ip_addresses.append("")
                    submasks.append("")
                    submasks2.append("")

                if len(_network["gateways"]) > 0 :
                    gateways.append(_network["gateways"][0])
                else:
                    gateways.append("")

                if len(_network["dnss"]) > 0 :
                    dns1.append(_network["dnss"][0])
                else:
                    dns1.append("")
                if len(_network["dnss"]) > 1 :
                    dns2.append(_network["dnss"][1])
                else:
                    dns2.append("")
        callback_script_content+=( "\ndeclare -a mac_addresses=(")
        for mac in mac_addresses:
            callback_script_content+=( "\"{}\" ".format(mac.lower()))          
        callback_script_content+=( ")\n")
        callback_script_content+=( "\ndeclare -a ip_addresses=(")
        for ip in ip_addresses:
            callback_script_content+=( "\"{}\" ".format(ip))           
        callback_script_content+=( ")\n")
        callback_script_content+=( "\ndeclare -a submasks=(") 
        for mask in submasks:
            callback_script_content+=( "\"{}\" ".format(mask))                
        callback_script_content+=( ")\n")
        callback_script_content+=( "\ndeclare -a submasks2=(")
        for mask in submasks2:
            callback_script_content+=( "\"{}\" ".format(mask))             
        callback_script_content+=( ")\n")
        callback_script_content+=( "\ndeclare -a gateways=(")
        for getway in gateways:
            callback_script_content+=( "\"{}\" ".format(getway))                  
        callback_script_content+=( ")\n")
        callback_script_content+=( "\ndeclare -a dns1=(")   
        for dns in dns1:
            callback_script_content+=( "\"{}\" ".format(dns))            
        callback_script_content+=( ")\n")
        #callback_script_content+=( "\ndeclare -a dns2=(")
        #for dns in dns2:
        #    callback_script_content+=( "\"{}\" ".format(dns))              
        #callback_script_content+=( ")\n")

        callback_script_content+=(callback_script_template)
        callback_script_content+=( "\n")

        with (runonce_run_folder / "_callback.sh").open(mode="wb") as f:
            f.write(callback_script_content.encode("utf-8"))
       
        #log.info("_callback.sh: \n {}".format(callback_script_content))
        cli.copy(src="scripts/remove_excluded.sh",
                 dst=runonce_run_folder,
                 src_inside_linux2v=True)
        cli.run("chmod +x {}".format(runonce_run_folder / "remove_excluded.sh")) 

        if Path("/root/post_script.tar.gz").exists():
            log.info("Try to unpack \"/root/post_script.tar.gz\".")
            cli.copy(src="scripts/post_script.sh",
                 dst=runonce_run_folder,
                 src_inside_linux2v=True)
            cli.copy(src="/root/post_script.tar.gz",
                 dst=runonce_bin_folder,
                 src_inside_linux2v=True)
            cli.run("chmod +x {}".format(runonce_run_folder / "post_script.sh"))        
            cli.run("tar xvzf /root/post_script.tar.gz -C {} --strip-components 1".format(runonce_run_folder))
            if not Path(Path(runonce_run_folder)/ "run.sh").exists():
                cli.run("tar xvf /root/post_script.tar.gz -C {} --strip-components 1".format(runonce_run_folder))
            if not Path(Path(runonce_run_folder)/ "run.sh").exists():
                cli.run("tar xvzf /root/post_script.tar.gz -C {}".format(runonce_run_folder))
            if not Path(Path(runonce_run_folder)/ "run.sh").exists():
                cli.run("tar xvf /root/post_script.tar.gz -C {}".format(runonce_run_folder))
            if not Path(Path(runonce_run_folder)/ "run.sh").exists():
                cli.run("unzip /root/post_script.tar.gz -d {}".format(runonce_run_folder))
            if Path(Path(runonce_run_folder)/ "run.sh").exists():
                os.remove("/root/post_script.tar.gz")
                log.info("chmod +x  {}".format(runonce_run_folder / "run.sh"))
                cli.run("chmod +x {}".format(runonce_run_folder / "run.sh"))
        
        log.info("chmod +x  {}".format(runonce_run_folder / "_callback.sh"))
        cli.run("chmod +x {}".format(runonce_run_folder / "_callback.sh"))
        log.info("chmod +x  {}".format(runonce_run_folder / "runonce.sh"))
        cli.run("chmod +x {}".format(runonce_bin_folder / "runonce.sh"))
        log.info("chmod +x  {}".format(rclocal_filepath.resolve()))
        cli.run("chmod +x {}".format(rclocal_filepath.resolve()))

    def find_boot_partition(self, disk_devices, boot_mnt_path):
        disk_partitions = find_disk_partitions()
        log.info("Found {} disk_partitions:{}".format(
            len(disk_partitions), log.pformat(disk_partitions)))
        bootable_parts = find_bootable_partitions()
        log.info("Found {} bootable partitions:{}".format(
            len(bootable_parts), log.pformat(bootable_parts)))
        stg = storage()
        for disk_guid, device in disk_devices.items():
            log.debug("Find partition on {}:{}".format(disk_guid, device))
            dev= None
            for d in stg.disks:
                if d.path == device :
                    dev = d
                    break
            if not dev or dev.is_mbr_partition_style():
                boot_partition_device = ""
                for part in disk_partitions[device]:
                    for parts in bootable_parts:
                        if part == parts :
                            boot_partition_device = part
                            break
                    if len(boot_partition_device) :                  
                        with MountContext(boot_partition_device, boot_mnt_path,
                                  dry=not cli.os_disk_operable):
                            grub_config = self.find_convertable_grub_config(boot_mnt_path)
                            if grub_config:
                                FoundBootPartition = True;
                                log.info("Found boot partition {} and grub {}".format(
                                    boot_partition_device, grub_config))
                                return {'grub_config': grub_config,
                                        'guid': disk_guid,
                                        'device': boot_partition_device}
                if len(boot_partition_device) == 0 :
                    for part in disk_partitions[device]:                    
                        with MountContext(part, boot_mnt_path,
                                  dry=not cli.os_disk_operable):
                            grub_config = self.find_convertable_grub_config(boot_mnt_path)
                            if grub_config:
                                FoundBootPartition = True;
                                log.info("Found boot partition {} and grub {}".format(
                                    part, grub_config))
                                return {'grub_config': grub_config,
                                        'guid': disk_guid,
                                        'device': part}
            elif dev.is_gpt_partition_style():
                efi_sys_part = None
                boot_part = None
                boot_partition_devices = []
                for p in dev.parts:
                    if p.is_efi_system_partition() :
                        efi_sys_part = p.path
                    if p.is_bootable():
                        boot_part = p.path
                if not efi_sys_part and boot_part:
                    for p in dev.parts:
                        if p.is_efi_system_partition() or p.is_swap_partition() or p.is_lvm_partition() or p.is_bootable():
                            continue
                        else:
                            boot_partition_devices.append(p.path)
                    for boot_partition_device in boot_partition_devices:
                        log.info("mount boot part: {}".format(boot_partition_device))
                        with MountContext(boot_partition_device, boot_mnt_path,
                                  dry=not cli.os_disk_operable):
                            grub_config = self.find_convertable_grub_config(boot_mnt_path)
                            if grub_config:
                                FoundBootPartition = True;
                                log.info("Found boot partition {} and grub {}".format(
                                    boot_partition_device, grub_config))
                                return {'grub_config': grub_config,
                                        'guid': disk_guid,
                                        'device': boot_partition_device}
                elif efi_sys_part :
                    for p in dev.parts:
                        if p.is_efi_system_partition() or p.is_swap_partition() or p.is_lvm_partition() or p.is_bootable():
                            continue
                        else:
                            boot_partition_devices.append(p.path)
                    for boot_partition_device in boot_partition_devices:
                        log.info("mount boot part: {}".format(boot_partition_device))
                        with MountContext(boot_partition_device, boot_mnt_path,
                                    dry=not cli.os_disk_operable):
                            if Path(boot_mnt_path / 'efi').exists() :
                                log.info("mount efi part: {}".format(efi_sys_part))
                                with MountContext(efi_sys_part, boot_mnt_path / "efi",
                                        dry=not cli.os_disk_operable):
                                    grub_config = self.find_convertable_grub_config(boot_mnt_path, is_uefi = True)
                                    if grub_config:
                                        FoundBootPartition = True;
                                        log.info("Found boot partition {}, EFI partition {} and grub {}".format(
                                            boot_partition_device, efi_sys_part, grub_config))
                                        return {'grub_config': grub_config,
                                                'guid': disk_guid,
                                                'device': boot_partition_device,
                                                'efi': efi_sys_part,
                                                'gpt_disk': dev.path}
                            elif Path(boot_mnt_path / 'boot/efi').exists():
                                log.info("mount efi part: {}".format(efi_sys_part))
                                with MountContext(efi_sys_part, boot_mnt_path / "boot/efi",
                                        dry=not cli.os_disk_operable):
                                    grub_config = self.find_convertable_grub_config(boot_mnt_path, is_uefi = True)
                                    if grub_config:
                                        FoundBootPartition = True;
                                        log.info("Found boot partition {}, EFI partition {} and grub {}".format(
                                            boot_partition_device, efi_sys_part, grub_config))
                                        return {'grub_config': grub_config,
                                                'guid': disk_guid,
                                                'device': boot_partition_device,
                                                'efi': efi_sys_part,
                                                'gpt_disk': dev.path}
                            elif boot_part and ( Path(boot_mnt_path / 'grub').exists() or Path(boot_mnt_path / 'grub2').exists() ):
                                grub_config = self.find_convertable_grub_config(boot_mnt_path)
                                if grub_config:
                                    FoundBootPartition = True;
                                    log.info("Found boot partition {} and grub {}".format(
                                        boot_partition_device, grub_config))
                                    return {'grub_config': grub_config,
                                            'guid': disk_guid,
                                            'device': boot_partition_device}

    def find_convertable_grub_config(self, folder_path, is_uefi=False):
        log.info("Detect {}".format(folder_path))
        if (folder_path / "boot").exists():
            folder_path = folder_path / "boot"

        grub_config = self.find_grub_config(folder_path, is_uefi)
        if not grub_config:
            return None

        log.info("Found grub config {!r}".format(grub_config))
        kernels_found = self.find_installed_kernels(folder_path)
        if not kernels_found:
            log.debug(kernels_found)
            log.warning("No kernel found in the mounted disk")
            return None

        # "best" means the one with the highest version which supports virtio.
        best_version = self.select_best_kernel_version(kernels_found)

        # Does best_kernel support virtio?
        if not self.has_virtio_supported(best_version):
            log.warning("No virtio support in selected kernel")
            return None

        log.info("Disk has bootable kernel with virtio supported")
        return grub_config

    def find_grub_config(self, folder_path, is_uefi=False):
        locations = {  # under /boot/
            "grub2": [
                "grub2/grub.cfg",
                "grub2/grub.conf",
            ],
            "grub": [
                "grub/menu.lst",
                "grub/grub.cfg",
                "grub/grub.conf",
            ]
        }
        if is_uefi:
            locations["grub2"].insert(0, "efi/EFI/redhat/grub.cfg")
            locations["grub2"].insert(0, "efi/EFI/centos/grub.cfg")

        for grub_version in ("grub2", "grub"):
            for loc in locations[grub_version]:
                if (folder_path / loc).exists():
                    return {'version': grub_version, 'bootcfg': loc}

    def find_installed_kernels(self, folder_path):
        kernels = seq(folder_path.iterdir()) \
            .filter(lambda k: k.is_file()) \
            .filter(lambda k: k.name.startswith('vmlinuz')) \
            .filter(lambda k: len(k.name) > len('vmlinuz')) \
            .filter(lambda k: 'rescue' not in k.name) \
            .map(lambda k: k.name)
        return kernels

    @staticmethod
    def extract_versions_from_kernels(kernels):
        log.info("{}".format(kernels))
        return seq(kernels) \
            .map(REGEX['vmlinuz_version'].match) \
            .filter(lambda m: m.groups) \
            .map(lambda r: r.groups()[0])

    def select_best_kernel_version(self, kernels):
        # "best" means the one with the highest version which supports virtio.
        versions = self.extract_versions_from_kernels(kernels)
        return sort_versions(versions)[-1]

    def has_virtio_supported(self, kernel_version):
        sorted_versions = sort_versions(
            [kernel_version, MIN_VERSION_CONTAINS_VIRTIO])
        is_kernel_version_bigger_than_min_version = \
            sorted_versions[-1] == kernel_version
        return is_kernel_version_bigger_than_min_version

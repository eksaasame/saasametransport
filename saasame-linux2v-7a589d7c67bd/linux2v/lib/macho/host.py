import os
import platform

# from ..lib import log

from .. import cli
from .. import log

def get_hostname():
    return cli.run('hostname')


def get_common_os_name():
    name = {
        'posix': 'LINUX',
        'nt': 'WINDOWS',
    }.get(os.name)
    if name == 'LINUX' and platform.mac_ver()[0]:
        return 'MAC'
    else:
        return name


def sort_versions(versions):
    return sorted((v.replace('-', '.') for v in versions),
                  key=lambda s: [int(sv) for sv in s.split('.')])

def get_hardware_info():
    manufacturer='Unknown'
    product='Unknown'
    output = cli.run('dmidecode --type system', quiet_log=True)
    log.info("dmidecode output: \n {}\n".format(output));
    for line in output.split("\n"):
        lline=line.strip()
        if lline.startswith('Manufacturer:'):
            key, var = lline.partition(":")[::2]
            var=var.strip()
            if var.startswith ('OpenStack'):
                manufacturer='OpenStack'
            elif var.startswith ('VMWare'):
                manufacturer='VMWare'
            elif var.startswith ('Microsoft'):
                manufacturer='Microsoft'
            elif var.startswith ('Alibaba'):
                manufacturer='OpenStack'
            elif var.startswith ('Xen'):
                manufacturer='Xen'                   
        if lline.startswith('Product Name:'):
            key, var = lline.partition(":")[::2]
            product=var.strip()
            break
    return manufacturer, product

def get_os_info():
    manufacturer, version, platform_id = platform.linux_distribution()
    if not manufacturer:
        os_name = platform.system()
        if os_name == "Darwin":
            manufacturer = "Apple Inc."
            version = platform.mac_ver()[0]
    return manufacturer, version


def running_on_linux():
    return get_common_os_name() == 'LINUX'


def _parse_redhat_release(content):
    labels =  content.split("\n")[0].split("(")
    if len(labels) > 1:
        *distribution, release, major = labels[0].split()
        minor = 1
        if major == '4':
            values = labels[1].strip("()").strip()
            minor = values[len(values)-1]       
        version = "{}.{}".format(major,minor)     
        return distribution[0], release, version, labels[1].strip("()")
    else:
        *distribution, release, version, label = content.split("\n")[0].split()
        return distribution[0], release, version, label.strip("()")

def _parse_ubuntu_release(content):
    for line in content.split("\n"):
        if line.startswith("VERSION_ID="):
            label = line.split('"')[1]
            if ',' in label:
                versionrelease = label.split(",")[0]
            elif '(' in label:
                versionrelease = label.split("(")[0]
            else:
                versionrelease = label
            versionrelease = versionrelease.split()
            version = versionrelease[0]
            release = versionrelease[1] if len(versionrelease) > 1 else None
        elif line.startswith("NAME="):
            distribution = line.split('"')[1]
    return distribution, release, version, label.strip()

def _parse_SuSE_release(content):
    distribution = "SuSE"
    for line in content.split("\n"):
        if line.startswith("SUSE"):
            distribution = "SuSE"
        elif line.startswith("openSUSE"):
            distribution = "openSUSE"
        elif line.startswith("VERSION ="):
            version = line.split('=')[1].strip()
        elif line.startswith("PATCHLEVEL ="):
            patch = line.split('=')[1].strip()
            version = "{}.{}".format(version,patch)
    return distribution, "", version, ""

def _parse_lsb_release(content):
    for line in content.split("\n"):
        if line.startswith("DISTRIB_ID="):
            distribution = line.split('=')[1]
        elif line.startswith("DISTRIB_RELEASE="):
            versionrelease = line.split('=')[1]
            versionrelease = versionrelease.split()
            version = versionrelease[0]
            release = versionrelease[1] if len(versionrelease) > 1 else None
        elif line.startswith("DISTRIB_DESCRIPTION="):
            label = line.split('"')[1]
    return distribution, release, version, label.strip()

def detect_os_version_in_etc_folder(etc_folder_path):
    if (etc_folder_path / "redhat-release").exists():  # RedHat or CentOS
        with (etc_folder_path / "redhat-release").open() as f:
            content = f.read()
            distribution, x, version, label = _parse_redhat_release(content)
    elif (etc_folder_path / "os-release").exists():  # Ubuntu
        with (etc_folder_path / "os-release").open() as f:
            content = f.read()
            distribution, x, version, label = _parse_ubuntu_release(content)
    elif (etc_folder_path / "SuSE-release").exists():  # SuSE
        with (etc_folder_path / "SuSE-release").open() as f:
            content = f.read()
            distribution, x, version, label = _parse_SuSE_release(content)    
    elif (etc_folder_path / "lsb-release").exists():
         with (etc_folder_path / "lsb-release").open() as f:
            content = f.read()
            distribution, x, version, label = _parse_lsb_release(content)
    else:
        distribution = version = None
    return distribution, version

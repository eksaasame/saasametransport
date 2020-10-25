import fnmatch
from ..exceptions import OSUnknown

os_families = {
    'RedHat': ['Red', 'CentOS', 'RedHat'],
    'Ubuntu': ['Debian', 'Ubuntu'],
    'SuSE': ['SLES', 'SUSE', 'SuSE']
}

os_distributions = {
    'RedHat': ['Red','RedHat'],
    'CentOS': ['CentOS'],
    'Ubuntu': ['Ubuntu'],
    'Debian': ['Debian'],
    'SuSE': ['SLES', 'SUSE', 'SuSE']
}

def get_family(dist):
    for family, dist_prefixes in os_families.items():
        for dist_prefix in dist_prefixes:
            if dist.startswith(dist_prefix):
                return family
    return None

def get_dist_display(dist):
    for distribution, dist_prefixes in os_distributions.items():
        for dist_prefix in dist_prefixes:
            if dist.startswith(dist_prefix):
                return distribution
    return None

def get_template(setting, os_family, os_version,
                 none_if_no_matched=False,
                 given_templates=None):
    templates = given_templates or default_templates
    os_versions = \
        templates[setting].get(get_family(os_family)) \
        or templates[setting].get(os_family) \
        or templates[setting].get('*') or {}

    for version in reversed(sorted(os_versions)):
        if fnmatch.fnmatch(os_version, version):
            return os_versions[version]

    if none_if_no_matched:
        return None

    msg = "Given os_version {} and os_family {} not found in templates." \
        .format(os_version, os_family)
    raise OSUnknown(msg)

default_templates = {
    'kernel_version': {
        'RedHat': {
            '*': {
                'initramfs_type': 'initramfs',
                'initramfs_str': 'initramfs',
                'injection_type': 'initramfs',
                'kmodules_folder': 'lib/modules',
                'filter_str': '.el',
                'extension' : '.img'
            },
            '7.*': {
                'initramfs_type': 'initramfs',
                'initramfs_str': 'initramfs',
                'injection_type': 'initramfs',
                'kmodules_folder': 'lib/modules',
                'filter_str': '.el7',
                'extension' : '.img'
            },
            '6.*': {
                'initramfs_type': 'initramfs',
                'initramfs_str': 'initramfs',
                'injection_type': 'initramfs',
                'kmodules_folder': 'lib/modules',
                'filter_str': '.el6',
                'extension' : '.img'
            },
            '5.*': {
                'initramfs_type': 'initrd',
                'initramfs_str': 'initrd',
                'injection_type': 'initrd',
                'kmodules_folder': 'lib/modules',
                'filter_str': '.el5',
                'extension' : '.img'
            },
            '4.*': {
                'initramfs_type': 'initrd',
                'initramfs_str': 'initrd',
                'injection_type': 'initrd_old',
                'kmodules_folder': 'lib/modules',
                'filter_str': '.EL',
                'extension' : '.img'
            }
        },
        'SuSE' : {
            '*':{
                'initramfs_type': 'initramfs',
                'initramfs_str': 'initrd',
                'injection_type': 'initramfs',
                'kmodules_folder': 'lib/modules',
                'filter_str': '-default',
                'extension' : ''
                },
            },
    },
    'eth': {
        'RedHat': {
            '*': {
                'folder': '/etc/sysconfig/network-scripts/',
                'glob': 'ifcfg-[ebW][tinr]*',
            }
        },
        'Ubuntu': {
            '*': {
                'folder': '/etc/network/',
                'glob': 'interfaces',
            }
        },
        'SuSE': {
            '*': {
                'folder': '/etc/sysconfig/network/',
                'glob': 'ifcfg-[ebW][tinr]*',
            }
        }
    },
    'networkrules': {
        '*': {
            '*': {
                'folder': '/etc/udev/rules.d',
                'glob': '70-persistent-net.rules',
            }
        }
    },
    'resolvconf': {
        'RedHat': {
            '*': {
                'target': "/etc/sysconfig/network"
            }
        },
        'SuSE': {
            '*': {
                'target': "/etc/sysconfig/network"
            }
        },
    },
    'network': {
        'RedHat': {
            '*': {
                'target': "/etc/sysconfig/network"
            }
        },
        'SuSE': {
            '*': {
                'target': "/etc/sysconfig/network"
            }
        },
    }
}

default_templates['bootup_network_interfaces'] = {
    'Ubuntu': {
        '*': {
            'file': 'etc/network/interfaces',
            'content': """
# Inplace network interfaces for Ubuntu booting up, by linux2v
# The loopback network interface
auto lo
iface lo inet loopback

# The primary network interface
auto eth0
iface eth0 inet dhcp
"""
        }
    }
}
default_templates['grub_console'] = {
    'Ubuntu': {
        '*': {
            'file': 'etc/default/grub',
            'line_prefix': "GRUB_CMDLINE_LINUX_DEFAULT=",
        }
    }
}

default_templates['drivers_injection'] = {
    'Unknown':{
        'initramfs': '',
        'initrd': '',
        'initrd_old': ''
    },
    'OpenStack':{
        'initramfs': '--add-drivers \"virtio virtio_blk virtio_net virtio_pci\"',
        'initrd': '--with virtio_pci --with virtio_blk --with virtio_net --with virtio',
        'initrd_old': '--with virtio_pci --with virtio_blk --with virtio_net --with virtio'
    },
    'VMWare':{
        'initramfs': '--add-drivers \"mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi\"',
        'initrd': '--preload mptbase --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi',
        'initrd_old': '--preload mptbase --with mptscsi --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi'
    },
    'VMWare_LSI_SAS':{
        'initramfs': '--add-drivers \"mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi\"',
        'initrd': '--preload mptbase --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi',
        'initrd_old': '--preload mptbase --with mptscsi --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi'
    },
    'VMWare_LSI_Parallel':{
        'initramfs': '--add-drivers \"mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi\"',
        'initrd': '--preload mptbase --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi',
        'initrd_old': '--preload mptbase --with mptscsi --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi'
    },
    'VMWare_Paravirtual':{
        'initramfs': '--add-drivers \"mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi\"',
        'initrd': '--preload mptbase --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi',
        'initrd_old': '--preload mptbase --with mptscsi --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi'
    },
    'Microsoft':{
        'initramfs': '--add-drivers \"hv_vmbus hv_netvsc hv_storvsc\"',
        'initrd': '--preload hv_vmbus --preload hv_storvsc --with hv_netvsc',
        'initrd_old': '--preload hv_vmbus --preload hv_storvsc --with hv_netvsc'
    },
    'Xen':{
        'initramfs': '--add-drivers \"xen:vbd xen:vif\"',
        'initrd': '--preload xen-platform-pci --with=xen-vbd --with=xen-balloon --with=xen-vnif',
        'initrd_old': '--preload xen-platform-pci --with=xen-vbd --with=xen-balloon --with=xen-vnif'
    }
}

default_templates['drivers_verify'] = {
    'Unknown':{
        'initramfs_drivers': '',
        'initramfs_count': 0,
        'initrd_drivers': '',
        'initrd_count': 0
    },
    'OpenStack':{
        'initramfs_drivers': 'virtio.ko|virtio_pci.ko|virtio_blk.ko',
        'initramfs_count': 3,
        'initrd_drivers': 'virtio.ko|virtio_pci.ko|virtio_blk.ko',
        'initrd_count': 3
    },
    'VMWare':{
        'initramfs_drivers': 'mptbase.ko|mptscsih.ko|mptspi.ko|mptsas.ko|vmw_pvscsi.ko',
        'initramfs_count': 5,
        'initrd_drivers': 'mptbase.ko|mptscsih.ko|mptfc.ko|mptspi.ko|mptsas.ko|vmw_pvscsi.ko',
        'initrd_count': 6
    },
    'VMWare_LSI_SAS':{
        'initramfs_drivers': 'mptbase.ko|mptscsih.ko|mptspi.ko|mptsas.ko',
        'initramfs_count': 4,
        'initrd_drivers': 'mptbase.ko|mptscsih.ko|mptfc.ko|mptspi.ko|mptsas.ko',
        'initrd_count': 5
    },
    'VMWare_LSI_Parallel':{
        'initramfs_drivers': 'mptbase.ko|mptscsih.ko|mptspi.ko',
        'initramfs_count': 3,
        'initrd_drivers': 'mptbase.ko|mptscsih.ko|mptspi.ko',
        'initrd_count': 3
    },
    'VMWare_Paravirtual':{
        'initramfs_drivers': 'vmw_pvscsi.ko',
        'initramfs_count': 1,
        'initrd_drivers': 'vmw_pvscsi.ko',
        'initrd_count': 1
    },
    'Microsoft':{
        'initramfs_drivers': 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko',
        'initramfs_count': 3,
        'initrd_drivers': 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko',
        'initrd_count': 3
    },
    'Xen':{
        'initramfs_drivers': 'xen-blkfront.ko|xen-netfront.ko',
        'initramfs_count': 2,
        'initrd_drivers': 'xen-vbd.ko|xen-vnif.ko|xen-balloon.ko|xen-platform-pci.ko',
        'initrd_count': 4
    }
}

def get_drivers_injection(manufacturer):
    return default_templates['drivers_injection'].get(manufacturer)

def get_drivers_verify(manufacturer):
    return default_templates['drivers_verify'].get(manufacturer)

# aliases
#for setting in default_templates:
#    if 'CentOS' in default_templates[setting]:
#        default_templates[setting]['Red'] = \
#            default_templates[setting]['CentOS']

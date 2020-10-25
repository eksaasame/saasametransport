from ..lib import log
from ..spec import (
    PartitionStyle
)


def lsblk():
    # log.debug("lsblk()")
    return """NAME                MAJ:MIN RM  SIZE RO TYPE MOUNTPOINT
vda                 252:0    0   11G  0 disk 
├─vda1              252:1    0  500M  0 part /boot
└─vda2              252:2    0  9.5G  0 part 
  ├─ssmllncher-root 253:0    0  8.5G  0 lvm  /
  └─ssmllncher-swap 253:1    0    1G  0 lvm  [SWAP]
vdb                 252:0    0   11G  0 disk 
├─vdb1              252:1    0  500M  0 part /boot
└─vdb2              252:2    0  9.5G  0 part 
  ├─vg_centos6forprotect-lv_root 253:0    0  8.5G  0 lvm  /
  └─vg_centos6forprotect-lv_swap 253:1    0    1G  0 lvm  [SWAP]
vdh                 252:112  0    4G  0 disk 
├─vdh1              252:113  0  500M  0 part 
└─vdh2              252:114  0  2.5G  0 part 
  ├─centos-swap     253:2    0  308M  0 lvm  
  └─centos-root     253:3    0  2.2G  0 lvm  
vdi                 252:128  0    4G  0 disk 
├─vdi1              252:129  0  500M  0 part 
└─vdi2              252:130  0  2.5G  0 part 
"""


def ls_l_dev_disks_by_id():
    # log.debug("ls_l_dev_disks_by_id()")
    return """total 0
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 dm-name-ssmllncher-root -> ../../dm-0
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 dm-name-ssmllncher-swap -> ../../dm-1
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 dm-uuid-LVM-opufdfoiGwgfllPoYDjmvaVh1Q9WFQppnx5GBc0VflY9hdKtlYvkcprPttY75FuG -> ../../dm-0
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 dm-uuid-LVM-opufdfoiGwgfllPoYDjmvaVh1Q9WFQppUWGXf23Lco1bz6M1tQZHj4f1bEYnzVg7 -> ../../dm-1
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 lvm-pv-uuid-KDzrNk-0bD0-oYlM-lYbv-gB6W-1jYH-jDfYqh -> ../../vda2
lrwxrwxrwx. 1 root root  9 Aug 15 03:02 virtio-e1de1a6d-cc55-477a-9 -> ../../vda
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 virtio-e1de1a6d-cc55-477a-9-part1 -> ../../vda1
lrwxrwxrwx. 1 root root 10 Aug 15 03:02 virtio-e1de1a6d-cc55-477a-9-part2 -> ../../vda2"""


def get_partition_style():
    log.debug("get_partition_style()")
    return PartitionStyle.MBR


def cat_os_release_output():
    log.debug("cat_os_release_output")
    return """NAME="Ubuntu"
VERSION="12.04.5 LTS, Precise Pangolin"
ID=ubuntu
ID_LIKE=debian
PRETTY_NAME="Ubuntu precise (12.04.5 LTS)"
VERSION_ID="12.04"
"""

def cat_os_release_output():
    log.debug("cat_os_release_output")
    return """NAME="Ubuntu"
VERSION="16.04 LTS (Xenial Xerus)"
ID=ubuntu
ID_LIKE=debian
PRETTY_NAME="Ubuntu 16.04 LTS"
VERSION_ID="16.04"
HOME_URL="http://www.ubuntu.com/"
SUPPORT_URL="http://help.ubuntu.com/"
BUG_REPORT_URL="http://bugs.launchpad.net/ubuntu/"
UBUNTU_CODENAME=xenial
"""


def cat_redhat_release_output():
    log.debug("cat_redhat_release_output()")
    "CentOS Linux release 7.2.1511 (Core)"
    return "CentOS release 6.7 (Final)\n"


def ip_link_show_output():
    log.debug("ip_link_show_output()")
    return """1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN mode DEFAULT
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
2: eth0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP mode DEFAULT qlen 1000
    link/ether 52:54:00:13:d1:47 brd ff:ff:ff:ff:ff:fff8caa09cef
3: eno50332416: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN mode DEFAULT qlen 1000
    link/ether 00:0c:29:02:4d:1b brd ff:ff:ff:ff:ff:ff
4: eno67111680: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc mq state DOWN mode DEFAULT qlen 1000
    link/ether 00:0c:29:02:4d:25 brd ff:ff:ff:ff:ff:ff
"""


def ifcfg_eth_output():
    "cat /etc/sysconfig/network-scripts/ifcfg-eth0"
    log.debug("ifcfg_eth_output()")
    return """TYPE=Ethernet
BOOTPROTO=dhcp
DEFROUTE=yes
PEERDNS=yes
PEERROUTES=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_PEERDNS=yes
IPV6_PEERROUTES=yes
IPV6_FAILURE_FATAL=no
NAME=eth0
UUID=af481803-45e1-48f2-bab1-eda93b90541d
DEVICE=eth0
ONBOOT=yes
"""


def lvm_pvscan():
    log.debug("lvm_pvscan()")
    return """PV /dev/vda2   VG ssmllncher   lvm2 [9.51 GiB / 40.00 MiB free]
PV /dev/vdb2   VG centos   lvm2 [2.51 GiB / 20.00 MiB free]
Total: 1 [9.51 GiB] / in use: 1 [9.51 GiB] / in no VG: 0 [0   ]"""


def blks_dict_output():
    # log.debug("blks_dict_output()")
    return """/dev/vda1: UUID="8c4aafdf-8da3-4f6b-94b9-5e9f6037153a" TYPE="xfs"
/dev/vda2: UUID="KDzrNk-0bD0-oYlM-lYbv-gB6W-1jYH-jDfYqh" TYPE="LVM2_member"
/dev/mapper/centos-root: UUID="ef3c9170-d7b1-493f-8e0c-04c9a9ff7672" TYPE="xfs"
/dev/mapper/centos-swap: UUID="b78de2c4-b074-4fff-8d55-5e63a1c18291" TYPE="swap"
/dev/vdb1: UUID="6e57e927-5cba-4e3d-8804-c3bdcb18943c" TYPE="ext4"
/dev/vdb2: UUID="iM7FV3-yqSY-bdFd-dQcI-Dlsg-PSPy-dRYk0h" TYPE="LVM2_member"
/dev/mapper/vg_centos6forprotect-lv_root: UUID="066d7ef0-a154-42a2-98f6-9cf510858661" TYPE="ext4"
/dev/mapper/vg_centos6forprotect-lv_swap: UUID="6550525a-4dbb-40c5-bbfa-0fb28c4a6430" TYPE="swap"
/dev/sr0: LABEL="RHEL/5.8 x86_64 DVD" TYPE="iso9660"
/dev/sda4: UUID="40D3-6012" TYPE="vfat"
/dev/sda1: LABEL="/boot" UUID="8a0a4069-2bbd-488a-bcf5-92b0c45f600b" TYPE="ext3" SEC_TYPE="ext2"
/dev/vdh1: UUID="368fd021-f681-49ad-b1ad-4f08eb7d70ec" TYPE="xfs"
/dev/vdh2: UUID="PFckci-ZUFJ-TYgo-1mVD-Nudi-Ba0u-jwk2Ak" TYPE="LVM2_member"
/dev/mapper/vdh1: UUID="368fd021-f681-49ad-b1ad-4f08eb7d70ec" TYPE="xfs"
/dev/mapper/vdh2: UUID="PFckci-ZUFJ-TYgo-1mVD-Nudi-Ba0u-jwk2Ak" TYPE="LVM2_member"
/dev/mapper/centos-swap: UUID="d13e7692-ae9a-4072-a01f-3c78c67b8c5a" TYPE="swa│·
p"                                                                            │·
/dev/mapper/centos-root: UUID="9347ef77-ca43-4101-85df-c89565da4b06" TYPE="xfs│·
/dev/vdj1: UUID="368fd021-f681-49ad-b1ad-4f08eb7d70ej" TYPE="vfat"
/dev/vdj2: UUID="PFckci-ZUFJ-TYgo-1mVD-Nudi-Ba0u-jwk2Aj" TYPE="xfs"
/dev/VolGroup00/LogVol01: UUID="77cce7ec-291d-4b67-bd8d-491ca158daa8" TYPE="ext3"
/dev/VolGroup00/LogVol00: TYPE="swap"
/dev/cdrom: LABEL="RHEL/5.8 x86_64 DVD" TYPE="iso9660"
/dev/loop0: UUID="2016-05-06-17-07-05-00" LABEL="20160505_091723" TYPE="iso9660"
/dev/vdj: PTTYPE="dos"
"""


def pvdisplay_output():
    log.debug("pvdisplay_output()")
    return """--- Physical volume ---
  PV Name               /dev/sda2
  VG Name               vg_cent6lvm
  PV Size               2.51 GiB / not usable 3.00 MiB
  Allocatable           yes (but full)
  PE Size               4.00 MiB
  Total PE              642
  Free PE               0
  Allocated PE          642
  PV UUID               yPbUFN-7TCh-Wt2Y-otnC-fTOo-hjKa-d7D6j9

  --- Physical volume ---
  PV Name               /dev/vda2
  VG Name               centos
  PV Size               9.51 GiB / not usable 3.00 MiB
  Allocatable           yes
  PE Size               4.00 MiB
  Total PE              2434
  Free PE               10
  Allocated PE          2424
  PV UUID               KDzrNk-0bD0-oYlM-lYbv-gB6W-1jYH-jDfYqh
"""


def new_disk_device():
    log.debug("new_disk_device()")
    return "/dev/vdb"


def fdisk_output():
    log.debug("fdisk_output()")
    return """
Disk /dev/vda: 21.5 GB, 21474836480 bytes, 41943040 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk label type: dos
Disk identifier: 0x00017c42

   Device Boot      Start         End      Blocks   Id  System
/dev/vda1   *        2048     1026047      512000   83  Linux
/dev/vda2         1026048    20971519     9972736   8e  Linux LVM

Disk /dev/mapper/centos-root: 9093 MB, 9093251072 bytes, 17760256 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes


Disk /dev/mapper/centos-swap: 1073 MB, 1073741824 bytes, 2097152 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes


Disk /dev/vdb: 9663 MB, 9663676416 bytes, 18874368 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
Disk label type: dos
Disk identifier: 0x000f2200

   Device Boot      Start         End      Blocks   Id  System
/dev/vdb1   *        2048     1026047      512000   83  Linux
/dev/vdb2         1026048    16777215     7875584   8e  Linux LVM

Disk /dev/mapper/vg_centos6forprotect-lv_root: 7205 MB, 7205814272 bytes, 14073856 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes


Disk /dev/mapper/vg_centos6forprotect-lv_swap: 855 MB, 855638016 bytes, 1671168 sectors
Units = sectors of 1 * 512 = 512 bytes
Sector size (logical/physical): 512 bytes / 512 bytes
I/O size (minimum/optimal): 512 bytes / 512 bytes
"""
# flake8: noqa

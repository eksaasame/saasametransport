1.7.17 (2017-12-13)
==================
- Support Linux Packer

1.7.16 (2017-12-4)
==================
- Support Multiple VG on the same disk

1.7.15 (2017-11-10)
==================
- Support GPT disk conversion 

1.7.13 (2017-10-14)
==================
- Fix Azure conversion issue
- Fix Export Image conversion issue

1.7.9 (2017-10-14)
==================
- Rewrite reset-network-settings script by BASH (Fix the different python version issue on target OS.)
- Add function to disable selinux for alibaba
- Change the drivers loading order for xen
- Improve the code to check the drive modules before dracut/mkinitrd
- First version for Azure support

1.7.6 (2017-7-10)
==================
- Fix conversion issue for LVM disk on AWS
- First Version to Support AWS conversion 

*Redhat 5.0,5.1,5.2 and CentOS 5.0,5.1,5.2
Need to copy the kmod-xenpvrpm corresponding to your hardware architecture and kernel variant to your guest operating system.
https://www.centos.org/docs/5/html/5.2/Virtualization/sect-Virtualization-Installation_and_Configuration_of_Para_virtualized_Drivers_on_Red_Hat_Enterprise_Linux_5.html

1.7.5 (2017-7-4)
==================
- Fix missing rc.local issue to support ubuntu 17.04
- Fix enumerate_disks issue
- Improve find boot partition function
- Improve the code to parser the blkid output
- Fix convert Chinese version issue
- Improve the command logic to rescan scsi
- Support vmware lsi logic parallel for linux launcher
- Improve mkinitrd for vmware driver support

1.7.2 (2017-6-13)
==================
- Support GPT disk
- Support LinuxLauncher can be installed on non-lvm environment
- Fix find boot partition issue

1.7.1 (2017-6-8)
==================
- Support lsb-release OS info parser (Support Ubuntu 10.04.4)

1.7.0 (2017-6-5)
==================
- Optimized the exception output string
- Improve the script to avoid the drivers injection when the drivers were ready.

1.6.9 (2017-6-2)
==================
- Add 'convert' command line option

1.6.8 (2017-5-26)
==================
- Fixed TypeError("'NoneType' object is not iterable") when not assign callbacks for launcher job
- Support more version of CentOS

1.6.7 (2017-5-23)
==================
- Fixed UnboundLocalError("local variable 'boot_mnt_path' referenced before assignment",)
- Improve the conversion performance

1.6.6 (2017-5-17)
==================
- Increase Timeout time

1.6.5 (2017-4-12)
==================
- Implement thrift take_xrays and take_xray functions
- Support recovery callback operation

1.6.4 (2017-4-4)
==================
- Support CentOS 4.8, 5.5

1.6.3 (2017-3-30)
==================
- Support Oracle Linux 7.x

1.6.2 (2016-10-29)
==================

 - Support re-package RedHat 7 kernel with separated root partitions.
 - Improrve the logic of detecting root partition.
 - Fix AttributeError in LauncherService and LauncherJob.
 - Fix error in cleanup folders.

1.6.1 (2016-10-21)
==================

 - Fix duplicated LauncherJob status shown on management confusingly.

1.6.0 (2016-10-20)
==================

 - Support Ubuntu 16.04.
 - Show error trackback only in debug mode.
 - Pass per-commit tesitng on Bitbucket Pipelines.

1.5.2 (2016-10-03)
==================

 - Fix no job status update during execution.
 - Fix no LVM grub file path error for Ubuntu targets.

1.5.1 (2016-09-08)
==================

 - Fix duplicated job status update to management.
 - Fix wrong license labeled.
 - Re-installation will overwrite systemctl service configuration now.

1.5.0 (2016-08-31)
==================

 - Support same LVM vgname job.
 - Restrict to 1 worker queue.
 - Refactor Path object name to end with _path.
 - Separate job workspace to avoid mount mess.
 - Always clean up job workspace.
 - Fix missing request in LauncherService._remove_job().
 - Many logging tweaks.
 - Support sequence disks lum mapping anyway.
 - Force 1 huey task worker at a time.
 - Refactor blkids to blk to keep partition type information.
 - Refactor used disks_lun_mapping to disk_devices in linux launcher.
 - Refactor out unused SCSI functions.
 - Refactor out unused fdisk code and ls dev mapper parse.
 - Refactor filter_lvm_devices, update_lvm_devices and cleanup_lvm_devices.
 - Update alog version to 0.9.9.
 - Use /dev/mapper for partition device if it exists.
 - Raise error if boot disk not found.

1.4.1 (2016-08-23)
==================

 - Support Boost string format.
 - Update saasame.thrift and srift.py to 1.0.222.
 - Integrated testing with Bitbucket Pipelines.

1.4.0 (2016-08-18)
==================

 - Support Thrift Server running with SSL certfile (server.pem required).
 - Clean unused serial number disk type code from LauncherJob.
 - Use log.warning() instead of deprecated log.warn().

1.3.3 (2016-08-17)
==================

 - Support launcher job of serial number disk type.
 - Update saasame.thrift and srift.py to 1.0.215.

1.3.2 (2016-08-10)
==================

 - Handle timeout while looping over multiple management addresses.
 - Also disalbe NetworkManager for Ubuntu instances.

1.3.1 (2016-08-05)
==================

 - Ensure rc.local is executable.
 - Always disable NetworkManager.

1.3.0 (2016-08-04)
==================

 - Support generic third party templates.
 - Add install.sh in RPM linux.spec.

1.2.1 (2016-08-03)
==================

 - Extend thrid party commands and copy-files support.
 - Fix README to abstract [version].
 - Update ZEO to 4.3.0.

1.2.0 (2016-07-29)
==================

 - Support HTTPS connection and `verify_management` thrift call of Management.

1.1.0 (2016-07-25)
==================

 - Build RPM package.

 - Compile source code into binary.

1.0.1 (2016-07-06)
==================

Fix Root disk not found by not continue finding in disks_lun_mapping.

1.0.0 (2016-07-06)
==================

 - Support third-party customization (mainly for additional RPMs).

 - Support convert of Ubuntu 12.04 LTS - 14.04 LTS.

 - Repackage all installed CentOS 7 kernels with virtio support.

 - Reset network interface series number.

 - Fix OpenStack console freezes on Ubuntu 12.04 instance.

 - Use assigned volumes only instead of looping echo one.

 - Better command-line usage.

 - Better Error handle.

0.9.1 (2016-05-30)
==================

First working version targeting CentOS 6 and 7.

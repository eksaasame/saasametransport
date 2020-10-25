# Linux Launcher Trouble Shooting (debug)

## Enable debug mode in development or testing server (not on production)

 You can set these in `/etc/linux2v/linux2v.ini` on testing server:

```
[app:main]
...
on_debug = true
os_disk_operable = false
job_history_show_error_trackback = true
...
```

## Check application log

```
systemctl status linux2v
journalctl -u linux2v > linux2v.log
/usr/local/linux2v/bin/linux2v list_jobs /etc/linux2v/linux2v.ini --showhistories=True
```

In most case there will be error traceback if a job failed.
The last linux2v command is written in `linux2v/linux2v/scripts/maincmd.py`.

```
% linux2v
Usage: linux2v [OPTIONS] COMMAND [ARGS]...

Options:
  --help  Show this message and exit.

Commands:
  call                     Call service method by Thrift client
  cleanup_all_lvm_devices  Manuallly cleanup LVM devices
  discardjob               Mark a job status to [discarded].
  httpcall                 Call service method by HTTP client
  initdb                   Initialize database for setting collections
  list_jobs                List all received jobs
  resetdb                  Test the store
  runjob                   Quick run (usually with --rerun for failure...
  runzeo                   Run a ZEO daemon for development or testing
  serve                    Run thrift server
  showjob                  Get job detail
  test_delay_job           Test delay job
  test_store               Test the store
```


**list_jobs** output looks like:

```
% linux2v list_jobs /etc/linux2v/linux2v.ini
...
==============================
2016-08-30T03:20:36.458440+00:00 <LauncherJob> whiskey-island-seventeen-william (f718b8a4-b2d9-4f43-8d56-6d75b85e485b) [job_state_initialed] 2016-08-30T03:20:36.458440+00:00
[LauncherJob.detail]
{'boot_disk': '',
 'created_time': '2016-Aug-30 03:20:36.458440',
 'id': 'f718b8a4-b2d9-4f43-8d56-6d75b85e485b',
 'is_error': False,
 'replica_id': 'f718b8a4-b2d9-4f43-8d56-6d75b85e485b',
 'state': 2,
 'updated_time': '2016-Aug-30 03:20:36.459086'}
```

**showjob** output looks like:

```
% linux2v showjob f718b8a4-b2d9-4f43-8d56-6d75b85e485b /etc/linux2v/linux2v.ini
2016-08-30T03:20:36.458440+00:00 <LauncherJob> whiskey-island-seventeen-william (f718b8a4-b2d9-4f43-8d56-6d75b85e485b) [job_state_initialed] 2016-08-30T03:20:36.458440+00:00
[LauncherJob.detail]
{'boot_disk': '',
 'created_time': '2016-Aug-30 03:20:36.458440',
 'id': 'f718b8a4-b2d9-4f43-8d56-6d75b85e485b',
 'is_error': False,
 'replica_id': 'f718b8a4-b2d9-4f43-8d56-6d75b85e485b',
 'state': 2,
 'updated_time': '2016-Aug-30 03:20:36.459086'}
Job Histories: History: 2016-Aug-30 03:20:36.458767 2 LauncherJob
               State: [job_state_none] -> [job_state_initialed]
               History: 2016-Aug-30 03:20:36.458922 1073741824
               LauncherJob State: [job_state_initialed] ->
               [job_state_discard] History: 2016-Aug-30
               03:20:36.459086 2 LauncherJob State:
               [job_state_discard] -> [job_state_initialed]
```

## Gather Linux file system status according to error traceback

1 - Dig into current OS status:

```
dmesg
ps aux
ip a
netstat -tlnp
df -T
lsblk
blkid
pvscan
vgscan
lvscan
pvdisplay
vgdisplay
lvdisplay
lvm dumpconfig
fdisk  # poor LVM support
ls -l /dev/mapper
ls -l /dev/disk/by-id
```

2 - System log

```
/var/log/messages*
/var/log/cron*
/var/log/secure*
/var/log/redis/*
/var/log/yum.log
```

For `/var/log ...` it can be easier to just `tar zcf varlog.tar.gz /var/log` and send the whole tar file back.
import sys
import os
from pkg_resources import load_entry_point
from textwrap import TextWrapper
from pprint import pprint

import click
import transaction
import uuid
import time

from ..lib import log
from ..lib import cli as commandline
from ..lib.macho import storage
from ..launcher.service import Launcher
from ..launcher.job import LauncherJob
from ..hueytask import (
    run_launcher_job,
    discard_launcher_job,
    test_log
)
from collections import OrderedDict
from ..mgmt import call_mgmt_service
from ..spec import (
    thrift,
    JobState
)
from .base import bootstrap_in_console
from ..lib import utc
from ..lib.macho.storage import (
    rescan_scsi_hosts,
    find_current_boot_disk,
    find_disk_partitions,
    filter_lvm_devices,
    find_all_vgnames,
    update_lvm_device,
    cleanup_lvm_devices,
    cleanup_all_lvm_devices,
    get_virtio_device_by_disk,
    online_missing_vgvols,
)

from ..launcher.converter import SystemDiskConverter

session_id = "cmdemu2v_session_id"


@click.group()
def cli():
    pass

def _ensure_lvm_disk_devices_is_up(disk_devices):
    tolerable_update_times = 10
    log.debug("Ensure LVM devices up.")
    lvm_devices = list(filter_lvm_devices(disk_devices.values()))
    if not lvm_devices:
        log.info("No LVM devices.")
        return

    log.info("Ensure LVM devices {} up.".format(lvm_devices))
    all_vgnames = find_all_vgnames()
    not_found_lvm_devices = []
    for device in lvm_devices:
        device_vgname = all_vgnames.get(device)
        if device_vgname:
            log.info("Found vgname {} of {}.".format(device_vgname,
                                                        device))
            continue
        log.warning("Refresh LVM devices to find vgname of {}."
                    .format(device))
        update_times = 0
        while update_times < tolerable_update_times:
            update_times += 1
            force_cleanup = update_times > tolerable_update_times / 2
            storage.cleanup_all_lvm_devices( force = force_cleanup )
            storage.online_missing_vgvols()
            time.sleep(3)
            storage.update_lvm_device(device)
            time.sleep(3)
            all_vgnames = find_all_vgnames()
            device_vgname = all_vgnames.get(device)
            if device_vgname:
                log.info("Found vgnames {} of {} at update times {}."
                            .format(device_vgname, device, update_times))
                break
            time.sleep(update_times * 2)
        else:
            msg = "Hit tolerable refresh times ({}).".format(update_times)
            log.error(msg)
            not_found_lvm_devices.append(device)

@cli.command()
@click.argument('callbacks', default = '')
@click.argument('type', default = 'OpenStack')
@click.argument('timeout', default = 30)
@click.argument('nocompress', default = 1)
@click.argument('selinux', default = 1)
@click.argument('boot', default = 'bios')
@click.argument('shrink', default = 0)
def convert( callbacks, type, timeout, nocompress, selinux, boot, shrink):
    "convert disks for recovery"
    _disks=OrderedDict()
    result=False
    error=''
    count = 0
    while len(_disks) == 0 and count < 5 :
        ++count
        rescan_scsi_hosts()
        os.system("vgscan")
        _disk_partitions=find_disk_partitions()
        boot_disk = find_current_boot_disk()
        for disk_device, partitions in _disk_partitions.items():
            if  disk_device != boot_disk:
                _disks[str(uuid.uuid4())] = disk_device

    disks=OrderedDict()
    disk_partitions=find_disk_partitions( exclude_swap_empty_part = False )
    boot_disk = find_current_boot_disk()
    for disk_device, partitions in disk_partitions.items():
        if  disk_device != boot_disk:
            disks[str(uuid.uuid4())] = disk_device

    if len(disks) > 0 :
        platform = ""
        version = ""
        architecture = ""
        hostname = ""
        try:   
            _ensure_lvm_disk_devices_is_up(disks)
            _callbacks = []
            if len(callbacks) > 0 :
                _callbacks = callbacks.split(';')
            system_disk_converter = SystemDiskConverter(str(uuid.uuid4()), _callbacks, timeout, "", type, nocompress = nocompress > 0, selinux = selinux > 0, boot = boot)
            system_disk_converter.convert(disks)
            system_disk_converter.cleanup()
            platform = system_disk_converter.platform
            version = system_disk_converter.version
            architecture = system_disk_converter.architecture
            hostname = system_disk_converter.hostname
            result=True            
        except Exception as exc:
            error = "Failed : {}".format(exc)
        finally:
            storage.cleanup_all_lvm_devices( force = True )
            if result: 
                print("PLATFORM: {} {}".format(platform, version))
                print("ARCHITECTURE: {}".format(architecture))
                print("HOSTNAME: {}".format(hostname))
                print("Succeeded.")
            else:
                print(error)
    else:
        print("No disk for conversion.")

@cli.command()
@click.option('--rerun', default=False)
@click.argument('job_id')
@click.argument('configfile', type=click.Path())
def runjob(rerun, job_id, configfile):
    "Quick run (usually with --rerun for failure happened) a job."
    bootstrap_in_console(configfile)
    run_launcher_job(job_id, rerun=rerun)


@cli.command()
@click.option('--showhistories', default=True)
@click.argument('job_id')
@click.argument('configfile', type=click.Path())
def showjob(showhistories, job_id, configfile):
    "Get job detail"
    env, request = bootstrap_in_console(configfile)
    _show_job(request.dbroot, job_id, showhistories=showhistories)


@cli.command()
@click.option('--reason', default=None)
@click.argument('job_id')
@click.argument('configfile', type=click.Path())
def discardjob(reason, job_id, configfile):
    "Mark a job status to [discarded]."
    bootstrap_in_console(configfile)
    discard_launcher_job(job_id, reason)


@cli.command()
def cleanup_all_lvm_devices():
    "Manuallly cleanup LVM devices"
    storage.cleanup_all_lvm_devices()


@cli.command()
@click.option('--force', default=False)
@click.argument('configfile', type=click.Path())
def resetdb(force, configfile):
    "Test the store"
    if not force:
        import time
        log.warning("!!! Reset DB !!! (Ctrl-C to stop it)")
        time.sleep(3)
    else:
        log.warning("!!! Forced Reset DB !!!")
    env, request = bootstrap_in_console(configfile)
    request.store.force_reset_db(request.dbroot)
    transaction.commit()


def _show_job(dbroot, job_id, showhistories=False):
    job = LauncherJob.get(dbroot, job_id)
    if not all(getattr(job, needed_attr, None)
               for needed_attr in ('created', 'detail')):
        print("No needed info: {}".format(job))
        return

    print(job.created, job)
    if job.detail:
        print("[LauncherJob.detail]")
        detail_dict = thrift.obj_to_dict(job.detail)
        detail_dict.pop('histories')
        pprint(detail_dict)
        if showhistories and job.detail.histories:
            prefix = "Job Histories: "
            wrapper = TextWrapper(initial_indent=prefix, width=70,
                                  subsequent_indent=' '*len(prefix))
            msgs = ""
            for h in job.detail.histories:
                msg = "History: {} {} {}\n".format(
                    h.time, h.state, h.description)
                msgs += msg
            print(wrapper.fill(msgs))

    if job.mgmt_detail:
        print("[LauncherJob.mgmt_detail]")
        pprint(thrift.obj_to_dict(job.mgmt_detail))
    if job.execution_detail:
        print("[LauncherJob.execution_detail]")
        pprint(thrift.obj_to_dict(job.execution_detail))


def _list_jobs(dbroot, showhistories=False):
    for job_id in LauncherJob.resources(dbroot):
        print("===" * 10)
        _show_job(dbroot, job_id, showhistories)


@cli.command()
@click.option('--showhistories', default=False)
@click.argument('configfile', type=click.Path())
def list_jobs(showhistories, configfile):
    "List all received jobs"
    env, request = bootstrap_in_console(configfile)
    print("{}".format(request))
    _list_jobs(request.dbroot, showhistories=showhistories)


@cli.command()
@click.argument('configfile', type=click.Path())
@click.argument('replica_id', default=None)
def test_store(configfile, replica_id):
    "Test the store"
    env, request = bootstrap_in_console(configfile)
    store = request.store
    store.initdb(request.dbroot)
    dbroot = request.dbroot
    _list_jobs(dbroot)

    job = LauncherJob(dbroot, id=replica_id)
    job.save()
    transaction.commit()
    print(job.created, job)


@cli.command()
@click.argument('configfile', type=click.Path())
def test_delay_job(configfile):
    "Test delay job"
    env, request = bootstrap_in_console(configfile)
    dbroot = request.dbroot
    for job_id in LauncherJob.ids(dbroot):
        print("Choosed job id: {}".format(job_id))
        job = LauncherJob.get(dbroot, job_id)
        if job.state == JobState.initialized:
            print("Got {}".format(job))
            job.schedule()
            transaction.commit()
            break
    else:
        print("All done.")
        transaction.abort()


@cli.command()
@click.argument('configfile', type=click.Path())
def serve(configfile):
    "Run thrift server"
    env, request = bootstrap_in_console(configfile)
    launcher = Launcher(environ=env)
    launcher.run_server()


@cli.command()
@click.argument('configfile', type=click.Path())
def initdb(configfile):
    "Initialize database for setting collections"
    # TODO: gen client uuid at first
    env, request = bootstrap_in_console(configfile)
    request.store.initdb(request.dbroot)
    transaction.commit()


@cli.command()
@click.argument('configfile', type=click.Path())
def testjob(configfile):
    "Just testing command"
    test_log()


@cli.command()
@click.argument('service_method')
@click.argument('configfile', type=click.Path())
def call(configfile, service_method):
    "Call service method by Thrift client "
    env, request = bootstrap_in_console(configfile)
    launcher = Launcher(environ=env)
    testing_methods = [
        "ping",
        "get_host_detail",
        "get_service_list",
        "enumerate_disks",
    ]
    if service_method == "testing":
        for testing_method in testing_methods:
            print("CALL: -*- {} -*-".format(testing_method))
            res = launcher.call(testing_method)

            pprint(thrift.obj_to_dict(res) if type(res) not in (list, tuple)
                   else [thrift.obj_to_dict(r) for r in res])
            print()
    elif service_method == "emulate_mgmt_create_job":
        job_trigger = thrift.job_trigger()
        job_trigger.type = 1
        job_trigger.interval = 60

        mgmt_job_detail = thrift.create_job_detail()
        mgmt_job_detail.type = 4
        mgmt_job_detail.management_id = "39504D0D-4608-4052-82FB-51E5C69ABED0"
        mgmt_job_detail.mgmt_addr = ["192.168.31.191"]
        mgmt_job_detail.triggers = [job_trigger]
        pprint(mgmt_job_detail)
        job_id = "39504D0D-4608-4052-82FB-51E5C69ABED0"
        res = launcher.call("create_job_ex",
                            session_id, job_id, mgmt_job_detail)
        pprint(res)
    else:
        res = launcher.call(service_method)
        pprint(res)


@cli.command()
def runzeo():
    "Run a ZEO daemon for development or testing"
    sys.exit(
        load_entry_point('ZEO', 'console_scripts', 'runzeo')(
            ['-a', 'localhost:9992', '-f', 'data/localstore.fs'])
    )


@cli.command()
@click.option('--ini')
@click.option('--host', default="127.0.0.1")
@click.option('--port', default=80)
@click.argument('service_method')
@click.argument('args', nargs=-1)
def httpcall(ini, host, port, service_method, args):
    "Call service method by HTTP client "
    if ini:
        env, request = bootstrap_in_console(ini)
    if service_method == "update_launcher_job_state":
        assert ini
        job_id = args[0]
        job = LauncherJob.get(request.dbroot, job_id)
        args = [session_id, job.detail]
    res = call_mgmt_service(host, port, service_method, *args)
    pprint(res)
    if service_method == "get_launcher_job_create_detail":
        for a, b in res.disks_lun_mapping.items():
            print(a, b)


@cli.command()
@click.argument('args', nargs=-1)
def command(args):
    "Just testing command"
    res = commandline.run(" ".join(args))
    print(res)


def main():
    cli(obj={})


if __name__ == '__main__':
    main()

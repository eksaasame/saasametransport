# import json
import time
import uuid
import socket

import transaction
from persistent import Persistent

from ..mgmt import call_mgmt_service
from ..exceptions import (
    LVMDeviceNotFound,
    JobExecutionFailed
)
from ..lib import log
from ..lib import utc
from ..lib.strutil import format_booststr
from ..lib.macho.storage import (
    filter_lvm_devices,
    find_all_vgnames,
    update_lvm_device,
    cleanup_lvm_devices,
    cleanup_all_lvm_devices,
    get_virtio_device_by_disk,
    online_missing_vgvols,
)
from ..lib.humanhash import humanize
from ..spec import (
    thrift,
    JobState,
)
from ..hueytask import (
    run_launcher_job
)
from .converter import SystemDiskConverter

# tasklog = get_task_logger(__name__)

session_id = "session_id_2v1"


def clone_job_detail(job, for_mgmt_update: bool=False):
    if not getattr(job, 'detail'):
        return getattr(job, 'detail')

    new_detail = thrift.launcher_job_detail(id=job.id, replica_id=job.id)
    new_detail.state = job.detail.state
    new_detail.created_time = job.detail.created_time
    new_detail.updated_time = job.detail.updated_time
    if for_mgmt_update:
        histories = [h for h in job.detail.histories
                     if not h.format.startswith('LauncherJob State: [')]
        new_detail.histories = histories[-1:] if histories else []
    else:
        new_detail.histories = job.detail.histories
    return new_detail


class LauncherJob(Persistent):

    _resource_pool = "jobs"
    tolerable_update_times = 10

    def __init__(self, dbroot, id=None, mgmt_detail=None):
        self.id = id or self.generate_uuid_str()
        detail = thrift.launcher_job_detail(id=self.id, replica_id=id)
        self.human_id = humanize(self.id)
        self.mgmt_detail = mgmt_detail
        self.created = utc.now()
        self.updated = None
        detail.created_time = self.created.strftime(thrift.TIME_FORMAT)
        detail.histories = []
        self.detail = detail
        self._dbroot = dbroot
        self.execution_detail = None
        self.state = JobState.none
        self.found_lvm_devices = []

    def __str__(self):
        return "<{}> {} ({}) [{}] {}".format(
            self.__class__.__name__,
            self.human_id,
            self.id,
            self.state_name,
            self.created
        )

    @property
    def state_name(self):
        return JobState._VALUES_TO_NAMES[self.detail.state]

    @classmethod
    def resources(cls, dbroot):
        return dbroot[cls._resource_pool]

    def save(self):
        self.resources(self._dbroot)[self.id] = self
        self._p_changed = True

    @staticmethod
    def generate_uuid_str():
        return str(uuid.uuid4())

    @classmethod
    def get(cls, dbroot, job_id):
        return cls.resources(dbroot).get(job_id)

    def call_mgmt(self, method, session_id, *args):
        args_str = "{!r}".format(args)
        if len(args_str) > 40:
            args_str = args_str[:40]
        log.warning('CALL MGMT: {} {}'.format(method, args_str))
        mgmt_port = getattr(self.mgmt_detail, 'mgmt_port', None) or \
            thrift.MANAGEMENT_SERVICE_PORT
        scheme = 'https' \
            if getattr(self.mgmt_detail, 'is_ssl', False) else 'http'
        for mgmt_addr in self.mgmt_detail.mgmt_addr:
            try:
                result = call_mgmt_service(
                    mgmt_addr, mgmt_port, method, scheme, session_id, *args)
            except socket.error as error:
                log.warning(error)
            else:
                return result
        return None

    def get_execution_detail_from_mgmt(self, session_id):
        return self.call_mgmt("get_launcher_job_create_detail",
                              session_id, self.id)

    def update_state_to_mgmt(self, session_id):
        detail = clone_job_detail(self, for_mgmt_update=True)
        return self.call_mgmt("update_launcher_job_state", session_id, detail)

    def log_job_history(self, msg_fmt, msg_args=None):
        self.update_detail_state(self.detail.state, msg_fmt, msg_args)

    def update_detail_state(self, state,
                            msg_fmt="", msg_args=None, updated=None):
        self.updated = updated or utc.now()
        updated_time = self.updated.strftime(thrift.TIME_FORMAT)
        if not msg_fmt:
            if self.detail.state != state:
                msg_fmt = "LauncherJob State: [%1%] -> [%2%]"
                msg_args = [JobState._VALUES_TO_NAMES[self.detail.state],
                            JobState._VALUES_TO_NAMES[state]]
            else:
                log.warning("No state changed and no msg format given.")
                return

        msg_args = msg_args or []
        description = format_booststr(msg_fmt, msg_args)
        log.info(description)
        job_history = thrift.job_history(
            time=updated_time,
            state=state,
            description=description,
            format=msg_fmt,
            arguments=msg_args
        )
        self.detail.histories.append(job_history)
        self.detail.state = state
        self.detail.updated_time = updated_time

    @staticmethod
    def _map_disks_to_devices(execution_detail):
        detect_type = getattr(execution_detail, 'disk_detect_type',
                              thrift.disk_detect_type.LINUX_DEVICE_PATH)
        serial_number_used = detect_type and \
            detect_type == thrift.disk_detect_type.SERIAL_NUMBER
        return {
            disk_guid: (get_virtio_device_by_disk(device)
                        if serial_number_used else device)
            for disk_guid, device
            in execution_detail.disks_lun_mapping.items()
            if device.strip()
        }

    def _execute(self, request, log, rerun):
        self.log_job_history("Initializing a Launcher job.")
        if self.detail.state in (JobState.discarded, JobState.finished):
            if not rerun:
                msg = "No execution due to job state: {}".format(self)
                log.error(msg)
                self.log_job_history(msg)
                return

            self.update_detail_state(JobState.initialized,
                                     "Rerun job: %1%", [self])

        if self.detail.state != JobState.initialized:
            execution_detail = self.get_execution_detail_from_mgmt(session_id)
            log.debug("Got {}".format(execution_detail))
            if not execution_detail:
                msg = ("Failed to get the launcher job create detail "
                       "from management server.")
                log.error(msg)
                self.update_detail_state(JobState.discarded, msg)
                return

            self.execution_detail = execution_detail
            self.update_detail_state(JobState.initialized)

        self.update_detail_state(JobState.converting)
        self.update_state_to_mgmt(session_id)
        try:
            system_disk_converter = SystemDiskConverter(self.id, self.execution_detail.callbacks, self.execution_detail.callback_timeout)
            disk_devices = self._map_disks_to_devices(self.execution_detail)
            log.info("Start to convert the disk(s) {}".format(disk_devices))
            self.log_job_history("Conversion started.")
            self.update_state_to_mgmt(session_id)
            self._ensure_lvm_disk_devices_is_up(disk_devices)
            system_disk_converter.convert(disk_devices)
            msg = "Disk(s) conversion completed."
            self.update_detail_state(JobState.finished, msg)
            self.update_state_to_mgmt(session_id)
        except Exception as exc:
            error = "Disk(s) conversion failed : {}.".format(exc)
            self.log_job_history(error)
            self.update_state_to_mgmt(session_id)
        finally:
            system_disk_converter.cleanup()

    def _ensure_lvm_disk_devices_is_up(self, disk_devices):
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
                self.found_lvm_devices.append(device)
                continue

            log.warning("Refresh LVM devices to find vgname of {}."
                        .format(device))
            update_times = 0
            while update_times < self.tolerable_update_times:
                update_times += 1
                force_cleanup = update_times > self.tolerable_update_times / 2
                cleanup_all_lvm_devices(force=force_cleanup)
                online_missing_vgvols()
                time.sleep(3)
                update_lvm_device(device)
                time.sleep(3)
                all_vgnames = find_all_vgnames()
                device_vgname = all_vgnames.get(device)
                if device_vgname:
                    log.info("Found vgnames {} of {} at update times {}."
                             .format(device_vgname, device, update_times))
                    self.found_lvm_devices.append(device)
                    break
                time.sleep(update_times * 2)
            else:
                msg = "Hit tolerable refresh times ({}).".format(update_times)
                log.error(msg)
                not_found_lvm_devices.append(device)

        if not_found_lvm_devices:
            raise LVMDeviceNotFound("{}".format(not_found_lvm_devices))

    def _cleanup_lvm_devices(self, force=False):
        if self.found_lvm_devices:
            log.info("Cleanup found LVM devices {}"
                     .format(self.found_lvm_devices))
            cleanup_lvm_devices(self.found_lvm_devices, force=force)

    def execute(self, request, log=log, rerun=False):
        try:
            self._execute(request, log, rerun)            
        except Exception as exc:
            import traceback
            why = traceback.format_exc()
            subject = repr(exc)
            for line in reversed(why.split("\n")):
                if 'converter.py' in line:
                    subject += '\n' + line
                    break

            if getattr(self, 'history_show_error_trackback', None):
                self.update_detail_state(JobState.discarded, why)
            else:
                self.update_detail_state(JobState.discarded, subject)
            raise JobExecutionFailed(subject)

        finally:
            #self.update_state_to_mgmt(session_id)
            self.cleanup(force=True)

    def schedule(self):
        transaction.after_commit("success", run_launcher_job, self.id)

    def cleanup(self, force=False):
        self._cleanup_lvm_devices(force=force)

    def discard(self, force=False, msg_fmt=None, msg_args=None):
        # del self._dbroot[self._resource_pool][self.id]
        self.update_detail_state(JobState.discarded, msg_fmt, msg_args)
        self.cleanup(force=force)

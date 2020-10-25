from pathlib import Path
import zipfile
import transaction
from thriftpy import rpc
from pyramid.settings import asbool
import os
import time
import datetime
import threading
import multiprocessing

from ..spec import (
    thrift,
    OsType,
    OsVersionInfo,
    EnumerateDiskFilter,
    PhysicalMachineInfo,
    GUID_LENGTH_STRING,
)

from .. import version
from ..request import create_request_from_environ
from ..lib import (
    log,
    cli
)
from ..lib.macho import (
    storage,
    host
)
from ..db.store import Store
from ..mgmt import call_mgmt_service
from .job import LauncherJob


class LauncherCommonServiceMixin(thrift.common_service):

    info = thrift.service_info(id=thrift.LINUX_LAUNCHER_SERVICE,
                               version=version)
    def __init__(self):
        super().__init__()
        self.mutex = 0
        self.lock = Lock()

    @staticmethod
    def invalid_operation(op=None, why=None):
        op = op or thrift.error_codes.SAASAME_E_FAIL
        return thrift.invalid_operation(
            what_op=getattr(thrift.error_codes, op),
            why=why
        )

    def unsupported_operation(self):
        return self.invalid_operation(why="Unsupported operation")

    @staticmethod
    def _prepare_error(exc):
        log.exception(exc)
        import traceback
        why = traceback.format_exc()
        error = thrift.invalid_operation(
            what_op=thrift.error_codes.SAASAME_E_FAIL,
            why=why)
        return error

    def _prepare_return(self, result):
        if self.on_debug:
            log.info("RETR {}".format(log.pformat(result)))
        return result

    def ping(self):
        log.info("RECV ping")
        return self._prepare_return(self.info)

    def get_host_detail(self, session_id, filter):
        try:
            log.info("RECV get_host_detail (session_id: {}, filter: {})"
                     .format(session_id, filter))

            physical_machine_info = PhysicalMachineInfo()
            common_os_name = host.get_common_os_name()
            manufacturer, os_version = host.get_os_info()
            physical_machine_info.client_name = "{}-{}".format(
                common_os_name, host.get_hostname().strip()
            )
            physical_machine_info.os_version = OsVersionInfo(os_version)
            os_type = getattr(OsType, common_os_name, OsType.UNKNOWN)
            physical_machine_info.os_type = os_type

            disk_infos = list(self._find_disk_infos(filter))
            physical_machine_info.disk_infos = disk_infos
            # thrift.diff(thrift.physical_machine_info(),
            #             physical_machine_info, type_only=True)
            # return self._prepare_return(thrift.physical_machine_info())
            return self._prepare_return(physical_machine_info)

        except Exception as exc:
            raise self._prepare_error(exc)

    def get_service_list(self, session_id):
        try:
            log.info("RECV get_service_list (session_id: {})"
                     .format(session_id))
            return self._prepare_return([self.info])

        except Exception as exc:
            raise self._prepare_error(exc)

    def _find_disk_infos(self, filter_style):
        for d in storage.find_disks():
            if filter_style == EnumerateDiskFilter.UNINITIALIZED:
                continue

            yield d.export_to_thrift()

    def enumerate_disks(self, filter_style):
        try:
            log.info(
                "RECV enumerate_disks (filter_style: {})".format(filter_style))
            disk_infos = list(self._find_disk_infos(filter_style))
            return self._prepare_return(disk_infos)

        except Exception as exc:
            raise self._prepare_error(exc)
    
    def verify_carrier ( self, carrier, is_ssl ):
        return False

    @staticmethod
    def _take_xray():
        try:
            log.info("Enter _take_xray")
            output_zip_path = '/etc/linux2v/linux2v.zip'
            if os.path.exists(output_zip_path) :
                os.remove(output_zip_path)
            jobs = cli.run("/usr/local/linux2v/bin/linux2v list_jobs /etc/linux2v/linux2v.ini --showhistories=True", quiet_log = True)
            log.info("list_jobs result :{}".format(jobs))
            logs = cli.run("/usr/bin/journalctl -u linux2v", quiet_log = True)
            log.info("output logs result length :{}".format(len(logs)))     
            zipf = zipfile.ZipFile(output_zip_path, 'w', zipfile.ZIP_DEFLATED)
            if len(logs) > 0 :
                zipf.writestr("linux2v.log", logs)
            if len(jobs) > 0 :
                zipf.writestr("linux2v_jobs.txt", jobs)
            zipf.close()     
            with open(output_zip_path,'rb') as f:
                content = f.read()
                log.info("zip content length : {}".format(len(content)))
                return content
        except Exception as exc:
            raise self._prepare_error(exc)
        return content
    
    def take_xray(self):
        log.info("RECV take_xray")
        return self._take_xray()

    def take_xrays(self):
        log.info("RECV take_xrays")
        return self._take_xray()

    def create_mutex(self, session, timeout):    
        with (yield from self.lock):
            if self.mutex == 0 or self.mutex < time.mktime(datetime.datetime.utcnow().timetuple()):
                self.mutex = (time.mktime(datetime.datetime.utcnow().timetuple()) + timeout)
                return True
        return False    

    def delete_mutex(self, session):
        with (yield from self.lock):
            self.mutex = 0
        return True

class LauncherService(thrift.launcher_service, LauncherCommonServiceMixin):

    def __init__(self, store, environ, on_debug=False):
        super().__init__()
        self.store = store
        self.on_debug = on_debug
        self.environ = environ

    def verify_management(self, *args, **kwargs):
        try:
            request = create_request_from_environ(self.environ)
            return self._verify_management(request, *args, **kwargs)
        except Exception as exc:
            raise self._prepare_error(exc)
        finally:
            transaction.abort()

    def create_job_ex(self, *args, **kwargs):
        try:
            request = create_request_from_environ(self.environ)
            return self._create_job_ex(request, *args, **kwargs)
        except Exception as exc:
            raise self._prepare_error(exc)
        finally:
            transaction.abort()

    def remove_job(self, *args, **kwargs):
        try:
            request = create_request_from_environ(self.environ)
            return self._remove_job(request, *args, **kwargs)
        except Exception as exc:
            raise self._prepare_error(exc)
        finally:
            transaction.abort()

    def _verify_management(self, request, management, port, is_ssl):
        log.info("_verify_management '{} {} {}'."
                 .format(management, port, is_ssl))
        scheme = 'https' if is_ssl else 'http'
        call_mgmt_service(management, port, "check_snapshots", scheme,
                          "", GUID_LENGTH_STRING)
        return True

    def _remove_job(self, request, session_id, job_id):
        log.info("_remove_job '{}'.".format(job_id))
        request = create_request_from_environ(self.environ)
        job = LauncherJob.get(request.dbroot, job_id)
        if not job:
            why = "Job not found '{}'.".format(job_id)
            log.warning(why)
            return False

        msg_fmt = "LauncherJob discarded by the call of remove_job."
        job.discard(force=True, msg_fmt=msg_fmt)
        return True

    def _create_job_ex(self, request, session_id, job_id, mgmt_job_detail):
        """
        The entry point of creating new launcher job.
        """
        log.info("RECV create_job_ex\n"
                 "(session_id: {}, job_id: {}, mgmt_job_detail: {})"
                 .format(session_id, job_id, mgmt_job_detail))
        # TODO assert mgmt_job_detail.type == JobType.launcher
        if not mgmt_job_detail.triggers:
            why = "No scheduler trigger for job."
            raise self.invalid_operation("SAASAME_E_INVALID_ARG", why)

        job_existed = LauncherJob.get(request.dbroot, job_id)
        if job_existed:
            why = "Duplicated job '{}'.".format(job_id)
            raise self.invalid_operation("SAASAME_E_JOB_ID_DUPLICATED", why)

        # create job
        job = LauncherJob(request.dbroot,
                          id=job_id, mgmt_detail=mgmt_job_detail)
        if asbool(getattr(self, 'job_history_show_error_trackback', False)):
            job.history_show_error_trackback = True
        job.save()
        job.schedule()
        transaction.commit()
        return job.detail

    # def get_job(self, session_id, job_id):
    #     """
    #     Get job detail
    #     """
    #     request = create_request_from_environ(self.environ)
    #     dbroot = request.dbroot
    #     job = LauncherJob.get(dbroot, job_id)
    #     return job.detail

    # def create_job(self, session_id, job_detail):
    #     raise self.unsupported_operation()

    # def interrupt_job(self, session_id, job_id):
    #     raise self.unsupported_operation()

    # def resume_job(self, session_id, job_id):
    #     raise self.unsupported_operation()

    # def remove_job(self, session_id, job_id):
    #     raise self.unsupported_operation()

    # def list_jobs(self, session_id):
    #     """
    #     """
    #     pass

    # def terminate(self, session_id):
    #     raise self.unsupported_operation()

    # def running_job(self, session_id, job_id):
    #     """
    #     See if service has job running.
    #     """


class Launcher:

    def __init__(self, environ):
        settings = environ['registry'].settings
        cli.adopt_by(settings)
        store = Store(settings=settings)
        self.on_debug = asbool(settings.get('on_debug'))
        if self.on_debug:
            print("DEBUG is set ON")
        self.host = settings.get('launcher.host')
        self.port = settings.get('launcher.port', thrift.LAUNCHER_SERVICE_PORT)
        self.ssl_settings = {}
        certfile = settings.get('launcher.ssl.certfile')

        if certfile:
            if Path(certfile).exists():
                self.ssl_settings['certfile'] = certfile
            else:
                log.warning("Not existed SSL cerfile {}".format(certfile))
        else:
            log.warning("No SSL configuration. Running without SSL.")

        self.service = LauncherService(store,
                                       environ=environ,
                                       on_debug=self.on_debug)
        if asbool(settings.get('job_history_show_error_trackback')):
            self.service.job_history_show_error_trackback = True

    def __str__(self):
        return "{} (host={}, port={})".format(self.__class__.__name__,
                                              self.host, self.port)

    def make_thrift_server(self):
        if not self.ssl_settings:
            server = rpc.make_server(
                thrift.launcher_service,
                self.service,
                self.host, self.port
            )
        else:
            log.info("Run Thrift server in SSL socket.")
            server = rpc.make_server(
                thrift.launcher_service,
                self.service,
                self.host, self.port,
                certfile=self.ssl_settings['certfile']
            )
        return server

    def run_server(self):
        server = self.make_thrift_server()
        #storage.cleanup_all_lvm_devices(force=True)
        try:
            print("{} serving...".format(self))
            server.serve()
        except KeyboardInterrupt:
            log.info(" - Keyboard Interrupt - {}".format(self))

    def ping_server(self):
        with rpc.client_context(self.service, self.host, self.port) as client:
            res = client.ping()
            return res

    def call(self, service_method, *args, **kwargs):
        host = kwargs['host'] if 'host' in kwargs else self.host
        service = kwargs['service'] if 'service' in kwargs else self.service
        with rpc.client_context(service, host, self.port) as client:
            try:
                act = getattr(client, service_method)
                res = act(*args)
                return res
            except thrift.invalid_operation as ouch:
                print()
                print("What_op:", ouch.what_op)
                print("Why:", ouch.why)

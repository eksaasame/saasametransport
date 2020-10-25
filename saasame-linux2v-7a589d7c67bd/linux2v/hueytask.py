# import json

import transaction

from huey import RedisHuey

from .request import create_request_from_environ
from .lib import log
from .lib.strutil import format_booststr
# from .lib.macho.storage import cleanup_all_lvm_devices
huey = RedisHuey()


# tasklog = get_task_logger(__name__)

session_id = "session_id_2v1"


@huey.task()
def test_log():
    msg = "Hello test_log"
    for level in ('debug', 'info', 'warn'):
        # getattr(tasklog, level)("tasklog:" + level + " " + msg)
        getattr(log, level)("log:" + level + " " + msg)


@huey.task()
def run_launcher_job(job_id, rerun=False):
    from .launcher.job import LauncherJob
    try:
        log.warning("TASK run_launcher_job {}".format(job_id))
        request = create_request_from_environ(huey.pyramid_env)
        job = LauncherJob.get(request.dbroot, job_id)
        log.warning("{}.execute()".format(job))
        job.execute(request, log=log, rerun=rerun)
        log.warning("Commited (run_launcher_job)")
    except Exception as exc:
        log.exception(exc)
        raise
    finally:
        transaction.commit()


@huey.task()
def discard_launcher_job(job_id, reason=""):
    from .launcher.job import LauncherJob
    try:
        log.warning("TASK run_launcher_job {}".format(job_id))
        request = create_request_from_environ(huey.pyramid_env)
        job = LauncherJob.get(request.dbroot, job_id)
        due_to_reason = " due to {}".format(reason) if reason else ""
        msg_fmt = "Discard job %1%%2%."
        msg_args = [job, due_to_reason]
        log.warning(format_booststr(msg_fmt, msg_args))
        job.discard(force=True, msg_fmt=msg_fmt, msg_args=msg_args)
        log.warning("Commited (discardjob)")
    except Exception as exc:
        log.exception(exc)
        raise
    finally:
        update_job_state_to_mgmt.schedule(args=[job_id], delay=1)
        transaction.commit()


@huey.task()
def update_job_state_to_mgmt(job_id):
    from .launcher.job import LauncherJob
    msg = "TASK update_job_state_to_mgmt {}".format(job_id)
    try:
        request = create_request_from_environ(huey.pyramid_env)
        job = LauncherJob.get(request.dbroot, job_id)
        msg += " [{}]".format(job.state_name)
        log.info("{}.update_state_to_mgmt()".format(job))
        job.update_state_to_mgmt(session_id)
        transaction.commit()
        msg += " commited"
    finally:
        log.warning(msg)

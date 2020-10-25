import os
import shlex
import shutil
from pathlib import Path
from subprocess import Popen, PIPE

from pyramid.settings import asbool
from . import log


os_disk_operable = True


def adopt_by(settings):
    global os_disk_operable
    if not asbool(settings.get('os_disk_operable', True)):
        os_disk_operable = False


def linux2v_root_path():
    import linux2v
    return Path(linux2v.__path__[0])


class CliException(Exception):
    pass


def pretty_dststr(dst):
    dststr = str(dst)
    if '/jobs/' in dststr:
        dststr = dststr.split('-')[-1]
    return dststr


def copy(src=None, dst=None, src_inside_linux2v=False, quiet_log=False):
    src_repr = "(linux2v){}".format(src) if src_inside_linux2v else src
    if not quiet_log:
        log.info("COPY: {} {}".format(src_repr, pretty_dststr(dst)))
    if src_inside_linux2v:
        src_file_path = linux2v_root_path() / src
    else:
        src_file_path = Path(src)
    assert src_file_path.exists()
    shutil.copy(str(src_file_path), str(dst))


def move(src=None, dst=None, quiet_log=False):
    if not quiet_log:
        log.info("MOVE: {} {}".format(src, pretty_dststr(dst)))
    shutil.move(str(src), str(dst))


def mkdir(dirpath, quiet=False, quiet_log=False):
    if not quiet_log:
        log.debug("MAKEDIRS: {}".format(pretty_dststr(dirpath)))
    os.makedirs(str(dirpath), exist_ok=quiet)


def rmdir(dirpath, quiet=False, quiet_log=False):
    if not quiet_log:
        log.debug("RMDIR{}: {}".format('S' if quiet else '',
                                       pretty_dststr(dirpath)))
    os.removedirs(str(dirpath)) if quiet else os.rmdir(str(dirpath))


def create_flat_tarfile(src_folder, target_path):
    cmd = "{!s}/scripts/flattar.sh {!s} {!s}".format(
        linux2v_root_path(), src_folder, target_path)
    run(cmd)


def run(cmd, dry=False, quiet_log=False):
    args = shlex.split(cmd)    
    if not quiet_log:
        log.info("CMD({})".format(cmd))
    if dry:
        return ""
    try:
        process = Popen(args, stdout=PIPE, stderr=PIPE)
        output, errs = process.communicate()
        retcode = process.returncode
        if retcode < 0:
            log.warning("[{}], {}".format(-retcode, output))
        else:
            log.info("[{}], {}".format(errs.decode('utf-8'), output.decode('utf-8')))
        return output.decode('utf-8')

    except OSError as e:
        msg = "Execution failed: {}".format(e)
        log.error(msg)
        raise CliException(e)


run.exception = CliException

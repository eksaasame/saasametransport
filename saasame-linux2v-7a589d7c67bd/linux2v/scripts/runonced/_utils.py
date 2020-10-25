#!/usr/bin/env python
"""
NOTE: This script should be Python 2 compatible.
"""
import logging
import shlex
import os
import re
from subprocess import Popen, PIPE
from io import open


log = logging.getLogger(__name__)


class CliException(Exception):
    pass


def move(src=None, dst=None):
    log.debug("MOVE: {src} {dst}".format(src=src, dst=dst))
    # shutil.move(src, dst)


def run(cmd):
    args = shlex.split(cmd)
    try:
        output = None
        process = Popen(args, stdout=PIPE, stderr=PIPE)
        communication = process.communicate()
        output = communication[0]
        retcode = process.returncode
        if retcode < 0:
            msg = "Process return code {}".format(-retcode)
            log.debug(msg)
        return output.decode('utf-8')

    except OSError:
        msg = "Execution failed"
        log.error(msg)
        raise CliException(msg)


def write_to_etc_folder(filepath, content, settings=None):
    if os.path.exists(filepath):
        backup_folder = settings and settings.get('backup_folder')
        if backup_folder:
            move(filepath, backup_folder)
    print(os.path.abspath(filepath))
    write_to_file(os.path.abspath(filepath), content)
    print(read_from_file(filepath))


def write_to_file(filepath, content, encoding='utf-8'):
    fp = open(filepath, "wb")
    fp.write(content.encode(encoding))
    fp.close()


def read_from_file(filepath, encoding='utf-8'):
    fp = open(filepath, "rb")
    content = fp.read().decode(encoding)
    fp.close()
    return content


def _parse_interface_name(line):
    ethname = line.split()[1][:-1]
    return ethname


def _parse_link_mac_address(line):
    mac_address = line.split()[1]
    return mac_address


def parse_ip_link(output):
    re_eth_pattern = re.compile("^\d+: .*")
    re_link_pattern = re.compile("^\s*link/.*")
    interfaces = []
    for line in output.split("\n"):
        if re_eth_pattern.match(line):
            current_eth_pair = {'name': _parse_interface_name(line)}
        elif re_link_pattern.match(line):
            current_eth_pair['mac_address'] = _parse_link_mac_address(line)
            try:
                assert len(current_eth_pair.keys()) == 2
                assert all(current_eth_pair.values())
                interfaces.append(current_eth_pair)
            except AssertionError:
                log.exception("current_eth_pair is not paired.")
                current_eth_pair = {}
    return interfaces

#!/usr/bin/env python
"""
NOTE: This script should be Python 2 compatible.
"""
import logging

from _utils import run, parse_ip_link, CliException
from _templates import os_families
import _host as host

os_version = host.os_version
os_family = host.os_family
if os_family in os_families['RedHat']:
    import _reset_redhat_network as _reset_network
    from _2vsettings_redhat import settings
elif os_family in os_families['Ubuntu']:
    import _reset_ubuntu_network as _reset_network
    from _2vsettings_ubuntu import settings
	
log = logging.getLogger(__name__)

def reset(testing=False):
    logging.basicConfig(filename='linux2v.log',level=logging.DEBUG)
    output = run("ip link show")
    logging.info("ip link show output : \n{}".format(output))
    eths = [iface for iface in parse_ip_link(output)
            if iface['name'].startswith("e")]
    _reset_network.create_eth_networks(eths, settings)
    _reset_network.reset_firewall()

    finish_commands = settings.get('finish_commands') or []
    for cmd in finish_commands:
        try:
            run(cmd)
        except CliException:
            log.exception("run command {} error".format(cmd))


def main():
    reset()

if __name__ == "__main__":
    main()

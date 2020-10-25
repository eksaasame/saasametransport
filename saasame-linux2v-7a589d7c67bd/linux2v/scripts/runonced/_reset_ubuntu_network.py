# import uuid

from _utils import write_to_etc_folder, run


def create_eth_networks(eths, settings):
    interfaces_content = settings['lo']
    for eth in eths:
        eth_content = _create_eth_network_script_content(
            eth,
            settings,
            # mac_address=eth['mac_address'],
        )
        interfaces_content += eth_content
    filepath = '/etc/network/interfaces'
    write_to_etc_folder(filepath, interfaces_content, settings=settings)


def _create_eth_network_script_content(eth,
                                       settings,
                                       bootproto='dhcp',
                                       onboot=True,
                                       mac_address=None,
                                       with_nm_controlled=False,
                                       with_ipv6=False):
    # TODO
    eth_content = settings['eth'].format(device=eth['name'])
    return eth_content


def reset_firewall():
    run("ufw disable")

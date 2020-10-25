import uuid

from _utils import write_to_etc_folder, run


def create_eth_networks(eths, settings):
    for eth in eths:
        eth_content = _create_eth_network_script_content(
            eth,
            settings,
            mac_address=eth['mac_address'],
        )
        print(eth_content)
        filepath = settings['eth_folder'] + "/ifcfg-" + eth['name']
        print("Write to {path}".format(path=filepath))
        write_to_etc_folder(filepath, eth_content, settings=settings)


def _create_eth_network_script_content(eth,
                                       settings,
                                       bootproto='dhcp',
                                       onboot=True,
                                       mac_address=None,
                                       with_nm_controlled=False,
                                       with_ipv6=False):
    eth_content = settings['eth'].format(
        bootproto=bootproto,
        device=eth['name'],
        uuid=uuid.uuid4()
    )
    if mac_address:
        eth_content += settings['hwaddr'].format(
            mac_address=mac_address.upper())
    if onboot:
        eth_content += settings['onboot']
    if with_ipv6:
        eth_content += settings['ipv6_autoconf']
    if with_nm_controlled:
        eth_content += settings['nm_controlled']
    return eth_content


def reset_firewall():
    pass
    # run("systemctl disable firewalld")

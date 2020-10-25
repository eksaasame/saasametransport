settings = {
    'backup_folder': '/backup2v',
    'eth_folder': "/etc/sysconfig/network-scripts",
    'eth': """TYPE=Ethernet
BOOTPROTO={bootproto}
PEERDNS=yes
PEERROUTES=yes
IPV4_FAILURE_FATAL=no
NAME={device}
UUID={uuid}
DEVICE={device}
""",
    'onboot': "ONBOOT=yes\n",
    'nm_controlled': "NM_CONTROLLED=yes\n",
    'hwaddr': "HWADDR={mac_address}\n",
    'ipv6_autoconf': """IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_PEERDNS=yes
IPV6_PEERROUTES=yes
IPV6_FAILURE_FATAL=no
""",
    'finish_commands': [
        'service NetworkManager stop',
        'chkconfig NetworkManager off',
        'service network restart',
    ]
}

settings = {
    'lo': """
# The loopback network interface
auto lo
iface lo inet loopback
""",
    'eth': """
# The primary network interface
auto {device}
iface {device} inet dhcp
""",
    'finish_commands': [
        'sudo stop network-manager',
        'update-rc.d -f NetworkManager remove',
        'service networking restart',
        # 'update-grub'
    ]
}

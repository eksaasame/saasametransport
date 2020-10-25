#!/bin/sh
if [ -e "/etc/systemd/system/linux_packer.service" ]; then
	rm /etc/systemd/system/linux_packer.service
fi
if [ -e "/etc/init/linux_packer.conf" ]; then
	rm /etc/init/linux_packer.conf
fi
if [ -e "/etc/init.d/linux_packer" ]; then
	rm /etc/init.d/linux_packer
fi

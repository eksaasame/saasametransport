#!/bin/sh
SYSTEMCTL=$(command -v systemctl)
INITCTL=$(command -v initctl)
CHKCONFIG=$(command -v chkconfig)
if [ -n "${SYSTEMCTL}" ]; then
	cp /usr/local/linux_packer/linux_packer.service /etc/systemd/system/linux_packer.service
	exit
else
	if [ -n "${INITCTL}" ]; then
                cp /usr/local/linux_packer/linux_packer.conf /etc/init/linux_packer.conf
                exit
        else
                if [ -n "${CHKCONFIG}" ]; then
                        cp /usr/local/linux_packer/linux_packer.systemV /etc/init.d/linux_packer
                        exit
                fi
        fi
fi

#!/bin/sh
SYSTEMCTL=$(command -v systemctl)
INITCTL=$(command -v initctl)
CHKCONFIG=$(command -v chkconfig)
if [ -n "${SYSTEMCTL}" ]; then
        systemctl stop linux_packer
        exit
else
	if [ -n "${INITCTL}" ]; then
                initctl stop linux_packer
                exit
        else
                if [ -n "${CHKCONFIG}" ]; then
                        service linux_packer stop
                        exit
                fi
        fi
fi



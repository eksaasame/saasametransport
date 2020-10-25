#!/bin/sh
SYSTEMCTL=$(command -v systemctl)
INITCTL=$(command -v initctl)
CHKCONFIG=$(command -v chkconfig)
if [ -n "${SYSTEMCTL}" ]; then
	systemctl daemon-reload
	systemctl stop linux_packer
	systemctl enable linux_packer
	systemctl start linux_packer
	exit
else
	if [ -n "${INITCTL}" ]; then
                initctl reload-configuration
                initctl start linux_packer
                exit
        else
                if [ -n "${CHKCONFIG}" ]; then
                        chkconfig --add linux_packer
                        service linux_packer start
                        exit
                fi
        fi
fi

#!/bin/bash
export PATH="$PATH:/bin:/sbin"

declare -a release_files=( /etc/SuSE-release /etc/redhat-release /etc/os-release /etc/lsb-release )
os_family="UNKNOWN"
for release_file in "${release_files[@]}"; do
	echo ${release_file}
	if [ -f $release_file ]; then
		r=$(cat $release_file | sed 's/ //g' | egrep -i 'CentOS|RedHat' | wc -l)
		u=$(cat $release_file | egrep -i 'Debian|Ubuntu' | wc -l)
		s=$(cat $release_file | egrep -i 'SUSE|SLES' | wc -l)
		if [ $r -gt 0 ] ; then
			os_family="RedHat"
		elif [ $u -gt 0 ] ; then
			os_family="Ubuntu"
		elif [ $s -gt 0 ] ; then
			os_family="SuSE"
		fi
		break
	fi
done

if [ "$os_family" == "RedHat" ]; then
	if   grep -q -i "release 7" /etc/redhat-release ; then
		dracut --add-drivers "mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi" -f /boot/initramfs-$(uname -r).img $(uname -r)
	elif   grep -q -i "release 6" /etc/redhat-release ; then	
		d=$(find /lib/modules/$(uname -r) -iname '*.ko*' | egrep -i 'mptbase.ko|mptscsih.ko|mptspi.ko|mptsas.ko|vmw_pvscsi.ko' -o | sort -u | wc -l)
		r=$(lsinitrd /boot/initramfs-$(uname -r).img | egrep -i 'mptbase.ko|mptscsih.ko|mptspi.ko|mptsas.ko|vmw_pvscsi.ko' -o | sort -u | wc -l)
		if [ $d -ge 5 ] && [ $r -ne 5 ]; then
			dracut --add-drivers "mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi" -f /boot/initramfs-$(uname -r).img $(uname -r)
		fi
	elif   grep -q -i "release 5" /etc/redhat-release ; then
	    mkinitrd --preload mptbase --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi -f /boot/initrd-$(uname -r).img $(uname -r)
	else
		mkinitrd --preload mptbase --with mptscsi --with mptscsih --with mptfc --with mptspi --with mptsas --with vmw_pvscsi -f /boot/initrd-$(uname -r).img $(uname -r)
	fi
elif [ "$os_family" == "SuSE" ]; then
	majversion=$(lsb_release -rs | cut -f1 -d.)
	if [ $majversion -ge  12 ] ; then
		dracut --add-drivers "mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi" -f /boot/initrd-$(uname -r) $(uname -r)
	else
		files=( /etc/fstab /boot/grub/menu.lst )
		uuids=($(ls /dev/disk/by-uuid))
		for uuid in "${uuids[@]}"; do
			dev=$(readlink -f /dev/disk/by-uuid/$uuid)
			for file in "${files[@]}"; do
				sed -i "s/\/dev\/${dev#/*/}/UUID=$uuid/g" $file
			done
		done
		mkinitrd -m "mptbase mptscsih mptfc mptspi mptsas vmw_pvscsi"
	fi
elif [ "$os_family" == "Ubuntu" ]; then
    echo "..."
fi
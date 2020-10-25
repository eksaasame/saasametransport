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
	d=$(find /lib/modules/$(uname -r) -iname '*.ko*' | egrep -i 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko' -o | sort -u | wc -l)
	if   [ $d -ge 3 ] ; then
		if  grep -q -i "release 7" /etc/redhat-release ; then
			r=$(lsinitrd /boot/initramfs-$(uname -r).img | egrep -i 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko' -o | sort -u | wc -l)
			if [ $r -ne 3 ]; then
				dracut --add-drivers "hv_vmbus hv_netvsc hv_storvsc" -f /boot/initramfs-$(uname -r).img $(uname -r)
			fi
		elif  grep -q -i "release 6" /etc/redhat-release ; then
			r=$(lsinitrd /boot/initramfs-$(uname -r).img | egrep -i 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko' -o | sort -u | wc -l)
			if [ $r -ne 3 ]; then
				dracut --add-drivers "hv_vmbus hv_netvsc hv_storvsc" -f /boot/initramfs-$(uname -r).img $(uname -r)
			fi
		else
			r=$(zcat /boot/initrd-$(uname -r).img | cpio -itv | egrep -i 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko' -o | sort -u | wc -l)
			s=$(cat /boot/initrd-$(uname -r).img | cpio -itv | egrep -i 'hv_vmbus.ko|hv_netvsc.ko|hv_storvsc.ko' -o | sort -u | wc -l)
			if [ $r -ne 3 ] && [ $s -ne 3 ]; then
				mkinitrd --preload hv_vmbus --preload hv_storvsc --with hv_netvsc -f /boot/initrd-$(uname -r).img $(uname -r)
			fi
		fi
	else
		echo "Need to install AZURE drivers first."
	fi
elif [ "$os_family" == "SuSE" ]; then
	majversion=$(lsb_release -rs | cut -f1 -d.)
	if [ $majversion -ge  12 ] ; then
		dracut --add-drivers "hv_vmbus hv_netvsc hv_storvsc" -f /boot/initrd-$(uname -r) $(uname -r)
	else
		files=( /etc/fstab /boot/grub/menu.lst )
		uuids=($(ls /dev/disk/by-uuid))
		for uuid in "${uuids[@]}"; do
			dev=$(readlink -f /dev/disk/by-uuid/$uuid)
			for file in "${files[@]}"; do
				sed -i "s/\/dev\/${dev#/*/}/UUID=$uuid/g" $file
			done
		done
		mkinitrd -m "hv_vmbus hv_storvsc hv_netvsc"
	fi
elif [ "$os_family" == "Ubuntu" ]; then
    echo "..."
fi
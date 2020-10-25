#!/bin/bash
declare -a release_files=( /etc/redhat-release /etc/os-release /etc/lsb-release )
os_family="UNKNOWN"
for release_file in "${release_files[@]}"; do
	echo ${release_file}
	if [ -f $release_file ]; then
		r=$(cat $release_file | egrep -i 'CentOS|RedHat' | wc -l)
		u=$(cat $release_file | egrep -i 'Debian|Ubuntu' | wc -l)
		if [ $r -ge 0 ] ; then
			os_family="RedHat"
		elif [ $u -ge 0 ] ; then
			os_family="Ubuntu"
		fi
		break
	fi
done
echo ${os_family}

eths=( $(ip link show | grep -oP 'e[a-z0-9]+:\s') )
echo ${eths[@]}

if [ "$os_family" == "RedHat" ]; then
	ifcfgs=( $(find /etc/sysconfig/network-scripts/ifcfg-* | xargs grep -l TYPE=Ethernet) )
	for ifcfg in "${ifcfgs[@]}"; do
		if [ -d "/backup2v" ]; then
			$(mv -f $ifcfg "/backup2v/*")
		else
			$(rm -f $ifcfg)
		fi
	done
	for eth in "${eths[@]}"; do
		ifcfg="/etc/sysconfig/network-scripts/ifcfg-${eth%:*}"
		echo ${ifcfg}
		echo "TYPE=Ethernet" > $ifcfg
		echo "BOOTPROTO=dhcp" >> $ifcfg
		echo "DEFROUTE=yes" >> $ifcfg
		echo "IPV4_FAILURE_FATAL=no" >> $ifcfg
		echo "IPV6INIT=yes" >> $ifcfg
		echo "IPV6_AUTOCONF=yes" >> $ifcfg
		echo "IPV6_DEFROUTE=yes" >> $ifcfg
		echo "IPV6_FAILURE_FATAL=no" >> $ifcfg
		echo "NAME=${eth%:*}" >> $ifcfg
		echo "UUID=$(cat /proc/sys/kernel/random/uuid)" >> $ifcfg
		echo "DEVICE=${eth%:*}" >> $ifcfg
		echo "ONBOOT=yes" >> $ifcfg
		echo "PEERDNS=yes" >> $ifcfg
		echo "PEERROUTES=yes" >> $ifcfg
		echo "IPV6_PEERDNS=yes" >> $ifcfg
		echo "IPV6_PEERROUTES=yes" >> $ifcfg
		echo "IPV6_PRIVACY=no" >> $ifcfg
	done
	declare -a finish_commands=( "service NetworkManager stop" "chkconfig NetworkManager off" "service network restart" )
	for cmd in "${finish_commands[@]}"; do
		${cmd}
	done
elif [ "$os_family" == "Ubuntu" ]; then
	interfaces="/etc/network/interfaces"
	if [ -d "/backup2v" ]; then
		$(cp -f $interfaces "/backup2v/interfaces")
	fi
	echo "# The loopback network interface" > $interfaces
	echo "auto lo" >> $interfaces
	echo "iface lo inet loopback" >> $interfaces
	echo "" >> $interfaces
	echo "# The primary network interface" >> $interfaces
	for eth in "${eths[@]}"; do
		echo "auto ${eth%:*}" >> $interfaces
		echo "iface ${eth%:*} inet dhcp" >> $interfaces
		echo "" >> $interfaces
	done
	declare -a finish_commands=( "sudo stop network-manager" "update-rc.d -f NetworkManager remove" "service networking restart" "ufw disable" )
	for cmd in "${finish_commands[@]}"; do
		${cmd}
	done
fi

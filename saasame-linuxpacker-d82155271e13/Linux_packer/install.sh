#!/bin/sh
KERNEL_VERSION=$(uname -r)
echo "KERNEL_VERSION=${KERNEL_VERSION}"
INSTALL_MANAGER=""
COMMAND_RESULT=$(command -v yum)
ZYPPER_RESULT=$(command -v zypper)
WGET_RESULT=""
echo "ZYPPER_RESULT=${ZYPPER_RESULT}"
IS_11SP4=$(cat /etc/*-release | grep VERSION_ID | grep 11.4)
echo "COMMAND_RESULT=${COMMAND_RESULT}"
R_LSMOD=$(lsmod | grep dattobd)
INSTALL_TYPE="install"
DATTO_DKMS=""
DATTO_UTILS=""
KERNEL_DEVEL_PACKAGE_PATH_ORI=""
KERNEL_DEVEL_PACKAGE_PATH=""
if [ -n "${DATTO_DKMS}" ] && [ -n "${DATTO_UTILS}" ]; then
	echo MUMI
fi
echo "R_LSMOD=${R_LSMOD}"

#setting the offline install path config
if [ -n "$1" ] && [ -n "$2" ]; then
	if [ -e "$1" ] && [ -e "$2" ]; then
		INSTALL_TYPE="localinstall"
		DATTO_DKMS=$1
		DATTO_UTILS=$2
	else
		echo datto install package\'s path is not correct.
		exit
	fi
	
	if [ -n "$3" ]; then
		if [ -e "$3" ]; then
			KERNEL_DEVEL_PACKAGE_PATH=$3
		else
			echo kernel devel install package\'s path is not correct.
		fi
	fi
fi

if [ -z "${R_LSMOD}" ]; then
	if [ -n "${COMMAND_RESULT}" ]; then #have yum
		echo "COMMAND_RESULT=${COMMAND_RESULT}"
		INSTALL_MANAGER="yum -y"
		if [ -z "${KERNEL_DEVEL_PACKAGE_PATH}" ]; then
			if [ -z "${KERNEL_VERSION}" ]; then
				echo "can't get kernel version, maybe need install manually"
				exit
			else			
				KERNEL_DEVEL_PACKAGE_PATH_ORI=$(grep "$KERNEL_VERSION" kernel-devel-path | tr -d '\r')
				if [ -z "${KERNEL_DEVEL_PACKAGE_PATH_ORI}" ]; then
					echo "can't get kernel devel package's path, maybe need install manually."
					exit
				else
					echo "KERNEL_DEVEL_PACKAGE_PATH_ORI=$KERNEL_DEVEL_PACKAGE_PATH_ORI"
					${INSTALL_MANAGER} install wget
					WGET_RESULT=$(command -v wget)
					if [ -n "${WGET_RESULT}" ]; then
						wget ${KERNEL_DEVEL_PACKAGE_PATH_ORI}
						if [ $? -e 0 ]; then
							#get the kernel devel path correctly.
							KERNEL_DEVEL_PACKAGE_PATH=${KERNEL_DEVEL_PACKAGE_PATH_ORI##*/*/}
							INSTALL_TYPE="localinstall"
							echo "file name is ${KERNEL_DEVEL_PACKAGE_PATH}"
						else
							echo can\'t get ${KERNEL_DEVEL_PACKAGE_PATH_ORI}, try to install it directly.
							KERNEL_DEVEL_PACKAGE_PATH=${KERNEL_DEVEL_PACKAGE_PATH_ORI}
							INSTALL_TYPE="install"
							#do that the same as we can't get wget
						fi
					else
						echo can\'t get wget command, try to install it directly.
						KERNEL_DEVEL_PACKAGE_PATH=${KERNEL_DEVEL_PACKAGE_PATH_ORI}
						INSTALL_TYPE="install"
					fi
				fi
			fi
		fi
		#after this we should get the ${KERNEL_DEVEL_PACKAGE_PATH} and ${INSTALL_TYPE}
		${INSTALL_MANAGER} ${INSTALL_TYPE} ${KERNEL_DEVEL_PACKAGE_PATH}
		if [ ${INSTALL_TYPE}="localinstall" ]; then
			rm -rf ${KERNEL_DEVEL_PACKAGE_PATH}
		fi
		#finish installed kernel devel		
		if [ ! -d "/usr/src/kernels/${KERNEL_VERSION}" ]; then
				echo "kernel devel install fail, please install manually."
				exit
		fi
		if [ -n "${DATTO_DKMS}" ] && [ -n "${DATTO_UTILS}" ]; then
			INSTALL_TYPE="localinstall"
		else
			${INSTALL_MANAGER} localinstall https://cpkg.datto.com/datto-rpm/repoconfig/datto-el-rpm-release-$(rpm -E %rhel)-latest.noarch.rpm		
			${INSTALL_MANAGER} install datto-el-rpm-release.noarch
			DATTO_DKMS="dkms-dattobd"
			DATTO_UTILS="dattobd-utils"
			INSTALL_TYPE="install"
		fi
		#install datto
		echo ${INSTALL_MANAGER} ${INSTALL_TYPE} ${DATTO_DKMS} ${DATTO_UTILS}
		${INSTALL_MANAGER} ${INSTALL_TYPE} ${DATTO_DKMS} ${DATTO_UTILS}
		#add the reboot agent
		sed -i "s:/sbin/datto_reload:/etc/datto/dla/mnt/usr/local/linux_packer/Reboot_agent:g" $(find / -name "*dattobd.sh")
		sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/iif [ $rootfstype = ext4 ]; then\nmount -t $rootfstype -o ro,noload "$rbd" \/etc\/datto\/dla\/mnt\nelse' $(find / -name "*dattobd.sh")
		sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/afi' $(find / -name "*dattobd.sh")
		dracut -f
	else
		if [ -n "${ZYPPER_RESULT}" ]; then
			#if use suse, you should type yes here to ensure the datto install
			ktype=$(uname -r | awk -F '-' '{ print $NF }')
			kver=$(uname -r | sed "s/-${ktype}//")
			kbuild=$(rpm -qa kernel-${ktype} | grep ${kver} | awk -F '.' '{ print $NF }')
			if [ -n "${DATTO_DKMS}" ] && [ -n "${DATTO_UTILS}" ]; then
				if [ -n "${KERNEL_DEVEL_PACKAGE_PATH}" ]; then
					sudo zypper --no-gpg-checks install ${KERNEL_DEVEL_PACKAGE_PATH}
					if [ !$? -e 0 ]; then
						echo zypper --no-gpg-checks install ${KERNEL_DEVEL_PACKAGE_PATH} fail, exit.
						exit
					fi
				else
					sudo zypper --no-gpg-checks install -C "kernel-syms = ${kver}.${kbuild}"
				fi
				sudo zypper --no-gpg-checks install ${DATTO_DKMS} ${DATTO_UTILS}
				if [ !$? -e 0 ]; then
					echo zypper --no-gpg-checks install ${DATTO_DKMS} ${DATTO_UTILS} fail, exit.
					exit
				fi
			else
				if [ -n "${IS_11SP4}" ]; then #suse 11.4 x64
					sudo zypper --no-gpg-checks install https://cpkg.datto.com/datto-rpm/repoconfig/datto-sle-rpm-release-11-latest.noarch.rpm
					sudo zypper --no-gpg-checks install -C "kernel-syms = ${kver}.${kbuild}"
					sudo zypper --no-gpg-checks install dkms-dattobd dattobd-utils
					modprobe --allow-unsupported dattobd
					#add the reboot agent
					sed -i "s:/sbin/datto_reload:/etc/datto/dla/mnt/usr/local/linux_packer/Reboot_agent:g" $(find / -name "*dattobd.sh")
					sed -i 's:$fstype:$rootfstype:g' $(find / -name "*dattobd.sh")
					sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/iif [ $rootfstype = ext4 ]; then\nmount -t $rootfstype -o ro,noload "$rbd" \/etc\/datto\/dla\/mnt\nelse' $(find / -name "*dattobd.sh")
					sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/afi' $(find / -name "*dattobd.sh")
					mkinitrd_setup
					mkinitrd -v -m dattobd
				else #suse 12i
					sudo zypper --no-gpg-checks addrepo http://download.opensuse.org/repositories/X11:/Bumblebee/SLE_12_SP3_Backports/X11:Bumblebee.repo
					sudo zypper --no-gpg-checks install https://cpkg.datto.com/datto-rpm/repoconfig/datto-sle-rpm-release-12-latest.noarch.rpm
					sudo zypper --no-gpg-checks install -C "kernel-syms = ${kver}"
					sudo zypper --no-gpg-checks install dkms-dattobd dattobd-utils
					modprobe --allow-unsupported dattobd
					#add the reboot agent
					sed -i "s:/sbin/datto_reload:/etc/datto/dla/mnt/usr/local/linux_packer/Reboot_agent:g" $(find / -name "*dattobd.sh")
					sed -i 's/modprobe/modprobe --allow-unsupported/g' $(find / -name "*dattobd.sh")
					sed -i 's:$fstype:$rootfstype:g' $(find / -name "*dattobd.sh")
					sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/iif [ $rootfstype = ext4 ]; then\nmount -t $rootfstype -o ro,noload "$rbd" \/etc\/datto\/dla\/mnt\nelse' $(find / -name "*dattobd.sh")
					sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/afi' $(find / -name "*dattobd.sh")
					dracut -f
				fi
			fi
			
		else
			INSTALL_MANAGER="apt-get -y --allow-unauthenticated"
			#$INSTALL_MANAGER install linux-headers-generic
			if [ -n "${DATTO_DKMS}" ] && [ -n "${DATTO_UTILS}" ]; then
				INSTALL_MANAGER="dpkg -i"
				${INSTALL_MANAGER} ${DATTO_DKMS} ${DATTO_UTILS}
				if [ !$? -e 0 ]; then
					echo ${INSTALL_MANAGER} ${DATTO_DKMS} ${DATTO_UTILS} fail, exit.
					exit
				fi
			else
				$INSTALL_MANAGER adv --recv-keys --keyserver keyserver.ubuntu.com 29FF164C
				echo "deb https://cpkg.datto.com/repositories $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/datto-linux-agent.list
				$INSTALL_MANAGER update
				$INSTALL_MANAGER install dattobd-dkms dattobd-utils
				#add the reboot agent
				sed -i "s:/sbin/datto_reload:/etc/datto/dla/mnt/usr/local/linux_packer/Reboot_agent:g" $(find /usr/ -name "*dattobd")
				sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/iif [ $rootfstype = ext4 ]; then\nmount -t $rootfstype -o ro,noload "$rbd" \/etc\/datto\/dla\/mnt\nelse' $(find /usr/ -name "*dattobd")
				sed -i '/mount -t $rootfstype -o ro "$rbd" \/etc\/datto\/dla\/mnt/afi' $(find /usr/ -name "*dattobd")
				update-initramfs -u
			fi
		fi
	fi
	modprobe --allow-unsupported dattobd
	modprobe dattobd
	R_LSMOD=$(lsmod | grep dattobd)
	if [ -z "${R_LSMOD}" ]; then
		echo "datto driver are not mount, please check the driver is install or not, or install manually"
		exit
	fi
fi
if [ -n "${COMMAND_RESULT}" ]; then
	rpm -ivp linux_packer-1.1.813-0.x86_64.rpm
else
	if [ -n "${ZYPPER_RESULT}" ]; then
		zypper --no-gpg-checks install linux_packer-1.1.813-0.x86_64.rpm
	else
		dpkg -i linux-packer_1.1.813-0_amd64.deb
	fi
fi


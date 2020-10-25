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
if [ -n "${COMMAND_RESULT}" ]; then #have yum
	echo "COMMAND_RESULT=${COMMAND_RESULT}"
	INSTALL_MANAGER="yum -y"
	if [ -z "${KERNEL_VERSION}" ]; then
		echo "can't get kernel version, maybe need install manually"
	else
		KERNEL_DEVEL_PACKAGE_PATH=$(grep "$KERNEL_VERSION" kernel-devel-path | tr -d '\r')
		if [ -z "${KERNEL_DEVEL_PACKAGE_PATH}" ]; then
			echo "can't get kernel devel package's path, maybe need install manually."
		else
			echo "KERNEL_DEVEL_PACKAGE_PATH=$KERNEL_DEVEL_PACKAGE_PATH"
			${INSTALL_MANAGER} install wget
			WGET_RESULT=$(command -v wget)
			if [ -n "${WGET_RESULT}" ]; then
				wget ${KERNEL_DEVEL_PACKAGE_PATH}
				echo "file name is ${KERNEL_DEVEL_PACKAGE_PATH##*/*/}"
				if [ -a ${KERNEL_DEVEL_PACKAGE_PATH##*/*/} ]; then
					${INSTALL_MANAGER} localinstall ${KERNEL_DEVEL_PACKAGE_PATH##*/*/}
					remove ${KERNEL_DEVEL_PACKAGE_PATH##*/*/}
				else
					echo "download ${KERNEL_DEVEL_PACKAGE_PATH} fail, install directly"
					${INSTALL_MANAGER} install ${KERNEL_DEVEL_PACKAGE_PATH}
				fi
			else
				${INSTALL_MANAGER} install ${KERNEL_DEVEL_PACKAGE_PATH}
			fi
		fi
	fi
	if [ ! -d "/usr/src/kernels/${KERNEL_VERSION}" ]; then
        	echo "kernel devel install fail, please install manually."
        	exit
	fi
	${INSTALL_MANAGER} localinstall https://cpkg.datto.com/datto-rpm/repoconfig/datto-el-rpm-release-$(rpm -E %rhel)-latest.noarch.rpm
	${INSTALL_MANAGER} install datto-el-rpm-release.noarch
	${INSTALL_MANAGER} install dkms-dattobd dattobd-utils
else
    if [ -n "${ZYPPER_RESULT}" ]; then
		#if use suse, you should type yes here to ensure the datto install
		ktype=$(uname -r | awk -F '-' '{ print $NF }')
		kver=$(uname -r | sed "s/-${ktype}//")
		kbuild=$(rpm -qa kernel-${ktype} | grep ${kver} | awk -F '.' '{ print $NF }')
		if [ -n "${IS_11SP4}" ]; then #suse 11.4 x64
			sudo zypper --no-gpg-checks install https://cpkg.datto.com/datto-rpm/repoconfig/datto-sle-rpm-release-11-latest.noarch.rpm
			sudo zypper --no-gpg-checks install -C "kernel-syms = ${kver}.${kbuild}"
			sudo zypper --no-gpg-checks install dkms-dattobd dattobd-utils
			modprobe --allow-unsupported dattobd
			mkinitrd-setup
			mkinitrd -v -m dattobd
		else #suse 12i
			sudo zypper --no-gpg-checks addrepo http://download.opensuse.org/repositories/X11:/Bumblebee/SLE_12_SP3_Backports/X11:Bumblebee.repo
			sudo zypper --no-gpg-checks install https://cpkg.datto.com/datto-rpm/repoconfig/datto-sle-rpm-release-12-latest.noarch.rpm
			sudo zypper --no-gpg-checks install -C "kernel-syms = ${kver}"
			sudo zypper --no-gpg-checks install dkms-dattobd dattobd-utils
			modprobe --allow-unsupported dattobd
			sed -i 's/modprobe/modprobe --allow-unsupported/g' $(find / -name "*dattobd.sh")
			dracut -f
		fi
    else
		INSTALL_MANAGER="apt-get -y --allow-unauthenticated"
		$INSTALL_MANAGER adv --recv-keys --keyserver keyserver.ubuntu.com 29FF164C
		echo "deb https://cpkg.datto.com/repositories $(lsb_release -sc) main" | sudo tee /etc/apt/sources.list.d/datto-linux-agent.list
		$INSTALL_MANAGER update
		$INSTALL_MANAGER install dattobd-dkms dattobd-utils
    fi
fi
modprobe --allow-unsupported dattobd
modprobe dattobd
R_LSMOD=$(lsmod | grep dattobd)
echo "${R_LSMOD}"
if [ -z "${R_LSMOD}" ]; then
	echo "datto driver are not mount, please check the driver is install or not, or install manually"
	exit
fi
if [ -n "${COMMAND_RESULT}" ]; then
	rpm -ivp linux_packer-1.0.0-1.el6.x86_64.rpm
else
	if [ -n "${ZYPPER_RESULT}" ]; then
		zypper install linux_packer-1.0.0-1.el6.x86_64.rpm
	else
		dpkg -i linux-packer_1.0.0-2_amd64.deb
	fi
fi
mkdir /etc/saasame
cp /usr/local/linux_packer/server.* /etc/saasame/


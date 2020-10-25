#!/bin/sh
PACKER_NAME=linux_packer
REGISTER_NAME=Linux_packer_https
PACKAGE_NAME=linux_packer-1.0.0.tar.gz
COMPILE_CONFIG_NAME=compile_config
LOCAL_PATH=/usr/local/
P_MAJOR=1
P_MINOR=1
P_BUILD=815
if [ -n "$1" ]
then
P_MAJOR=$1
fi
if [ -n "$2" ]
then
P_MINOR=$2
fi
if [ -n "$3" ]
then
P_BUILD=$3
fi
VERSION=$P_MAJOR.$P_MINOR.$P_BUILD
PACKAGE_NAME=$PACKER_NAME-$VERSION.tar.gz
VENDOR=SaaSaMe
VENDOR_MAIL_ADDRESS=ek@saasame.com
PACKAGER="Chihhung Kuo"
PACKAGER_MAIL_ADDRESS=Chihhung@saasame.com
RPMBUILD_PATH=/home/makerpm/rpmbuild
#create COMPILE_CONFIG_NAME
echo "#pragma once" > $COMPILE_CONFIG_NAME.h
echo "#ifndef" $COMPILE_CONFIG_NAME"_H" >> $COMPILE_CONFIG_NAME.h
echo "#define" $COMPILE_CONFIG_NAME"_H" >> $COMPILE_CONFIG_NAME.h
echo "#define PRODUCT_MAJOR "$P_MAJOR >> $COMPILE_CONFIG_NAME.h
echo "#define PRODUCT_MINOR "$P_MINOR >> $COMPILE_CONFIG_NAME.h
echo "#define PRODUCT_BUILD "$P_BUILD >> $COMPILE_CONFIG_NAME.h
echo "#endif" >> $COMPILE_CONFIG_NAME.h

#create linux_packer.sh
echo "### BEGIN INIT INFO" >> $PACKER_NAME.sh
echo "# Provides:          $PACKAGER" >> $PACKER_NAME.sh
echo "# Required-Start:    $local_fs" >> $PACKER_NAME.sh
echo "# Required-Stop:     $local_fs" >> $PACKER_NAME.sh
echo "# Default-Start:     3 5" >> $PACKER_NAME.sh
echo "# Default-Stop:      0 1 2 4 6" >> $PACKER_NAME.sh
echo "# Short-Description: $PACKER_NAME" >> $PACKER_NAME.sh
echo "# Description:       $PACKER_NAME" >> $PACKER_NAME.sh
echo "### END INIT INFO" >> $PACKER_NAME.sh
echo "#!/bin/bash" > $PACKER_NAME.sh
echo "ulimit -n 65535" >> $PACKER_NAME.sh
echo "export PATH=\$PATH:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/root/bin" >> $PACKER_NAME.sh
echo $LOCAL_PATH$PACKER_NAME"/"$PACKER_NAME >> $PACKER_NAME.sh
chmod 755 $PACKER_NAME.sh

#create linux_packer.service
echo "[Unit]" > $PACKER_NAME.service
echo "Description=SaaSaMe linux packer" >> $PACKER_NAME.service
echo "After=syslog.target network.target" >> $PACKER_NAME.service
echo "" >> $PACKER_NAME.service
echo "[Service]" >> $PACKER_NAME.service
echo "WorkingDirectory="$LOCAL_PATH$PACKER_NAME"/" >> $PACKER_NAME.service
echo "User=root" >> $PACKER_NAME.service
echo "Group=root" >> $PACKER_NAME.service
echo "ExecStart="$LOCAL_PATH$PACKER_NAME"/"$PACKER_NAME".sh" >> $PACKER_NAME.service
echo "Restart=always" >> $PACKER_NAME.service
echo "RestartSec=5" >>$PACKER_NAME.service
echo "" >> $PACKER_NAME.service
echo "[Install]" >> $PACKER_NAME.service
echo "WantedBy=default.target" >> $PACKER_NAME.service

#create linux_packer.conf
echo "description \""$PACKER_NAME"\"" > $PACKER_NAME.conf
echo "author \""$PACKAGER"\"" >> $PACKER_NAME.conf
echo "start on filesystem or runlevel [345]" >> $PACKER_NAME.conf
echo "stop on runlevel [06]" >> $PACKER_NAME.conf
echo "respawn" >> $PACKER_NAME.conf
echo "script" >> $PACKER_NAME.conf
echo "export HOME=\"/srv\"" >> $PACKER_NAME.conf
echo "echo \$\$ > /var/run/linux_packer.pid" >> $PACKER_NAME.conf
echo "exec /usr/local/"$PACKER_NAME"/"$PACKER_NAME".sh" >> $PACKER_NAME.conf
echo "end script" >> $PACKER_NAME.conf
echo "pre-start script" >> $PACKER_NAME.conf
echo "echo \"[\`date\`] linux_packer Starting\" >> /var/log/linux_packer.log" >> $PACKER_NAME.conf
echo "end script" >> $PACKER_NAME.conf
echo "pre-stop script" >> $PACKER_NAME.conf
echo "rm /var/run/"$PACKER_NAME".pid" >> $PACKER_NAME.conf
echo "echo \"[\`date\`] linux_packer Stopping\" >> /var/log/linux_packer.log" >> $PACKER_NAME.conf
echo "end script" >> $PACKER_NAME.conf

#create package.sh
echo "#!/bin/sh" >> package.sh
echo "rpmbuild -ba $PACKER_NAME.spec" >> package.sh
echo "mv -f /home/makerpm/rpmbuild/RPMS/x86_64/$PACKER_NAME-$VERSION-1.el6.x86_64.rpm $PACKER_NAME-1.0.0-1.el6.x86_64.rpm" >> package.sh
chmod 755 package.sh

#create linux_packer.systemV
echo "#!/bin/sh" > $PACKER_NAME.systemV
echo "### BEGIN INIT INFO" >> $PACKER_NAME.systemV
echo "# Provides:          $PACKAGER" >> $PACKER_NAME.systemV
echo "# Required-Start:    $local_fs" >> $PACKER_NAME.systemV
echo "# Required-Stop:     $local_fs" >> $PACKER_NAME.systemV
echo "# Default-Start:     3 5" >> $PACKER_NAME.systemV
echo "# Default-Stop:      0 1 2 4 6" >> $PACKER_NAME.systemV
echo "# Short-Description: $PACKER_NAME" >> $PACKER_NAME.systemV
echo "# Description:       $PACKER_NAME" >> $PACKER_NAME.systemV
echo "### END INIT INFO" >> $PACKER_NAME.systemV
echo "PATH=/sbin:/usr/sbin:/bin:/usr/bin" >> $PACKER_NAME.systemV
echo "DESC=\""$PACKER_NAME"\"" >> $PACKER_NAME.systemV
echo "NAME="$PACKER_NAME >> $PACKER_NAME.systemV
echo "DAEMON=/usr/local/\$NAME/\$NAME" >> $PACKER_NAME.systemV
echo "DAEMON_ARGS=\"--options args\"" >> $PACKER_NAME.systemV
echo "PIDFILE=/var/run/\$NAME.pid" >> $PACKER_NAME.systemV
echo "PID=-1" >> $PACKER_NAME.systemV
echo "SCRIPTNAME=/etc/init.d/\$NAME" >> $PACKER_NAME.systemV
echo "[ -x \"\$DAEMON\" ] || exit 0" >> $PACKER_NAME.systemV
echo "do_start()" >> $PACKER_NAME.systemV
echo "{" >> $PACKER_NAME.systemV
echo "if [ -f \${PIDFILE} ]; then" >> $PACKER_NAME.systemV
echo "echo \"PID file \${PIDFILE} already exists!\"" >> $PACKER_NAME.systemV
echo "exit" >> $PACKER_NAME.systemV
echo "fi" >> $PACKER_NAME.systemV
echo "\$DAEMON >/dev/null 2>&1 &" >> $PACKER_NAME.systemV
echo "PID=\$!" >> $PACKER_NAME.systemV
echo "if [ -z \$PID ]; then" >> $PACKER_NAME.systemV
echo "echo \"get PID failed!\"" >> $PACKER_NAME.systemV
echo "exit" >> $PACKER_NAME.systemV
echo "else" >> $PACKER_NAME.systemV
echo "echo \"Starting successfully, whose pid is \${PID}\"" >> $PACKER_NAME.systemV
echo "fi" >> $PACKER_NAME.systemV
echo "touch \$PIDFILE" >> $PACKER_NAME.systemV
echo "echo \${PID} > \${PIDFILE}" >> $PACKER_NAME.systemV
echo "}" >> $PACKER_NAME.systemV
echo "do_stop()" >> $PACKER_NAME.systemV
echo "{" >> $PACKER_NAME.systemV
echo "if [ -f \${PIDFILE} ]; then" >> $PACKER_NAME.systemV
echo "PID=\$( cat \${PIDFILE} )" >> $PACKER_NAME.systemV
echo "if [ -z \$PID ]; then" >> $PACKER_NAME.systemV
echo "echo \"get PID failed!\"" >> $PACKER_NAME.systemV
echo "exit" >> $PACKER_NAME.systemV
echo "else" >> $PACKER_NAME.systemV
echo "if [ -z \"\`ps axf | grep \$PID | grep -v grep\`\" ]; then" >> $PACKER_NAME.systemV
echo "echo \"Process dead but pidfile exists!\"" >> $PACKER_NAME.systemV
echo "rm -f \$PIDFILE" >> $PACKER_NAME.systemV
echo "exit" >> $PACKER_NAME.systemV
echo "else" >> $PACKER_NAME.systemV
echo "kill -9 \$PID" >> $PACKER_NAME.systemV
echo "echo \"Stopping service successfully , whose pid is \$PID\"" >> $PACKER_NAME.systemV
echo "rm -f \$PIDFILE" >> $PACKER_NAME.systemV
echo "fi" >> $PACKER_NAME.systemV
echo "fi" >> $PACKER_NAME.systemV
echo "else" >> $PACKER_NAME.systemV
echo "echo \"File $PIDFILE does NOT exist!\"" >> $PACKER_NAME.systemV
echo "fi" >> $PACKER_NAME.systemV
echo "}" >> $PACKER_NAME.systemV
echo "case \"\$1\" in" >> $PACKER_NAME.systemV
echo "start)" >> $PACKER_NAME.systemV
echo "do_start" >> $PACKER_NAME.systemV
echo ";;" >> $PACKER_NAME.systemV
echo "stop)" >> $PACKER_NAME.systemV
echo "do_stop" >> $PACKER_NAME.systemV
echo ";;" >> $PACKER_NAME.systemV
echo "*)" >> $PACKER_NAME.systemV
echo "echo \"Usage: \$SCRIPTNAME {start|stop|status|restart|force-reload}\" >&2" >> $PACKER_NAME.systemV
echo "exit 3" >> $PACKER_NAME.systemV
echo ";;" >> $PACKER_NAME.systemV
echo "esac" >> $PACKER_NAME.systemV
chmod 755 $PACKER_NAME.systemV

#create linux_packer.spec
echo "%define Name "$PACKER_NAME > $PACKER_NAME.spec
echo "%define Install_path "$LOCAL_PATH"%{Name}" >> $PACKER_NAME.spec
echo "%define Version "$VERSION >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%define BuildRoot /tmp/%{Name}-%{Version}-buildroot" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "Name:           %{Name}" >> $PACKER_NAME.spec
echo "Vendor:     "$VENDOR" <"$VENDOR_MAIL_ADDRESS">" >> $PACKER_NAME.spec
echo "Packager:   "$PACKAGER" <"$PACKAGER_MAIL_ADDRESS">" >> $PACKER_NAME.spec
echo "Version:        %{Version}" >> $PACKER_NAME.spec
echo "Release:        1%{?dist}" >> $PACKER_NAME.spec
echo "Summary:        linux_packer" >> $PACKER_NAME.spec
echo "Group:          Applications/System" >> $PACKER_NAME.spec
echo "License:        GPLv2+" >> $PACKER_NAME.spec
echo "#URL:" >> $PACKER_NAME.spec
echo "Source0:        %{name}-%{Version}.tar.gz" >> $PACKER_NAME.spec
echo "Source1:        %{name}.conf" >> $PACKER_NAME.spec
echo "Source2:        %{name}.service" >> $PACKER_NAME.spec
echo "Source3:        %{name}.systemV" >> $PACKER_NAME.spec
echo "BuildRoot:      %{BuildRoot}" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "#not sure" >> $PACKER_NAME.spec
echo "#BuildRequires:" >> $PACKER_NAME.spec
echo "#Requires:" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%description" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%pre" >> $PACKER_NAME.spec
echo 'LSDATTO=$(lsmod | grep dattobd)' >> $PACKER_NAME.spec
echo 'if [ -z "${LSDATTO}" ]; then' >> $PACKER_NAME.spec
echo 'echo "can not find dattobd module, please install datto first"' >> $PACKER_NAME.spec
echo 'exit 1' >> $PACKER_NAME.spec
echo 'fi' >> $PACKER_NAME.spec
echo "if [ \$1 = 2 ] || [ \"\$1\" = \"upgrade\" ]; then" >> $PACKER_NAME.spec
echo %{Install_path}/stopservice.sh >> $PACKER_NAME.spec
echo %{Install_path}/serviceremove.sh >> $PACKER_NAME.spec
echo "fi" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%build" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "#only decompress the package and execute the service config bash file" >> $PACKER_NAME.spec
echo "%install" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{_sysconfdir}/%{Name}/" >> $PACKER_NAME.spec
#echo "mkdir -p %{buildroot}%{_sysconfdir}/init/" >> $PACKER_NAME.spec
#echo "mkdir -p %{buildroot}%{_sysconfdir}/systemd/system/" >> $PACKER_NAME.spec
#echo "mkdir -p %{buildroot}%{_sysconfdir}/init.d/" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{_sysconfdir}/saasame/" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{Install_path}" >> $PACKER_NAME.spec
echo "install -p -m 644 %{SOURCE0} %{buildroot}%{_sysconfdir}/%{Name}/" >> $PACKER_NAME.spec
#echo "install -p -m 644 %{SOURCE1} %{buildroot}%{_sysconfdir}/init/%{Name}.conf" >> $PACKER_NAME.spec
#echo "install -p -m 644 %{SOURCE2} %{buildroot}%{_sysconfdir}/systemd/system/%{Name}.service" >> $PACKER_NAME.spec
#echo "install -p -m 755 %{SOURCE3} %{buildroot}%{_sysconfdir}/init.d/%{Name}" >> $PACKER_NAME.spec
echo "install -p -m 644 %{SOURCE1} %{buildroot}%{Install_path}/%{Name}.conf" >> $PACKER_NAME.spec
echo "install -p -m 644 %{SOURCE2} %{buildroot}%{Install_path}/%{Name}.service" >> $PACKER_NAME.spec
echo "install -p -m 755 %{SOURCE3} %{buildroot}%{Install_path}/%{Name}.systemV" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{Install_path}" >> $PACKER_NAME.spec
echo "tar zxvf %{buildroot}%{_sysconfdir}/%{Name}/%{name}-%{Version}.tar.gz -C %{buildroot}%{Install_path}" >> $PACKER_NAME.spec
echo "rm -f %{buildroot}%{_sysconfdir}/%{Name}/%{name}-%{Version}.tar.gz" >> $PACKER_NAME.spec
echo "cp -f %{buildroot}%{Install_path}/server.* %{buildroot}%{_sysconfdir}/saasame/" >> $PACKER_NAME.spec

echo "#after install over, starting service" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%post" >> $PACKER_NAME.spec
echo "#sudo mkdir -p %{Install_path}" >> $PACKER_NAME.spec
echo "#sudo cp -rf %{buildroot}%{Install_path}/*.* %{Install_path}" >> $PACKER_NAME.spec
echo "#bash %{Install_path}/\$\${setting_sh} -b -p" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "sudo dmidecode -t memory -q | grep 'Size: ' | egrep -v 'Installed|Enabled|Maximum' | sed 's/.*[:]//'|sed 's/[(].*//' | sed 's/ //g' > /usr/local/"$PACKER_NAME"/.memory" >> $PACKER_NAME.spec
echo %{Install_path}/serviceconfig.sh >> $PACKER_NAME.spec
echo %{Install_path}/initservice.sh >> $PACKER_NAME.spec
#echo "initctl reload-configuration" >> $PACKER_NAME.spec
#echo "initctl start %{Name}" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%preun" >> $PACKER_NAME.spec
#echo "initctl stop %{Name}" >> $PACKER_NAME.spec
echo "if [ \$1 = 0 ] || [ \"\$1\" = \"remove\" ]; then" >> $PACKER_NAME.spec
echo %{Install_path}/stopservice.sh >> $PACKER_NAME.spec
echo %{Install_path}/serviceremove.sh >> $PACKER_NAME.spec
echo "fi" >> $PACKER_NAME.spec
echo "%postun" >> $PACKER_NAME.spec
echo "if [ \$1 = 0 ] || [ \"\$1\" = \"remove\" ]; then" >> $PACKER_NAME.spec
echo "rm -rf %{Install_path}" >> $PACKER_NAME.spec
echo "rm -rf /etc/saasame" >> $PACKER_NAME.spec
echo "fi" >> $PACKER_NAME.spec
echo "%files" >> $PACKER_NAME.spec
echo "%{Install_path}/*" >> $PACKER_NAME.spec
#echo "%config %{_sysconfdir}/init/%{Name}.conf" >> $PACKER_NAME.spec
#echo "%{_sysconfdir}/init.d/%{Name}" >> $PACKER_NAME.spec
#echo "%{_sysconfdir}/systemd/system/%{Name}.service" >> $PACKER_NAME.spec
echo "%{_sysconfdir}/saasame/*" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%doc" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%changelog" >> $PACKER_NAME.spec
chmod 644 $PACKER_NAME.spec
#build start
make clean;make
chmod 755 $PACKER_NAME
if [ -e $PACKER_NAME ]
then
cp ../reboot_agent/Reboot_agent Reboot_agent
tar zcvf $PACKAGE_NAME $PACKER_NAME $PACKER_NAME.sh server.crt server.key serviceremove.sh serviceconfig.sh initservice.sh stopservice.sh Reboot_agent
chmod 644 $PACKAGE_NAME
fi



if [ -e $PACKAGE_NAME ]
then
mv -f $PACKAGE_NAME $RPMBUILD_PATH/SOURCES/$PACKAGE_NAME
mv -f $PACKER_NAME.conf $RPMBUILD_PATH/SOURCES/$PACKER_NAME.conf
mv -f $PACKER_NAME.service $RPMBUILD_PATH/SOURCES/$PACKER_NAME.service
mv -f $PACKER_NAME.systemV $RPMBUILD_PATH/SOURCES/$PACKER_NAME.systemV
mv -f $PACKER_NAME.spec $RPMBUILD_PATH/SPECS/$PACKER_NAME.spec
mv -f package.sh $RPMBUILD_PATH/SPECS/package.sh
mkdir -p $RPMBUILD_PATH/SPECS/$REGISTER_NAME
cp -f ../$REGISTER_NAME/$REGISTER_NAME $RPMBUILD_PATH/SPECS/$REGISTER_NAME
fi

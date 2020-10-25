#!/bin/sh
PACKER_NAME=linux_packer
PACKAGE_NAME=linux_packer-1.0.0.tar.gz
COMPILE_CONFIG_NAME=compile_config
LOCAL_PATH=/usr/local/
P_MAJOR=1
P_MINOR=0
P_BUIID=0
VERSION=1.0.0
VENDOR=SaaSaMe
VENDOR_MAIL_ADDRESS=ek@saasame.com
PACKAGER="Chihhung Kuo"
PACKAGER_MAIL_ADDRESS=Chihhung@saasame.com
RPMBUILD_PATH=/home/makerpm/rpmbuild
#create COMPILE_CONFIG_NAME
echo "#pragma once" > $COMPILE_CONFIG_NAME.h
echo "#ifndef" $COMPILE_CONFIG_NAME"_H" >> $COMPILE_CONFIG_NAME.h
echo "#define" $COMPILE_CONFIG_NAME"_H" >> $COMPILE_CONFIG_NAME.h
if [ -n "$1" ]
then
echo "#define PRODUCT_MAJOR "$1 >> $COMPILE_CONFIG_NAME.h
else
echo "#define PRODUCT_MAJOR "$P_MAJOR >> $COMPILE_CONFIG_NAME.h
fi
if [ -n "$2" ]
then
echo "#define PRODUCT_MINOR "$2 >> $COMPILE_CONFIG_NAME.h
else
echo "#define PRODUCT_MINOR "$P_MINOR >> $COMPILE_CONFIG_NAME.h
fi
if [ -n "$3" ]
then
echo "#define PRODUCT_BUILD "$3 >> $COMPILE_CONFIG_NAME.h
else
echo "#define PRODUCT_BUILD "$P_BUIID >> $COMPILE_CONFIG_NAME.h
fi
echo "#endif" >> $COMPILE_CONFIG_NAME.h

#create linux_packer.sh
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
echo "Source1:        %{name}.service" >> $PACKER_NAME.spec
echo "BuildRoot:      %{BuildRoot}" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "#not sure" >> $PACKER_NAME.spec
echo "#BuildRequires:" >> $PACKER_NAME.spec
echo "#Requires:" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%description" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%prep" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%build" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "#only decompress the package and execute the service config bash file" >> $PACKER_NAME.spec
echo "%install" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{_sysconfdir}/%{Name}/" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{_sysconfdir}/systemd/system/" >> $PACKER_NAME.spec
echo "install -p -m 644 %{SOURCE0} %{buildroot}%{_sysconfdir}/%{Name}/" >> $PACKER_NAME.spec
echo "install -p -m 644 %{SOURCE1} %{buildroot}%{_sysconfdir}/systemd/system/%{Name}.service" >> $PACKER_NAME.spec
echo "mkdir -p %{buildroot}%{Install_path}" >> $PACKER_NAME.spec
echo "tar zxvf %{buildroot}%{_sysconfdir}/%{Name}/%{name}-%{Version}.tar.gz -C %{buildroot}%{Install_path}" >> $PACKER_NAME.spec
echo "rm -f %{buildroot}%{_sysconfdir}/%{Name}/%{name}-%{Version}.tar.gz" >> $PACKER_NAME.spec
echo "#after install over, starting service" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%post" >> $PACKER_NAME.spec
echo "#sudo mkdir -p %{Install_path}" >> $PACKER_NAME.spec
echo "#sudo cp -rf %{buildroot}%{Install_path}/*.* %{Install_path}" >> $PACKER_NAME.spec
echo "#bash %{Install_path}/\$\${setting_sh} -b -p" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "sudo dmidecode -t memory -q | grep 'Size: ' | egrep -v 'Installed|Enabled|Maximum' | sed 's/.*[:]//'|sed 's/[(].*//' | sed 's/ //g' > /usr/local/"$PACKER_NAME"/.memory" >> $PACKER_NAME.spec
echo "systemctl daemon-reload" >> $PACKER_NAME.spec
echo "systemctl stop %{Name}" >> $PACKER_NAME.spec
echo "systemctl enable %{Name}" >> $PACKER_NAME.spec
echo "systemctl start %{Name}" >> $PACKER_NAME.spec
echo "" >> $PACKER_NAME.spec
echo "%preun" >> $PACKER_NAME.spec
echo "systemctl stop %{Name}" >> $PACKER_NAME.spec
echo "systemctl disable %{Name}" >> $PACKER_NAME.spec
echo "%postun" >> $PACKER_NAME.spec
echo "rm -rf %{Install_path}" >> $PACKER_NAME.spec
echo "%files" >> $PACKER_NAME.spec
echo "%{Install_path}/*" >> $PACKER_NAME.spec
echo "%config %{_sysconfdir}/systemd/system/%{Name}.service" >> $PACKER_NAME.spec
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
tar zcvf $PACKAGE_NAME $PACKER_NAME $PACKER_NAME.sh server.crt server.key
chmod 644 $PACKAGE_NAME
fi

if [ -e $PACKAGE_NAME ]
then
mv -f $PACKAGE_NAME $RPMBUILD_PATH/SOURCES/$PACKAGE_NAME
mv -f $PACKER_NAME.service $RPMBUILD_PATH/SOURCES/$PACKER_NAME.service
mv -f $PACKER_NAME.spec $RPMBUILD_PATH/SPECS/$PACKER_NAME.spec
fi

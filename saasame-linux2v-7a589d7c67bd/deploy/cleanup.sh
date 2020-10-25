#!/bin/sh
envbin=/opt/py35/envs/linux2v/bin

## Clean up Linux Launcher
systemctl stop linux2v
cp linux2v/deploy/production/production.ini .
cp linux2v/deploy/production/resetdb.ini .
cp linux2v/deploy/production/circus.ini .
cp linux2v/deploy/production/linux2v.service /etc/systemd/system/linux2v.service
rm -rf linux2v/data
mkdir linux2v/data
$envbin/linux2v resetdb --force=True resetdb.ini
rm resetdb.ini

cd linux2v
$envbin/python  setup.py develop
$envbin/pip install -r requirements.pip/production.pip
cd ..
rm -rf linux2v/tmp/
rm -rf linux2v/deploy/
rm -rf linux2v/.git
rm -rf linux2v/.idea
rm -rf linux2v/.cache
rm -rf linux2v/dl.fedoraproject.org
rm -rf depssh*
rm retouched.sh
rm linux2v/.gitignore
rm linux2v/development.ini
rm linux2v/production.ini
rm linux2v/test.ini
rm linux2v/circus.conf
rm linux2v/linux2v.log
find linux2v -name '__pycache__' | xargs rm -rf

systemctl --system daemon-reload
# systemctl start linux2v
systemctl enable linux2v


## Clean up root
rm .bash_history
rm .zshrc
rm linux2v.log
rm store_circus.ini
rm .lvm_history
rm .vimrc
rm .viminfo
rm .xonsh_man_completions
rm lvmdump-LDevC-20160329102828.tgz
rm vg_centos6forkvm-lv_root.diskfile
rm .python_history
rm -rf .ssh/
rm -rf .local/
rm -rf .cache/
rm -rf lvmdump-LDevC-20160329102828
rm -rf .vim
rm -rf thriftpy-bak

echo "Cloud168.ssm" | passwd root --stdin
touch "build-`date "+%Y-%m-%dT%H:%M:%SZ"`"

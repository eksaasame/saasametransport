# Build Environment

## Requirements

- OS: CentOS 7 x86-64bit
	- With LVM and default vgname `ssmllncher`
	- Recommanded:
		- CPU >= 1
		- RAM >= 1024 MB
		- Disk >= 16 GB
- Installed RPMs:
	- linux2v_rpms-1.6.1.el7.x86_64.tar.gz
		- What's inside linux2v_rpms? See `deploy/makerpm/linux2v.spec`.
	- rpm-build-4.11.3-17.el7.x86_64
	- git-1.8.3.1-6.el7_2.1.x86_64
	- gcc
- Required source:
	- linux2v source: `/root/linux2v`
		- `git clone git@bitbucket.org:saasame/linux2v.git /root/linux2v`
			- Require [root's ssh pubkey on Bitbucket repo deployment settings](https://confluence.atlassian.com/bitbucket/use-deployment-keys-294486051.html).
	- thriftpy source: `/root/thriftpy`
		- `git clone https://github.com/eleme/thriftpy.git /root/thriftpy`
- Required files (not in source):
	- `/root/Miniconda3-latest-Linux-x86_64.sh`
		- [Miniconda3](http://conda.pydata.org/miniconda.html) is used to build builder environment.
		- Be sure to verify its MD5 sums.
	- Under `/root/linux2v/deploy/makerpm/SOURCES/`
		- Miniconda3-latest-Linux-x86_64.sh
			- The same Miniconda3 file. This one will be packaged into Linux Launcher RPM.
		- server.pem
			- `cat server.crt server.key > server.pem`

## Steps

After requirements done:

```
source /usr/local/linux2v/bin/activate
cd /root/linux2v
sh deploy/compile.sh
cd /root
sh linux2v/deploy/buildbuilder.sh
```
Final package `linux2v_rpms-1.6.2.el7.x86_64.tar.gz` will be created under `/root/linux2v/deploy/makerpm/`.

## Make a new version from 1.6.2 to 1.6.3 or 1.7.0

Add changelog in HISTORY.rst.
All strings of `1.6.2` in Linux Launcher sources should be replaced to `1.6.3` or `1.7.0` and be commited into Git repository. Including:

- `HISTORY.md`
- `linux2v/__init__.py`
- `deploy/makerpm/linux2v.spec`

Then follow the steps:

```
source /usr/local/linux2v/bin/activate
cd /root/linux2v
sh deploy/uncompile.sh
sh deploy/compile.sh
cd /root
sh linux2v/deploy/buildbuilder.sh
```
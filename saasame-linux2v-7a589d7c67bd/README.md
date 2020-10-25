# linux2v

## Goal

Move linux from physical/virtual to AWS/OpenStack/...etc.

## Spec

1. Detect Linux boot image to see if virtio included.
    - Mount given disks.
    - Check if the root of mount contains boot.
    - Check if boot image is Linux and has virtio included.
    - Return the result of detection.
2. Jobs management.
3. Thrift service.

## Installation

1. Install CentOS 7 64bit (minimal is enough) as launcher host.
2. Download 192.168.31.125:output\Linux Launcher\linux2v-rpms-[version]-el7.x86_64.tar.gz to launcher host.
3. On launcher host:
```
#!sh
tar zxf linux2v_rpms-[version]-el7.x86_64.tar.gz
cd linux2v_rpms-[version] && ./install.sh
```

## Configuration

### How to change SSL key

```
a. cat server.crt server.key > server.pem
b. cp server.pem /etc/linux2v/ssl/server.pem
c. systemctl restart linux2v
```

## Upgrade
Copy new package to Linux Launcher.

```
scp linux2v_rpms-[version]-el7.x86_64.tar.gz root@[Linux Launcher IP address]:
```

ssh into Linux Launcher and run:

```
tar zxf linux2v_rpms-[version]-el7.x86_64.tar.gz
cd linux2v_rpms-[version] && ./install.sh
```


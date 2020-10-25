#!/bin/sh
pip install -Iv nuitka==0.5.27
src=/root/linux2v
pip install -q $src
echo "Compiling linux2v..."
cd ${src}/linux2v/lib/
nuitka --module log.py --recurse-not-to=alog
cd ${src}/linux2v/
nuitka --module spec.py --recurse-not-to=thriftpy --recurse-not-to=psutil
cd ${src}/linux2v/fixtures
nuitka --module mac.py

cd ${src}/linux2v/db/
nuitka --module store.py --recurse-not-to=transaction --recurse-not-to=BTrees.OOBTree

cd ${src}/linux2v/lib/
nuitka --module cli.py --recurse-not-to=linux2v --recurse-not-to=pyramid.settings
nuitka --module strutil.py
nuitka --module humanhash.py
nuitka --module utc.py --recurse-not-to=arrow

cd ${src}/linux2v/lib/macho
nuitka --module host.py
nuitka --module disk.py --recurse-not-to=psutil
nuitka --module storage.py --recurse-not-to=functional

cd ${src}/linux2v/
nuitka --module exceptions.py
nuitka --module srift.py
nuitka --module mgmt.py --recurse-not-to=thriftpy.http
nuitka --module response.py --recurse-not-to=pyramid.response --recurse-not-to=zope.interface --recurse-not-to=pyramid.interfaces
nuitka --module request.py --recurse-not-to=pyramid.request --recurse-not-to=pyramid.decorator --recurse-not-to=pyramid_zodbconn --recurse-not-to=zope.interface --recurse-not-to=pyramid.interfaces --recurse-not-to=pyramid.util

cd ${src}/linux2v/launcher
nuitka --module scheduler.py
nuitka --module templates.py
nuitka --module thirdparty.py --recurse-not-to=pyramid.decorator
nuitka --module networkconverter.py --recurse-not-to=arrow
nuitka --module converter.py --recurse-not-to=functional
nuitka --module job.py --recurse-not-to=persistent --recurse-not-to=transaction
nuitka --module service.py --recurse-not-to=thriftpy --recurse-not-to=transaction --recurse-not-to=pyramid.settings

cd ${src}/linux2v/
nuitka --module hueytask.py --recurse-not-to=transaction --recurse-not-to=huey
nuitka --module hueyserve.py --recurse-not-to=pyramid.paster

cd  ${src}/linux2v/scripts
nuitka --module base.py --recurse-to=pyramid.scripts.common --recurse-not-to=pyramid.paster --recurse-not-to=pyramid.compat
nuitka --module maincmd.py --recurse-none

cd  ${src}
rm linux2v/db/store.py
rm linux2v/exceptions.py
rm linux2v/fixtures/mac.py
rm linux2v/launcher/converter.py
rm linux2v/launcher/job.py
rm linux2v/launcher/networkconverter.py
rm linux2v/launcher/scheduler.py
rm linux2v/launcher/service.py
rm linux2v/launcher/templates.py
rm linux2v/launcher/thirdparty.py
rm linux2v/lib/cli.py
rm linux2v/lib/log.py
rm linux2v/lib/strutil.py
rm linux2v/lib/humanhash.py
rm linux2v/lib/macho/disk.py
rm linux2v/lib/macho/host.py
rm linux2v/lib/macho/storage.py
rm linux2v/lib/utc.py
rm linux2v/mgmt.py
rm linux2v/request.py
rm linux2v/response.py
rm linux2v/srift.py
rm linux2v/spec.py
rm linux2v/hueytask.py
rm linux2v/hueyserve.py
rm linux2v/scripts/base.py
rm linux2v/scripts/maincmd.py

"""
Avoid alog imported everywhere with external package while compiling
"""
import alog

debug = alog.debug
info = alog.info
warning = alog.warning
error = alog.error
exception = alog.exception

set_level = alog.set_level
pformat = alog.pformat

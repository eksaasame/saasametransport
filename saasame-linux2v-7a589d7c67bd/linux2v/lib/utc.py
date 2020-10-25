import time
import datetime

import arrow


def posix_time(given_dt=None):
    given_dt = given_dt or datetime.datetime.utcnow()
    return time.mktime(given_dt.timetuple())


def now():
    return arrow.utcnow()

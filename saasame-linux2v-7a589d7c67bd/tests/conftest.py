import pytest
import alog

from linux2v.scripts.base import bootstrap_in_console
from linux2v.lib import cli


@pytest.fixture(scope="session", autouse=True)
def bootstrap_for_all_tests(request):
    alog.info("bootstrap_for_all_tests()")
    environ, linux2v_request = bootstrap_in_console('tests.ini')
    if cli.os_disk_operable:
        pytest.exit("cli.os_disk_operable shouldn't be True in testing.")
    linux2v_request.store.initdb(linux2v_request.dbroot)
    seen = set([None])
    session = request.node
    for item in session.items:
        cls = item.getparent(pytest.Class)
        if cls not in seen:
            cls.obj.test_environ = environ
            cls.obj.linux2v_request = linux2v_request
            seen.add(cls)

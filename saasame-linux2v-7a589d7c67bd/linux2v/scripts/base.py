import sys

from pyramid.paster import (
    get_appsettings,
    setup_logging,
    bootstrap,
)

from ..lib import log
from ..request import create_request_from_environ


def load_settings_from_argv(argv=sys.argv):
    from pyramid.scripts.common import parse_vars
    config_uri = argv[1]
    options = parse_vars(argv[2:])
    setup_logging(config_uri)
    log.info("Setup logging: " + config_uri)
    settings = get_appsettings(config_uri, options=options)
    return settings


def bootstrap_in_console(configfile):
    setup_logging(configfile)
    env = bootstrap(configfile)
    request = create_request_from_environ(env)
    return env, request

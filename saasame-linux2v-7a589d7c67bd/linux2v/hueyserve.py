import os
from pyramid.paster import bootstrap

from .hueytask import (  # noqa
    huey,
    update_job_state_to_mgmt,
    run_launcher_job
)

huey.pyramid_env = bootstrap(os.environ['PYRAMID_CONFIG'])

import transaction
import uuid
from unittest.mock import patch, MagicMock

from linux2v.launcher.job import (
    LauncherJob,
    clone_job_detail
)
from linux2v import mgmt
from linux2v.spec import (
    thrift,
    JobState,
)

from .base import BaseLauncherTest


class TestLauncherJob(BaseLauncherTest):
    def setup(self):
        replica_id = str(uuid.uuid4())
        self.job = LauncherJob(self.linux2v_request.dbroot, id=replica_id)

    def test_clone_job_detail(self):
        self.job.update_detail_state(JobState.initialized)
        self.job.log_job_history('LauncherJob State: [histories] -> [testing]')
        assert len(self.job.detail.histories) == 2
        detail = clone_job_detail(self.job)
        assert len(detail.histories) == 2

    def test_clone_job_detail_for_mgmt_update(self):
        self.job.update_detail_state(JobState.initialized)
        self.job.log_job_history('Not starts with LauncherJob State: [')
        assert len(self.job.detail.histories) == 2
        detail = clone_job_detail(self.job, for_mgmt_update=True)
        assert len(detail.histories) == 1

    def test_call_mgmt(self):
        patch_config = {'method.return_value': {}}
        with patch.object(mgmt, 'call_mgmt_service', **patch_config):
            self.job.mgmt_detail = MagicMock()
            self.job.get_execution_detail_from_mgmt('session_id')
            self.job.update_state_to_mgmt('session_id')

    def test_update_state(self):
        self.job.update_detail_state(JobState.discarded)

    def test_update_state_with_msg(self):
        msg = "Test update state A."
        self.job.update_detail_state(JobState.initialized, msg)

    def test_update_state_with_msg_args(self):
        msg_format = "Test boost format %1% %2%"
        msg_args = [self.job, "OK"]
        self.job.update_detail_state(JobState.discarded, msg_format, msg_args)

    def test__ensure_lvm_disk_devices_is_up(self):
        execution_detail = thrift.launcher_job_create_detail()
        execution_detail.disks_lun_mapping = {
            "60874b4b-4652-4a0d-a4ca-f0daec064429": "/dev/vdb"}
        disk_devices = self.job._map_disks_to_devices(execution_detail)
        self.job._ensure_lvm_disk_devices_is_up(disk_devices)

    def test_detail_histories(self):
        self.job.update_detail_state(JobState.initialized)
        self.job.update_detail_state(JobState.discarded)
        self.job.update_detail_state(JobState.initialized)
        self.job.save()
        transaction.commit()
        assert len(self.job.detail.histories) == 3
        for h in self.job.detail.histories:
            print(h.time, h.state, h.description)

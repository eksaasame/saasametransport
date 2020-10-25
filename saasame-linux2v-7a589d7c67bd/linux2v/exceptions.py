

class X2VException(Exception):
    pass


class DiskPartitionNotFound(X2VException):
    pass


class LVMDeviceNotFound(X2VException):
    pass


class OSUnknown(X2VException):
    pass


class JobExecutionFailed(X2VException):
    pass


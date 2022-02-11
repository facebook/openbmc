#!/usr/bin/python3


class FwBaseException(Exception):
    pass


class FwJsonExceptionMultipleFiles(FwBaseException):
    pass


class FwJsonExceptionMissingFiles(FwBaseException):
    pass


class FwJsonExceptionMismatchEntities(FwBaseException):
    pass


class FwUpgraderUnexpectedJsonKey(FwBaseException):
    pass


class FwUpgraderMissingJsonKey(FwBaseException):
    pass


class FwUpgraderUnexpectedFileHash(FwBaseException):
    pass


class FwUpgraderMissingFileHash(FwBaseException):
    pass


class FwUpgraderFailedUpgrade(FwBaseException):
    pass

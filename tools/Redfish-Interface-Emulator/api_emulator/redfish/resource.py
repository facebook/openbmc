# Copyright Notice:
# Copyright 2016-2019 DMTF. All rights reserved.
# License: BSD 3-Clause License. For full text see link: https://github.com/DMTF/Redfish-Interface-Emulator/blob/master/LICENSE.md

# Resource module based on Resource.0.9.2

class StateEnum(object):
    """
    States based on Resource.State in Resource.0.9.2 (Redfish)
    """
    ENABLED = 'Enabled'
    DISABLED = 'Disabled'
    OFFLINE = 'Offline'
    IN_TEST = 'InTest'
    STARTING = 'Starting'
    ABSENT = 'Absent'


class HealthEnum(object):
    """
    Health based on Resource.Health in Resource.0.9.2 (Redfish)
    """
    OK = 'OK'
    WARNING = 'Warning'
    CRITICAL = 'Critical'
    # WARNING: THIS IS NOT IN THE SPEC -- In the mockup data for system 1
    DEGRADED = 'Degraded'


def Status(state, health, health_rollup=None):
    """
    Status based on Resource.Status in Resource.0.9.2 (Redfish)
    """
    status = {'State': state, 'Health': health}

    if health_rollup is not None:
        status['HealthRollup'] = health_rollup
    return status

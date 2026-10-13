from typing import Any, Dict  # noqa: F401

# The root filesystem is in RAM, so a reboot wipes the generated rules and
# reverts to this file. Endpoints that must serve right after the BMC comes
# up, without waiting to be reconfigured, need a default here.
RULES = {
    "/api/sys/logfile": {"POST": ["MANAGED_HOST_ANY"]},
}  # type: Dict[str, Any]
RULES_REGEXP = {}  # type: Dict[str, Any]

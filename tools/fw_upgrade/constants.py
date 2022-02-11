#!/usr/bin/python3
from enum import Enum


# Keys from fw_version_schema.json
UFW_VERSION = "version"
UFW_NAME = "filename"
UFW_HASH = "hash"
UFW_HASH_VALUE = "hash_value"

# Keys from fw_manifest_schema.json
UFW_GET_VERSION = "get_version"
UFW_CMD = "upgrade_cmd"
UFW_PRIORITY = "priority"
UFW_REBOOT_REQD = "reboot_required"
UFW_ENTITY_INSTANCE = "entities"
UFW_POST_ACTION = "post_action"
UFW_CONDITION = "condition"
UFW_CONTINUE_ON_ERROR = "continue_on_error"

# Other constants
# Wait between upgrads - in secs
WAIT_BTWN_UPGRADE = 5
DEFAULT_UPGRADABLE_ENTITIES = "all"


# =============================================================================
# Enumerations
# =============================================================================
class ResetMode(Enum):
    USERVER_RESET = "yes"
    HARD_RESET = "hard"
    NO_RESET = "no"


class UpgradeMode(Enum):
    UPGRADE = "yes"
    NO_UPGRADE = "no"
    FORCE_UPGRADE = "Forced Mode Only"


class HashType(Enum):
    SHA1SUM = "sha1sum"
    MD5SUM = "md5sum"


class UpgradeState(Enum):
    NONE = 100
    SUCCESS = 1
    FAIL = 0

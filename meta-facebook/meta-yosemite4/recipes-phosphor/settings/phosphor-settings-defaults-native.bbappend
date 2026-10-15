# Drop the pre-created DIMM inventory objects.
#
# The upstream template creates 20 objects per slot (12 Sentinel Dome + 8 Wailua
# Falls) for all 8 slots, unconditionally: the settings YAML is expanded by a
# build-time code generator that cannot know the board config, which is
# discovered per slot at runtime. Those objects declare Decorator.Asset, so
# `mfgtool inventory` lists them as empty entries on configs without the
# hardware -- all 8 WF DIMMs on every T1/T1C slot, plus A5/A11 on T1.
#
# dimm-inventory now creates objects through phosphor-inventory-manager for the
# DIMMs dimm-util actually reports, so the static template is redundant.
#
# Remove upstream once the revert of the corresponding openbmc/openbmc change
# lands and is synced.
SRC_URI:remove = "file://yosemite4-temporary-host-dimm-inventory.yml"
SETTINGS_HOST_TEMPLATES:remove = "yosemite4-temporary-host-dimm-inventory.yml"

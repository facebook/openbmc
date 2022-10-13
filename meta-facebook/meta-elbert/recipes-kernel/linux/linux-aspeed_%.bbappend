LINUX_VERSION_EXTENSION = "-elbert"

COMPATIBLE_MACHINE = "elbert"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI += "file://elbert.cfg"

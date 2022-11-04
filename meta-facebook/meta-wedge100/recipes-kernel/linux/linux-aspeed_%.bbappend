LINUX_VERSION_EXTENSION = "-wedge100"

COMPATIBLE_MACHINE = "wedge100"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI += "file://wedge100.cfg \
           "

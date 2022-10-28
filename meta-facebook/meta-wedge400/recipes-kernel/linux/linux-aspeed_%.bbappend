LINUX_VERSION_EXTENSION = "-wedge400"

COMPATIBLE_MACHINE = "wedge400"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI += "file://wedge400.cfg \
           "

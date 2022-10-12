LINUX_VERSION_EXTENSION = "-cmm"
COMPATIBLE_MACHINE = "cmm"

FILESEXTRAPATHS:prepend := "${THISDIR}/board_config:"

SRC_URI += "file://cmm.cfg"

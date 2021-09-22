LINUX_VERSION_EXTENSION = "-fbcc"

COMPATIBLE_MACHINE = "clearcreek"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "file://fbcc.cfg"

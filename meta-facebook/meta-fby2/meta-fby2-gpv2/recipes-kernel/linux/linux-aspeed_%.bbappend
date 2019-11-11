LINUX_VERSION_EXTENSION = "-fbgp2"

COMPATIBLE_MACHINE = "fbgp2"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"

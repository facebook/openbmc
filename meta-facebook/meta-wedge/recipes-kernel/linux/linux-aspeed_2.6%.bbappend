LINUX_VERSION_EXTENSION = "-wedge"

COMPATIBLE_MACHINE = "wedge"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

KERNEL_MODULE_AUTOLOAD += "fb_panther_plus"

SRC_URI += "file://defconfig \
           "

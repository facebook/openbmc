LINUX_VERSION_EXTENSION = "-minilaketb"

COMPATIBLE_MACHINE = "minilaketb"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"

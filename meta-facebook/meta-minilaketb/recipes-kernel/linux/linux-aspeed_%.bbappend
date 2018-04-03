LINUX_VERSION_EXTENSION = "-minilaketb"

COMPATIBLE_MACHINE = "minilaketb"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"

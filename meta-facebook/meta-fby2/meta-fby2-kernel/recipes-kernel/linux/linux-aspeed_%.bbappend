LINUX_VERSION_EXTENSION = "-fby2-kernel"

COMPATIBLE_MACHINE = "fby2-kernel"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"

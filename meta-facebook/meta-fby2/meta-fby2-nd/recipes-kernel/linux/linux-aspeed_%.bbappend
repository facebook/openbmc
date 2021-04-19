LINUX_VERSION_EXTENSION = "-fby2"

COMPATIBLE_MACHINE = "fby2|fbnd|northdome|northdome-vboot"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += '${@bb.utils.contains("MACHINE_FEATURES", "tpm2", "file://fbnd-vboot.cfg", "file://fbnd.cfg", d)}'

KERNEL_MODULE_AUTOLOAD += " \
"

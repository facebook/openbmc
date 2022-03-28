LINUX_VERSION_EXTENSION = "-fby2"

COMPATIBLE_MACHINE = "fby2|fbnd|northdome|northdome-vboot"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " file://fbnd.cfg"
SRC_URI:remove:mf-tpm2 = "file://fbnd.cfg"
SRC_URI:append:mf-tpm2 = " file://fbnd-vboot.cfg"

KERNEL_MODULE_AUTOLOAD += " \
"

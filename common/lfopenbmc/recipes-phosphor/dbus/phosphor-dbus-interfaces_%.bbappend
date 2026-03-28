FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-CPER-add-generic-event-for-CPER-errors.patch \
    file://0002-add-events-and-errors-for-firmware-update.patch \
    file://0003-com.amd-AMD-OEM-RAS-configuration-interface-added.patch \
"

do_write_config[depends] = ""

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-CPER-add-generic-event-for-CPER-errors.patch \
    file://0002-Control-Port-Add-interface-for-monitoring-control.patch \
"

do_write_config[depends] = ""

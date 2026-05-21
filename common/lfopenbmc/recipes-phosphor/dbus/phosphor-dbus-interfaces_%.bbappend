FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-CPER-add-generic-event-for-CPER-errors.patch \
"

do_write_config[depends] = ""

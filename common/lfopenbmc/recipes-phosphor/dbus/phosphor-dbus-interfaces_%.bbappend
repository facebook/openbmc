FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-CPER-add-generic-event-for-CPER-errors.patch \
    file://0002-add-events-and-errors-for-firmware-update.patch \
"

SRC_URI:append:ventura2 = " \
    file://0101-valve-add-ValveUnableToReachSetPoint-event.patch \
    file://0102-configuration-add-AnalogValve-D-Bus-interface.patch \
"

do_write_config[depends] = ""

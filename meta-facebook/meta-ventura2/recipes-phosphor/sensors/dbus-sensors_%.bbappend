FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
PACKAGECONFIG:append = " valvemonitor"

SRC_URI:append = " \
    file://0016-implement-analog-valve-control-for-VT2-inventory.patch \
"

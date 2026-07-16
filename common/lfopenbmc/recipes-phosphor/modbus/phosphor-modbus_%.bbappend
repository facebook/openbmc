FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-rtu-control-port-monitoring-over-D-Bus.patch \
    file://0002-rtu-re-read-firmware-on-monitoring-resume.patch \
"

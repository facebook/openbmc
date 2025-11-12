FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-Add-sensor-reading-events-for-virtual-sensors.patch \
"

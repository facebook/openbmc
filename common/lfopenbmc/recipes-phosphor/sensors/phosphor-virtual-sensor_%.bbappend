FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-Add-a-units-and-objPath-member-to-the-sensor-class.patch \
    file://0002-Add-structured-logging-for-threshold-events.patch \
"

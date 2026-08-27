FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-rtu-re-read-firmware-on-monitoring-resume.patch \
"

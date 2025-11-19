FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-oemcommands-Stabilize-motherboard-FRU-selection-on-A.patch \
    file://0002-yosemite4-fb-ipmi-oem-skip-FRU-read-when-motherboard.patch \
"

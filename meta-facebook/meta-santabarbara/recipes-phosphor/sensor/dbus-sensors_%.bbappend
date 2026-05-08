FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-Thresholds-Add-option-to-log-thresholds-on-second-hi.patch \
"

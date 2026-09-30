FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-redfish-Add-FaultLog-attachment-download-support.patch \
"

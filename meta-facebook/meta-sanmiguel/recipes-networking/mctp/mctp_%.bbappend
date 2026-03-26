FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://1001-mctpd-allow-bridge-for-assign-static-endpoint.patch \
"

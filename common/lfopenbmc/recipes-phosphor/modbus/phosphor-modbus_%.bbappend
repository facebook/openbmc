FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-rtu-port-use-name-based-device-path-lookup.patch \
"

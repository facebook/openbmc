FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-udev-builtin-net_id-Extend-persistent-naming-support.patch \
"

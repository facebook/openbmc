FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-meta-facebook-yosemite5-add-APML-recovery.patch \
    "

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PTEST_ENABLED = ""

SRC_URI:append:openbmc-fb = " \
    file://less.cfg \
    "

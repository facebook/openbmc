FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/${PN}:"
SRC_URI:append:openbmc-fb-lf = " \
    file://ifconfig.cfg \
"

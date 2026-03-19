FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem=meta"

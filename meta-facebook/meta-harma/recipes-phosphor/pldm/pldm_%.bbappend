FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

EXTRA_OEMESON:append = " -Dmaximum-transfer-size=150 "
EXTRA_OEMESON:append = " -Dsensor-polling-time=2000 "

SRC_URI += " \
    file://host_eid \
"



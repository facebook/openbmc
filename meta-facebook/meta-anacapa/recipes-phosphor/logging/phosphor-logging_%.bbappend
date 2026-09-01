FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

EXTRA_OEMESON:append = " \
    -Damd-event-log=enabled \
    -Dmax-cper-raw-binary-size=65536 \
"

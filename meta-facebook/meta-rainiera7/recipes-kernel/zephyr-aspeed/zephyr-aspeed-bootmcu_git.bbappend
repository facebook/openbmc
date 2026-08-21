FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ast2700_rainiera7_bootmcu.overlay \
"

BOOTMCU_EXTRA_DTC_OVERLAY_FILE:append = ";${UNPACKDIR}/ast2700_rainiera7_bootmcu.overlay"

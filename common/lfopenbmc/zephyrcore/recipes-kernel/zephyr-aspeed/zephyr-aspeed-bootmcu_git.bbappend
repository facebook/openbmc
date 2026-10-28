FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ast2700_facebook_bootmcu.overlay \
"

BOOTMCU_EXTRA_DTC_OVERLAY_FILE ?= "${UNPACKDIR}/ast2700_facebook_bootmcu.overlay"

EXTRA_OECMAKE:append = " \
    -DEXTRA_DTC_OVERLAY_FILE="${BOOTMCU_EXTRA_DTC_OVERLAY_FILE}" \
"

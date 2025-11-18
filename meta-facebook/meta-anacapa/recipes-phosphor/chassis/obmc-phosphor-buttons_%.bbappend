FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-fix-reset-button-not-working.patch \
    "

EXTRA_OEMESON:append = " -Dreset-button-do-warm-reboot=enabled "

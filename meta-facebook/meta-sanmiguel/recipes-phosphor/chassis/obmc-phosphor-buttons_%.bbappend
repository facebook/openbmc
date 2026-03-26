FILESEXTRAPATHS:append := "${THISDIR}/${PN}:"

DEPENDS += "libgpiod"

SRC_URI += " \
    file://0001-gpio-Add-support-for-retrieving-gpio-number-via-line.patch \
    "

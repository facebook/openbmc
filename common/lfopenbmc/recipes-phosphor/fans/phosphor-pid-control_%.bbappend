FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

SRC_URI:append:openbmc-fb-lf = " \
    file://0001-Fix-log-flooding-caused-by-per-sensor-failsafe-loggi.patch \
"

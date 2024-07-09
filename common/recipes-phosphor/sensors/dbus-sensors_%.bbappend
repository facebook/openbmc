FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-psusensor-avoid-crash-by-activate-sensor-twice.patch \
    file://0002-PwmSensor-Fix-function-getValue-returns-wrong-data-t.patch \
"

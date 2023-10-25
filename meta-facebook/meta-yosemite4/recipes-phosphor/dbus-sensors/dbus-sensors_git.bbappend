FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-PSUSensor-support-device-adc128d818-and-ina233.patch \
    file://0002-PSUSensor-add-adm1281-support.patch \
    file://0003-fanSensor-fix-bug-for-the-pwmsensor-probing-error.patch \
"

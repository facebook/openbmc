FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-clang-format-copy-latest-and-re-format.patch \
    file://0002-PSUSensor-add-ina233-support.patch \
    file://0003-PSUSensor-add-adm1281-support.patch \
    file://0004-fanSensor-Fix-pwmsensors-space-naming-probe-error.patch \
"

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-i2c-vr-add-preUpdateFirmware-hook-for-MPS-devices.patch \
"

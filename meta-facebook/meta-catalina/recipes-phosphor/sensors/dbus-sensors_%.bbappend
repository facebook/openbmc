FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
    file://0101-smbpbi-Optimize-i2cReadDataBytes-using-combined-ioct.patch \
    file://0102-smbpbi-Add-support-for-PowerState-config-option.patch \
    file://0103-SmbpbiSensor-Add-range-checking-for-values.patch \
    "

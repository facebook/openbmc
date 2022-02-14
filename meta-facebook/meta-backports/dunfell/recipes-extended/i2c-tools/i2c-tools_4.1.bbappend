FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-i2cget-Add-support-for-I2C-block-data.patch \
    file://0002-i2cget-Document-the-support-of-I2C-block-read.patch \
    file://0003-i2cget-Add-support-for-SMBus-block-read.patch \
    "

FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += " \
    file://1000-clemente-dts-Add-NCSI-properties-fan-TACH.patch \
    file://1001-clemente-dts-enable-byte-mode-on-I2C11.patch \
    file://1002-clemente-dts-Add-ADC-channel-gains-for-MAX1363.patch \
    file://clemente-sensor.cfg \
"


FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI += " \
    file://1000-clemente-dts-Add-NCSI-properties-fan-TACH.patch \
    file://1001-clemente-dts-enable-byte-mode-on-I2C11.patch \
    file://1002-clemente-dts-Add-ADC-channel-gains-for-MAX1363.patch \
    file://1003-clemente-dts-add-shunt-resistor-micro-ohm.patch \
    file://1004-clemente-dts-add-GPIO-expander-and-LED-for-HDD-activity-LED.patch \
    file://1005-clemente-dts-Add-EEPROMs-for-boot-and-data-drive-FRUs.patch \
    file://1006-clemente-dts-add-gpio-line-name-to-io-exp.patch \
    file://clemente-sensor.cfg \
"


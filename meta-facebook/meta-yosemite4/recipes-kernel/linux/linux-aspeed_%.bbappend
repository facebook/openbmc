FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://1000-add-meta-yv4-bmc-dts-setting.patch \
    file://1001-ARM-dts-aspeed-yosemite4-support-mux-to-cpld.patch \
    file://1002-ARM-dts-aspeed-yosemite4-Revise-gpio-name.patch \
    file://1003-remove-pincontrol-on-GPIO-U5.patch \
    file://1004-Meta-yv4-dts-add-mac-config-property.patch \
    file://1005-yosemite4-Add-EEPROMs-for-NICs-in-DTS.patch \
    file://1006-Add-ina233-and-ina238-devicetree-config-for-EVT-sche.patch \
    file://1007-Adjust-resistor-calibration-for-matching-hardware-de.patch \
    file://1008-ARM-dts-aspeed-yosemite4-Revise-i2c-duty-cycle.patch \
    file://1009-ARM-dts-aspeed-yosemite4-disable-GPIO-internal-Pull-.patch \
    file://1010-yosemite4-Change-IOE-i2c-address.patch \
    file://1011-Revise-duty-cycle-for-SMB9-and-SMB10.patch \
    file://1012-Add-nct7363-in-yosemite4-dts.patch \
    file://1013-ARM-dts-aspeed-yosemite4-add-I3C-config-in-DTS.patch \
    file://1014-ARM-dts-aspeed-yosemite4-add-fan-led-config.patch \
    file://1015-Add-XDP710-in-yosemite4-dts.patch \
    file://1016-ARM-dts-aspeed-yosemite4-add-RTQ6056-support.patch \
    file://1017-meta-facebook-yosemite4-update-dts-file-to-enable-mp.patch \
    file://1018-ARM-dts-aspeed-yosemite4-adjust-mgm-cpld-ioexp.patch \
    file://1019-ARM-dts-aspeed-yosemite4-remove-mctp-driver-on-bus-1.patch \
    file://1020-ARM-dts-aspeed-yosemite4-fix-GPIO-linename-typo.patch \
    file://1021-ARM-dts-aspeed-yosemitet4-add-RTQ6056-support-on-11-.patch \
    file://1022-ARM-dts-aspeed-yosemite4-remove-power-sensor-0x44-on.patch \
    file://1023-ARM-dts-aspeed-yosemite4-add-Spider-Board-SQ52205-dr.patch \
    file://1024-ARM-dts-aspeed-yosemite4-add-GPIO-I6-to-dts.patch \
"

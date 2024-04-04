FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0500-add-meta-yv4-bmc-dts-setting.patch \
    file://0501-hwmon-ina233-Add-ina233-driver.patch \
    file://0502-hwmon-max31790-support-to-config-PWM-as-TACH.patch \
    file://0503-Add-adm1281-driver.patch \
    file://0504-hwmon-max31790-add-fanN_enable-for-all-fans.patch \
    file://0505-ARM-dts-aspeed-yosemite4-support-mux-to-cpld.patch \
    file://0506-ARM-dts-aspeed-yosemite4-Revise-gpio-name.patch \
    file://0507-remove-pincontrol-on-GPIO-U5.patch \
    file://0508-NCSI-Add-propety-no-channel-monitor-and-start-redo-p.patch \
    file://0509-Meta-yv4-dts-add-mac-config-property.patch \
    file://0510-yosemite4-Add-EEPROMs-for-NICs-in-DTS.patch \
    file://0511-Add-ina233-and-ina238-devicetree-config.patch \
    file://0512-Adjust-resistor-calibration-for-matching-hardware-de.patch \
    file://0513-ARM-dts-aspeed-yosemite4-Revise-i2c-duty-cycle.patch \
    file://0514-ARM-dts-aspeed-yosemite4-Enable-ipmb-device-for-OCP-.patch \
    file://0515-pinctrl-pinctrl-aspeed-g6-correct-the-offset-of-SCU6.patch \
    file://0516-ARM-dts-aspeed-yosemite4-disable-GPIO-internal-Pull-.patch \
    file://0517-yosemite4-Change-IOE-i2c-address.patch \
    file://0518-hwmon-Driver-for-Nuvoton-NCT7363Y.patch \
    file://0519-Revise-duty-cycle-for-SMB9-and-SMB10.patch \
"

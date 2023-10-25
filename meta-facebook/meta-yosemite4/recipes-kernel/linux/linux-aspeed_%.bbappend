FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://0500-add-meta-yv4-bmc-dts-setting.patch \
    file://0501-hwmon-ina233-Add-ina233-driver.patch \
    file://0502-hwmon-max31790-support-to-config-PWM-as-TACH.patch \
    file://0503-Add-adm1281-driver.patch \
"


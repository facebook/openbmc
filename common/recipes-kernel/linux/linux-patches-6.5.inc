FILESEXTRAPATHS:prepend := "${THISDIR}/6.5:"

#
# JFFS2 patches to workaround kernel panic.
# Highly recommended for all the OpenBMC platforms running JFFS2.
#
SRC_URI:append = " \
    file://0100-jffs2-kill-garbage-collect-thread-when-filesystem-is.patch \
    file://0101-jffs2-suppress-jffs2-messages-when-reading-inode.patch \
"

#
# JTAG patches.
# Used by multiple OpenBMC platforms for firmware upgrade.
#
SRC_URI:append = " \
    file://0120-drivers-jtag-Add-JTAG-core-driver.patch \
    file://0121-dt-binding-jtag-Aspeed-2400-and-2500-series.patch \
    file://0122-Add-Aspeed-SoC-24xx-and-25xx-families-JTAG-master-dr.patch \
    file://0123-jtag-aspeed-support-sysfs-interface.patch \
    file://0124-jtag-support-driver-specific-ioctl-commands.patch \
    file://0125-jtag-aspeed-support-JTAG_RUN_CYCLE-ioctl-command.patch \
    file://0126-jtag-aspeed-add-AST2600-support.patch \
    file://0127-Documentation-jtag-Add-ABI-documentation.patch \
    file://0128-Documentation-jtag-Add-JTAG-core-driver-ioctl-number.patch \
    file://0129-drivers-jtag-Add-JTAG-core-driver-Maintainers.patch \
    file://0130-fixup-jtag-patch-series.patch \
    file://0131-ARM-dts-aspeed-g6-add-jtag-controllers.patch \
    file://0132-ARM-dts-aspeed-g5-add-jtag-controller.patch \
"

#
# SPI patches.
# "0150-spi-aspeed-add-shutdown-path..." is required for all the AST25XX
# platforms.
# "0151-spi-add-user-mode..." is used by several FBOSS platforms to access
# flashes using spidev/flashrom.
#
SRC_URI:append = " \
    file://0150-spi-aspeed-add-shutdown-path-for-AST25XX-SPI-control.patch \
    file://0151-spi-add-user-mode-aspeed-spi-driver.patch \
"

#
# I2C patches used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0160-aspeed-i2c-add-clock-duty-cycle-property.patch \
    file://0161-dt-bindings-aspeed-i2c-add-properties-for-setting-i2.patch \
    file://0162-i2c-aspeed-update-ast2400-timing-settings.patch \
    file://0163-i2c-add-a-slave-backend-to-receive-and-queue-message.patch \
    file://0164-i2c-aspeed-add-i2c-slave-inactive-timeout-support.patch \
"

#
# PWM patches used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0165-dt-bindings-hwmon-fan-Add-fan-binding-to-schema.patch \
    file://0166-dt-bindings-hwmon-Support-Aspeed-g6-PWM-TACH-Control.patch \
    file://0167-hwmon-aspeed-g6-pwm-tacho-Support-for-ASPEED-g6-PWM-.patch \
    file://0168-aspeed-g6.dtsi-update-for-PWM-and-TACH-driver.patch \
"

#
# NCSI patches used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0169-net-ncsi-Simplify-Kconfig-dts-control-flow.patch \
    file://0170-net-ncsi-Fix-netlink-major-minor-version-numbers.patch \
    file://0171-net-ncsi-Add-NC-SI-1.2-Get-MC-MAC-Address-command.patch \
"

#
# eMMC patch used by FBOSS OpenBMC platforms.
#
SRC_URI:append = " \
    file://0200-mmc-sdhci-of-aspeed-add-skip_probe-module-parameter.patch \
"

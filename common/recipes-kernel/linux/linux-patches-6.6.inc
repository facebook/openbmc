FILESEXTRAPATHS:prepend := "${THISDIR}/6.6:"

#
# Patches to be upstreamed soon.
#
SRC_URI:append = " \
    file://0001-ARM-dts-aspeed-Add-Facebook-FBDarwin-AST2600-BMC.patch \
    file://0002-ARM-dts-aspeed-elbert-Enable-mdio0-devices.patch \
    file://0003-ARM-dts-aspeed-Add-Facebook-fblite-r1-BMC.patch \
    file://0004-ARM-dts-aspeed-Expand-data0-partition-in-facebook-bm.patch \
    file://0005-mtd-spi-nor-winbond-Add-support-for-w25q01jv.patch \
    file://0006-net-ftgmac100-reset-MAC-by-SCU-on-RMII.patch \
    file://0007-net-ftgmac100-Get-link-speed-and-duplex-for-NC-SI.patch \
    file://0008-net-ncsi-check-for-netlink-driven-responses-before-r.patch \
"

#
# JFFS2 patches to workaround kernel panic.
# Highly recommended for all the OpenBMC platforms running JFFS2.
#
SRC_URI:append = " \
    file://0100-jffs2-kill-garbage-collect-thread-when-filesystem-is.patch \
    file://0101-jffs2-suppress-jffs2-messages-when-reading-inode.patch \
"

#
# Aspeed OTP patch used by multiple OpenBMC platforms for AST2600 ABR feature.
#
SRC_URI:append = " \
    file://0102-drivers-misc-Add-Aspeed-OTP-control-register.patch \
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
    file://0165-net-mctp-i2c-invalidate-flows-immediately-on-TX-erro.patch \
    file://0166-Add-ASPEED-AST2600-I2Cv2-controller-driver.patch \
    file://0167-i2c-ast2600-support-ssif.patch \
"

#
# PWM patches used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0170-aspeed-g6.dtsi-update-for-PWM-and-TACH-driver.patch \
"

#
# NCSI patches used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0180-meta-facebook-common-add-pcc-config.patch \
    file://0181-NCSI-Add-propety-no-channel-monitor-and-start-redo-p.patch \
    file://0182-net-ncsi-specify-maximum-package-to-probe.patch \
"

#
# UART patches used by multiple OpenBMC platforms
#
SRC_URI:append = " \
    file://0190-dmaengine-aspeed-Add-AST2600-UART-DMA-driver.patch \
    file://0191-serial-8250-Add-AST2600-UART-driver.patch \
"

#
# I3C patches used by OpenBMC platforms.
#
SRC_URI:append = " \
    file://0200-ARM-dts-aspeed-g6-Add-AST2600-I3Cs.patch \
    file://0201-i3c-ast2600-Validate-AST2600-I3C-for-MCTP-over-I3C.patch \
    file://0202-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0203-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
    file://0204-mctp-Fix-incorrect-tx-flow-invalidation-condition-in.patch \
    file://0205-i3c-dw-Add-err-messages-for-ibi_fifo_clear-handling.patch \
    file://0206-i3c-dw-handle-transfer-abort-errors-distinctly.patch \
"

#
# eMMC patch used by FBOSS OpenBMC platforms.
#
SRC_URI:append = " \
    file://0210-mmc-sdhci-of-aspeed-add-skip_probe-module-parameter.patch \
"

#
# Aspeed Soc drivers used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0220-soc-aspeed-Add-Post-Code-Control.patch \
"

#
# Device drivers for OpenBMC platforms.
#
SRC_URI:append = " \
    file://0300-To-support-MCP9600-temperature-driver.patch \
    file://0301-hwmon-max31790-support-to-config-PWM-as-TACH.patch \
    file://0302-hwmon-max31790-add-fanN_enable-for-all-fans.patch \
    file://0303-hwmon-max31790-revise-the-scale-to-write-pwm.patch \
    file://0304-hwmon-pmbus-Add-xdpe132g5c-driver.patch \
    file://0305-hwmon-Driver-for-Nuvoton-NCT7363Y.patch \
    file://0306-hwmon-new-driver-for-ISL28022-power-monitor.patch \
    file://0308-hwmon-Add-net_brcm-driver.patch \
    file://0309-Kernel6v5-Support-apml-driver.patch \
    file://0310-hwmon-ina2xx-Remove-use-of-i2c_match_id.patch \
    file://0311-hwmon-ina2xx-Add-device-tree-support-to-pass-alert-p.patch \
    file://0312-hwmon-ina2xx-Reorder-include-files-to-alphabetic-ord.patch \
    file://0313-hwmon-ina2xx-Replace-platform-data-with-device-prope.patch \
    file://0314-hwmon-ina2xx-Use-bit-operations.patch \
    file://0315-hwmon-ina2xx-Mark-regmap_config-as-const.patch \
    file://0316-hwmon-ina2xx-Use-local-regmap-pointer-if-used-more-t.patch \
    file://0317-hwmon-ina2xx-Re-initialize-chip-using-regmap-functio.patch \
    file://0318-hwmon-ina2xx-Fix-various-overflow-issues.patch \
    file://0319-hwmon-ina2xx-Consolidate-chip-initialization-code.patch \
    file://0320-hwmon-ina2xx-Set-alert-latch.patch \
    file://0321-hwmon-ina2xx-Move-ina2xx_get_value.patch \
    file://0322-hwmon-ina2xx-Convert-to-use-with_info-hwmon-API.patch \
    file://0323-hwmon-ina2xx-Pass-register-to-alert-limit-write-func.patch \
    file://0324-hwmon-ina2xx-Add-support-for-current-limits.patch \
    file://0325-hwmon-ina2xx-Use-shunt-voltage-to-calculate-current.patch \
    file://0326-hwmon-ina2xx-Add-support-for-has_alerts-configuratio.patch \
    file://0327-hwmon-ina2xx-Add-support-for-INA260.patch \
    file://0328-hwmon-ina226-Add-support-for-SY24655.patch \
    file://0329-hwmon-ina2xx-switch-regmap-cache-to-flat.patch \
    file://0330-dt-bindings-hwmon-Add-MPS-mp2869-mp29608-mp29612-mp2.patch \
    file://0331-hwmon-add-MP2869-MP29608-MP29612-and-MP29816-series-.patch \
    file://0332-hwmon-add-MP29502-driver.patch \
    file://0333-hwmon-ina238-add-ina237-support.patch \
    file://0334-hwmon-ina238-Add-support-for-SQ52206.patch \
    file://0335-hwmon-pmbus-Add-support-for-the-MP5998-controllers.patch \
    file://0336-iio-adc-rtq6056-Correct-the-sign-bit-index.patch \
"

#
# Backport UBIFS fixes.
#
SRC_URI:append = " \
    file://0600-ubifs-Fix-memory-leak-of-bud-log_hash.patch \
    file://0601-ubifs-ubifs_link-Fix-wrong-name-len-calculating-when.patch \
    file://0602-ubifs-Check-c-dirty_-n-p-n_cnt-and-c-nroot-state-und.patch \
    file://0603-ubifs-fix-sort-function-prototype.patch \
    file://0604-ubifs-Remove-unreachable-code-in-dbg_check_ltab_lnum.patch \
    file://0605-ubifs-dbg_check_idx_size-Fix-kmemleak-if-loading-zno.patch \
    file://0606-ubifs-Queue-up-space-reservation-tasks-if-retrying-m.patch \
    file://0607-ubifs-Fix-unattached-xattr-inode-if-powercut-happens.patch \
    file://0608-ubifs-Fix-adding-orphan-entry-twice-for-the-same-ino.patch \
    file://0609-ubifs-Fix-space-leak-when-powercut-happens-in-linkin.patch \
    file://0610-ubifs-Fix-unattached-inode-when-powercut-happens-in-.patch \
    file://0611-ubifs-dbg_orphan_check-Fix-missed-key-type-checking.patch \
    file://0612-ubifs-add-check-for-crypto_shash_tfm_digest.patch \
    file://0613-ubifs-Don-t-add-xattr-inode-into-orphan-area.patch \
    file://0614-ubifs-Remove-insert_dead_orphan-from-replaying-orpha.patch \
"

#
# Misc kernel tweaks for OpenBMC platforms.
#
SRC_URI:append = " \
    file://0500-Revert-linux-kernel-patch-usb-gadget-f_ecm-Add-suspe.patch \
"

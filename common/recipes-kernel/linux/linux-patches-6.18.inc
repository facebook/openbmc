FILESEXTRAPATHS:prepend := "${THISDIR}/6.18:"

#
# Patches to be upstreamed soon.
#
SRC_URI:append = " \
    file://0001-ARM-dts-aspeed-Add-Facebook-fblite-r1-BMC.patch \
    file://0002-ARM-dts-aspeed-Add-device-tree-for-Facebook-meru-AST.patch \
"

#
# JTAG patches.
# Used by multiple OpenBMC platforms for firmware upgrade.
#
SRC_URI:append = " \
    file://0100-drivers-jtag-Add-JTAG-core-driver.patch \
    file://0101-dt-binding-jtag-Aspeed-2400-and-2500-series.patch \
    file://0102-Add-Aspeed-SoC-24xx-and-25xx-families-JTAG-master-dr.patch \
    file://0103-jtag-aspeed-support-sysfs-interface.patch \
    file://0104-jtag-support-driver-specific-ioctl-commands.patch \
    file://0105-jtag-aspeed-support-JTAG_RUN_CYCLE-ioctl-command.patch \
    file://0106-jtag-aspeed-add-AST2600-support.patch \
    file://0107-Documentation-jtag-Add-ABI-documentation.patch \
    file://0108-Documentation-jtag-Add-JTAG-core-driver-ioctl-number.patch \
    file://0109-drivers-jtag-Add-JTAG-core-driver-Maintainers.patch \
    file://0110-fixup-jtag-patch-series.patch \
    file://0111-ARM-dts-aspeed-g6-add-jtag-controllers.patch \
    file://0112-ARM-dts-aspeed-g5-add-jtag-controller.patch \
"

#
# ASPEED controller related patches.
#
SRC_URI:append = " \
    file://0200-spi-aspeed-add-shutdown-path-for-AST25XX-SPI-control.patch \
    file://0201-spi-add-user-mode-aspeed-spi-driver.patch \
    file://0202-aspeed-i2c-add-clock-duty-cycle-property.patch \
    file://0203-dt-bindings-aspeed-i2c-add-properties-for-setting-i2.patch \
    file://0204-i2c-aspeed-update-ast2400-timing-settings.patch \
    file://0205-i2c-add-a-slave-backend-to-receive-and-queue-message.patch \
    file://0206-i2c-aspeed-add-i2c-slave-inactive-timeout-support.patch \
    file://0207-Add-ASPEED-AST2600-I2Cv2-controller-driver.patch \
    file://0208-ARM-dts-aspeed-g6-Add-AST2600-I3Cs.patch \
    file://0209-dt-bindings-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB.patch \
    file://0210-i3c-i3c-hub-Add-Renesas-RG3MxxB12A1-I3C-HUB-driver.patch \
    file://0211-aspeed-g6.dtsi-update-for-PWM-and-TACH-driver.patch \
    file://0212-ARM-dts-aspeed-g6-add-pcc-config-in-aspeed-g6.dtsi.patch \
    file://0213-dmaengine-aspeed-Add-AST2600-UART-DMA-driver.patch \
    file://0214-serial-8250-Add-AST2600-UART-driver.patch \
    file://0215-drivers-misc-Add-Aspeed-OTP-control-register.patch \
    file://0216-soc-aspeed-lpc-pcc-Add-PCC-controller-support.patch \
    file://0217-i2c-ast2600-support-ssif.patch \
    file://0218-ARM-dts-aspeed-g6-Add-UDMA-node.patch \
    file://0219-net-ftgmac100-reset-MAC-by-SCU-on-RMII.patch \
    file://0220-net-ncsi-check-for-netlink-driven-responses-before-r.patch \
    file://0221-i3c-ast2600-Validate-AST2600-I3C-for-MCTP-over-I3C.patch \
    file://0222-i3c-dw-Add-err-messages-for-ibi_fifo_clear-handling.patch \
    file://0223-i3c-dw-handle-transfer-abort-errors-distinctly.patch \
    file://0224-i3c-dw-Handle-unexpected-responses-caused-by-SDA-noi.patch \
    file://0225-i3c-handle-null-transfers-during-response-processing.patch \
    file://0226-i3c-master-ast2600-Fix-missing-of_node-for-virtual-I.patch \
    file://0227-i3c-dw-Improve-IBI-queue-handling-when-encountering-.patch \
    file://0228-i3c-master-dw-i3c-master-Fix-race-condition-in-halt-.patch \
"

#
# NCSI patches used by multiple OpenBMC platforms.
#
SRC_URI:append = " \
    file://0500-NCSI-Add-propety-no-channel-monitor-and-start-redo-p.patch \
    file://0501-net-ncsi-specify-maximum-package-to-probe.patch \
"

#
# hwmon/sensor drivers
#
SRC_URI:append = " \
    file://0600-hwmon-max31790-support-to-config-PWM-as-TACH.patch \
    file://0601-hwmon-max31790-add-fanN_enable-for-all-fans.patch \
    file://0602-hwmon-pmbus-Add-xdpe132g5c-driver.patch \
    file://0603-hwmon-Add-net_brcm-driver.patch \
    file://0604-Kernel6v5-Support-apml-driver.patch \
    file://0605-apml-Add-support-for-SP7-platform.patch \
    file://0606-hwmon-pmbus-tda38640-Add-identification-for-PMBUS_VO.patch \
"

#
# Hacks and workarounds
#
SRC_URI:append = " \
    file://0900-jffs2-kill-garbage-collect-thread-when-filesystem-is.patch \
    file://0901-jffs2-suppress-jffs2-messages-when-reading-inode.patch \
    file://0902-mmc-sdhci-of-aspeed-add-skip_probe-module-parameter.patch \
    file://0903-usb-gadget-f_ecm-disable-suspend-resume-feature.patch \
"

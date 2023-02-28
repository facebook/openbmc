FILESEXTRAPATHS:prepend := "${THISDIR}/6.1:"

SRC_URI:append = " \    
    file://0100-dt-bindings-Add-bindings-for-aspeed-pwm-tach.patch \
    file://0101-pwm-Add-Aspeed-ast2600-PWM-support.patch \
    file://0102-Porting-tach-driver-for-ast26xx-from-Aspeed-linux.patch \
    file://0103-aspeed-i2c-add-clock-duty-cycle-property.patch \
    file://0104-dt-bindings-aspeed-i2c-add-properties-for-setting-i2.patch \
    file://0105-drivers-misc-Add-Aspeed-OTP-control-register.patch \
    file://0106-drivers-jtag-Add-JTAG-core-driver.patch \
    file://0107-dt-binding-jtag-Aspeed-2400-and-2500-series.patch \
    file://0108-Add-Aspeed-SoC-24xx-and-25xx-families-JTAG-master-dr.patch \
    file://0109-jtag-aspeed-support-sysfs-interface.patch \
    file://0110-jtag-support-driver-specific-ioctl-commands.patch \
    file://0111-jtag-aspeed-support-JTAG_RUN_CYCLE-ioctl-command.patch \
    file://0112-jtag-aspeed-add-AST2600-support.patch \
    file://0113-Documentation-jtag-Add-ABI-documentation.patch \
    file://0114-Documentation-jtag-Add-JTAG-core-driver-ioctl-number.patch \
    file://0115-drivers-jtag-Add-JTAG-core-driver-Maintainers.patch \
    file://0116-fixup-jtag-patch-series.patch \
    file://0117-ARM-dts-aspeed-g6-add-jtag-controllers.patch \
    file://0118-ARM-dts-aspeed-g5-add-jtag-controller.patch \
    file://0119-jffs2-kill-garbage-collect-thread-when-filesystem-is.patch \
    file://0120-jffs2-suppress-jffs2-messages-when-reading-inode.patch \
    file://0121-mmc-sdhci-of-aspeed-add-skip_probe-module-parameter.patch \
    file://0122-spi-add-user-mode-aspeed-spi-driver.patch \
    file://0123-Kernel-5.15-i2c-add-a-slave-backend-to-receive-and-q.patch \
    file://0124-kernel-5.15-add-i2c-slave-inactive-timeout-support-3.patch \
    file://0125-port-xdpe132g5c-pmbus-driver-from-linux-5.10-to-6.0.patch \
    file://0126-Added-shutdown-path-in-spi_aspeed_smc-driver.patch \
"

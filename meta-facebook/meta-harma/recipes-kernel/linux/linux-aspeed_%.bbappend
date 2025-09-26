FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"

SRC_URI:append = " file://harma-local.cfg"

SRC_URI:append = " \
    file://1000-ARM-dts-aspeed-Harma-add-lpc_pcc-device.patch \
    file://1001-ARM-dts-aspeed-Harma-stop-NIC-run-time-polling.patch \
    file://1002-ARM-dts-aspeed-Harma-read-cpu-temp-and-power.patch \
    file://1003-ARM-dts-aspeed-Harma-set-ncsi-package-equal-to-1.patch \
    file://1004-ARM-dts-aspeed-Harma-Setting-i3c-to-i2c-device.patch \
    file://1005-ARM-dts-aspeed-Harma-enable-uart-dma-mode.patch \
    file://1006-ARM-dts-aspeed-Harma-revise-i2c0-and-i2c2-duty-cycle.patch \
    file://1007-ARM-dts-aspeed-Harma-enable-I2Cv2-driver-and-fine-tu.patch \
    file://1008-ARM-dts-aspeed-Harma-Enable-i2c-slave-timeout.patch \
    file://1009-ARM-dts-aspeed-harma-add-new-line-between-the-child-.patch \
    file://1010-ARM-dts-aspeed-harma-use-power-monitor-instead-of-po.patch \
    file://1011-ARM-dts-aspeed-harma-add-sq52206-power-monitor-devic.patch \
    file://1012-ARM-dts-aspeed-harma-add-retimer-sgpio.patch \
    file://1013-ARM-dts-aspeed-harma-add-mctp-i2c-controller-node.patch \
"

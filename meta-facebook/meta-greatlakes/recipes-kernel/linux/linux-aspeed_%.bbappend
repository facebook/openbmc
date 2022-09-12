FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append = " \
    file://aspeed-bmc-facebook-greatlakes.dts;subdir=git/arch/${ARCH}/boot/dts \
    file://0001-Revise-aspeed-2600-dtsi.patch \
    file://0002-Add-jtag-driver.patch \
    file://0003-Add-pwm-tach-driver.patch \
    file://0004-wdt-aspeed-Reorder-extend-operation.patch \
"
KERNEL_DEVICETREE = "aspeed-bmc-facebook-greatlakes.dtb"

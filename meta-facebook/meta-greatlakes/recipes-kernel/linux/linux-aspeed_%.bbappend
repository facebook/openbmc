FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI:append = " \
    file://aspeed-bmc-facebook-greatlakes.dts;subdir=git/arch/${ARCH}/boot/dts \
"
KERNEL_DEVICETREE = "aspeed-bmc-facebook-greatlakes.dtb"

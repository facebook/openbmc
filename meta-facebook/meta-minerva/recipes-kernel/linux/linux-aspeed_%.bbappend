FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
    file://aspeed-bmc-facebook-minerva.dts \
"

do_configure:append(){
    cp ${WORKDIR}/aspeed-bmc-facebook-minerva.dts ${S}/arch/arm/boot/dts/aspeed
    echo "dtb-\$(CONFIG_ARCH_ASPEED) += aspeed-bmc-facebook-minerva.dtb" >> ${S}/arch/arm/boot/dts/aspeed/Makefile
}

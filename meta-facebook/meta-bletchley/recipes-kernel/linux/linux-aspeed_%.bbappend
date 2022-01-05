FILESEXTRAPATHS:prepend := "${THISDIR}/linux-aspeed:"
SRC_URI += " \
            file://0001-dt-bindings-Add-bindings-for-aspeed-pwm-tach.patch \
            file://0002-pwm-Add-Aspeed-ast2600-PWM-support.patch \
            file://0003-Porting-tach-driver-for-ast26xx-from-Aspeed-linux.patch \
            file://0004-Add-support-for-mp5023-device-from-Monolithic-Power-.patch \
            file://0005-arch-arm-dts-update-bletchley-dts.patch \
            file://0006-mtd-spi-nor-winbond-Add-support-for-w25q01jv-iq.patch \
            file://0007-tpm-fix-NPE-on-probe-for-missing-device.patch \
            file://0008-mtd-aspeed-smc-improve-probe-resilience.patch \
           "


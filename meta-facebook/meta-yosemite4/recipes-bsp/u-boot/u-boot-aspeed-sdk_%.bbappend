FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0500-arm-dts-ast2600-evb-disable-MDIO-function.patch \
            file://0501-ARM-Aspeed-add-a-config-for-FMC_WDT2-timer-reload-va.patch \
            file://0502-arm-dts-ast2600-evb-Enable-alternate-boot.patch \
            file://fmc-wdt2-timer.cfg \
           "


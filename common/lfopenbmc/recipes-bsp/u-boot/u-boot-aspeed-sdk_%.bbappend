FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

require u-boot-version-override.inc

SRC_URI:append:openbmc-fb-lf:aspeed-g6 = "\
        file://fmc-wdt2-timer.cfg \
        file://0500-ARM-Aspeed-add-a-config-for-FMC_WDT2-timer-reload-va.patch \
        file://0501-mem_test-Support-to-save-mtest-result-in-environm.patch \
        file://0502-configs-openbmc-Revise-size-of-environment-and-us.patch \
        file://0503-mtd-spi-Add-support-for-W25Q01JVSFIN.patch \
        "

SRC_URI:append:openbmc-fb-lf:aspeed-g7 = " \
    file://0100-ARM-uboot-dts-add-facebook-common-dts.patch \
    file://0500-ARM64-Aspeed2700-add-a-config-for-WDTA-timer-reload-.patch \
    file://wdta-timer.cfg \
"

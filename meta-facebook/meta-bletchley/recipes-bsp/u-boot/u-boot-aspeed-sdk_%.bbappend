FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"
SRC_URI +=" \
    file://fmc-wdt2-timer.cfg \
    file://0001-ARM-Aspeed-add-a-config-for-FMC_WDT2-timer-reload-va.patch \
    "

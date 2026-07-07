FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://ast2700_facebook_bootmcu.overlay \
    file://0001-mcu-runtime-sdram_ast2700-Update-tREFI-accordingly.patch;patchdir=aspeed-zephyr-project \
    file://0002-mcu-runtime-ast2700-Return-status-from-fpga-phy-init.patch;patchdir=aspeed-zephyr-project \
    file://0003-mcu-runtime-sdram_ast2700-Add-sram-parameter-WA.patch;patchdir=aspeed-zephyr-project \
    file://0004-mcu-runtime-sdram_ast2700-Improve-RX-eye-height.patch;patchdir=aspeed-zephyr-project \
    file://0005-mcu-runtime-sdram_ast2700-Move-wdt-dram-sw-reset-to-.patch;patchdir=aspeed-zephyr-project \
"

BOOTMCU_EXTRA_DTC_OVERLAY_FILE ?= "${UNPACKDIR}/ast2700_facebook_bootmcu.overlay"

EXTRA_OECMAKE:append = " \
    -DEXTRA_DTC_OVERLAY_FILE="${BOOTMCU_EXTRA_DTC_OVERLAY_FILE}" \
"

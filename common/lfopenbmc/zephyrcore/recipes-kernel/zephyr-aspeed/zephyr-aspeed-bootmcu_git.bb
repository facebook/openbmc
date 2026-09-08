require recipes-kernel/zephyr-kernel/zephyr-image.inc
require zephyr-aspeed-src.inc
require zephyr-aspeed-project-src.inc

SUMMARY = "BootMCU runtime firmware"
PACKAGE_ARCH = "${MACHINE_ARCH}"

PROVIDES += "virtual/bootmcu"
PV = "1.0+git"

ZEPHYR_BOARD_BOOTMCU ??= "ast2700_evb/ast2700/bootmcu"
ZEPHYR_BOARD = "${ZEPHYR_BOARD_BOOTMCU}"
ZEPHYR_MAKE_OUTPUT += "${BOOTMCU_FMC_BINARY} ${BOOTMCU_FW_BINARY}"

ZEPHYR_SRC_DIR ??= "${S}/aspeed-zephyr-project/apps/mcu-runtime"

SRC_URI:append:yosemite5a7 = " file://ast2700_yosemite5a7_bootmcu.overlay \
                             "
SRC_URI:append:rainiera7 = " file://ast2700_rainiera7_bootmcu.overlay \
                             "
SRC_URI:append:ventura2a7 = " file://ast2700_ventura2a7_bootmcu.overlay \
                             "

BOOTMCU_EXTRA_DTC_OVERLAY_FILE:append:yosemite5a7 = ";${UNPACKDIR}/ast2700_yosemite5a7_bootmcu.overlay"
BOOTMCU_EXTRA_DTC_OVERLAY_FILE:append:rainiera7 = ";${UNPACKDIR}/ast2700_rainiera7_bootmcu.overlay"
BOOTMCU_EXTRA_DTC_OVERLAY_FILE:append:ventura2a7 = ";${UNPACKDIR}/ast2700_ventura2a7_bootmcu.overlay"

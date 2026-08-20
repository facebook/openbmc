# Refer to meta-zephyr/meta-zephyr-core/recipes-kernel/zephyr-kernel/zephyr-kernel-src-x.x.x.inc
# creating this file

ZEPHYR_BRANCH = "aspeed-main-v3.7.0"
# Tag for v00.03.08
SRCREV_zephyr = "6f1d80dd2d3f23d81b9d266411f99fb0b473b796"
SRCREV_mbedtls = "be1dd5b11d8f59af0ab0995985e348a429999013"
SRCREV_cmsis = "4b96cbb174678dcd3ca86e11e1f24bc5f8726da0"

SRC_URI_ZEPHYR = "git://github.com/AspeedTech-BMC/zephyr;protocol=https"
SRC_URI_ZEPHYR_MBEDTLS = "git://github.com/zephyrproject-rtos/mbedtls;protocol=https"
SRC_URI_ZEPHYR_CMSIS = "git://github.com/zephyrproject-rtos/cmsis;protocol=https"

SRC_URI = "\
    ${SRC_URI_ZEPHYR};name=zephyr;branch=${ZEPHYR_BRANCH};destsuffix=${S}/zephyr \
    ${SRC_URI_ZEPHYR_MBEDTLS};name=mbedtls;nobranch=1;destsuffix=${S}/modules/crypto/mbedtls \
    ${SRC_URI_ZEPHYR_CMSIS};name=cmsis;nobranch=1;destsuffix=${S}/modules/hal/cmsis \
"

ZEPHYR_MODULES = "\
${S}/modules/crypto/mbedtls\;\
${S}/modules/hal/cmsis\;\
"

# Override meta-zephyr default image naming
ZEPHYR_IMAGE_LINK_NAME = "${PN}"

INSANE_SKIP:${PN} += "arch"

INHIBIT_SYSROOT_STRIP = "1"
INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

DEFAULT_PREFERENCE = "-1"
require u-boot-common-aspeed-sdk_${PV}.inc

UBOOT_MAKE_TARGET ?= "DEVICE_TREE=${UBOOT_DEVICETREE}"

require recipes-bsp/u-boot/u-boot-aspeed.inc

PROVIDES += "u-boot"
DEPENDS += "bc-native dtc-native"

SRC_URI:append:df-phosphor-mmc = " ${@bb.utils.contains('MACHINE_FEATURES', 'mf-ufs', \
                'file://u-boot-env-ast2700-ufs.txt', 'file://u-boot-env-ast2700.txt', d)}"

UBOOT_ENV_SIZE:df-phosphor-mmc = "0x20000"
UBOOT_ENV:df-phosphor-mmc = "u-boot-env"
UBOOT_ENV_SUFFIX:df-phosphor-mmc = "bin"
UBOOT_ENV_TXT:df-phosphor-mmc = "${@bb.utils.contains('MACHINE_FEATURES', 'mf-ufs', \
                'u-boot-env-ast2700-ufs.txt', 'u-boot-env-ast2700.txt', d)}"

do_compile:append() {
    if [ -n "${UBOOT_ENV}" ]
    then
        # Generate default environment image
        # add -r parameter if wants redundant environment image
        ${B}/tools/mkenvimage -s ${UBOOT_ENV_SIZE} -o ${B}/${UBOOT_ENV_BINARY} ${UNPACKDIR}/${UBOOT_ENV_TXT}
    fi
}

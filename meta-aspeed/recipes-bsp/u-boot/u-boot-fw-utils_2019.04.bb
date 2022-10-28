require u-boot-common.inc

PV = "v2019.04+git${SRCPV}"
SRC_URI += "file://fw_env.config \
            file://fw_env.config.64k \
			"
SRCBRANCH = "openbmc/helium/v2019.04"
include common/recipes-bsp/u-boot-fbobmc/use-intree-shipit.inc

SUMMARY = "U-Boot bootloader fw_printenv/setenv utilities"
DEPENDS += "mtd-utils"

INSANE_SKIP:${PN} = "already-stripped"
EXTRA_OEMAKE:class-target = 'CROSS_COMPILE=${TARGET_PREFIX} CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" V=1'
EXTRA_OEMAKE:class-cross = 'HOSTCC="${CC} ${CFLAGS} ${LDFLAGS}" V=1'

inherit uboot-config

# default fw_env config file to be installed
FW_ENV_CONFIG_FILE ??= "fw_env.config"
do_compile () {
	oe_runmake -C ${S} ${UBOOT_MACHINE} O=${B}
	oe_runmake -C ${S} envtools O=${B}
}

do_install () {
	install -d ${D}${base_sbindir}
	install -d ${D}${sysconfdir}
	install -m 755 ${B}/tools/env/fw_printenv ${D}${base_sbindir}/fw_printenv
	install -m 755 ${B}/tools/env/fw_printenv ${D}${base_sbindir}/fw_setenv
	bbdebug 1 "install ${FW_ENV_CONFIG_FILE}"
	install -m 0644 ${WORKDIR}/${FW_ENV_CONFIG_FILE} ${D}${sysconfdir}/fw_env.config
}

do_install:class-cross () {
	install -d ${D}${bindir_cross}
	install -m 755 ${B}/tools/env/fw_printenv ${D}${bindir_cross}/fw_printenv
	install -m 755 ${B}/tools/env/fw_printenv ${D}${bindir_cross}/fw_setenv
}

SYSROOT_DIRS:append:class-cross = " ${bindir_cross}"

PACKAGE_ARCH = "${MACHINE_ARCH}"
BBCLASSEXTEND = "cross"

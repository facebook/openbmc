SUMMARY = "U-Boot bootloader image creation tool"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=a2c678cfd4a4d97135585cad908541c6"
SECTION = "bootloader"

DEPENDS = "openssl"

SRCBRANCH = "openbmc/helium/v2016.07"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/theopolis/u-boot.git;branch=${SRCBRANCH};protocol=https \
           file://fw_env.config \
          "

PV = "v2016.07"
S = "${WORKDIR}/git"

EXTRA_OEMAKE_class-target = 'CROSS_COMPILE="${TARGET_PREFIX}" CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'
EXTRA_OEMAKE_class-native = 'CC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'
EXTRA_OEMAKE_class-nativesdk = 'CROSS_COMPILE="${HOST_PREFIX}" CC="${CC} ${CFLAGS} ${LDFLAGS}" HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}" STRIP=true V=1'

do_compile () {
  oe_runmake sandbox_defconfig
  oe_runmake cross_tools NO_SDL=1
}

do_install () {
  install -d ${D}${bindir}
  install -m 0755 tools/mkimage ${D}${bindir}/uboot-mkimage
  ln -sf uboot-mkimage ${D}${bindir}/mkimage
}

BBCLASSEXTEND = "native nativesdk"

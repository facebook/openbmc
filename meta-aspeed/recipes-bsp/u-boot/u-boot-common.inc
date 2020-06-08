HOMEPAGE = "http://www.denx.de/wiki/U-Boot/WebHome"
DESCRIPTION = "U-Boot, a boot loader for Embedded boards based on PowerPC, \
ARM, MIPS and several other processors, which can be installed in a boot \
ROM and used to initialize and test the hardware or to download and run \
application code."
SECTION = "bootloaders"
DEPENDS += "flex-native bison-native"

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=30503fd321432fc713238f582193b78e"
PE = "1"

SRCREV = "AUTOINC"
SRC_URI = "git://github.com/facebook/openbmc-uboot.git;branch=${SRCBRANCH};protocol=https \
          "

S = "${WORKDIR}/git"
# Improve code quality.
EXTRA_OEMAKE += 'KCFLAGS="-Werror"'

# To support defconfig segmentation
DEPENDS += "kern-tools-native"
do_configure_prepend() {
  if [ -n "${UBOOT_CONFIG_BASE}" -a -e ${WORKDIR}/${UBOOT_MACHINE}.append  ]; then
    bbnote "Append ${UBOOT_MACHINE}.append to ${UBOOT_CONFIG_BASE} to generate ${UBOOT_MACHINE}"
    KCONFIG_CONFIG=${S}/configs/${UBOOT_MACHINE} \
      merge_config.sh -m ${S}/configs/${UBOOT_CONFIG_BASE} ${WORKDIR}/${UBOOT_MACHINE}.append
  fi
}



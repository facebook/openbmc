HOMEPAGE = "http://www.denx.de/wiki/U-Boot/WebHome"
DESCRIPTION = "U-Boot, a boot loader for Embedded boards based on PowerPC, \
ARM, MIPS and several other processors, which can be installed in a boot \
ROM and used to initialize and test the hardware or to download and run \
application code."
SECTION = "bootloaders"
DEPENDS += "flex-native bison-native"

LICENSE = "GPL-2.0-or-later"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=30503fd321432fc713238f582193b78e"
PE = "1"

SRC_URI = "git://github.com/facebook/openbmc-uboot.git;branch=${SRCBRANCH};protocol=https"
SRCREV = "AUTOINC"
S = "${WORKDIR}/git"

# u-boot v2020.01, which is used in Dunfell and above, has different settings
# than earlier u-boot versions.  Set these variables so that we can use older
# u-boot versions with newer Yocto versions.
B = "${WORKDIR}/build"
do_configure[cleandirs] = "${B}"
UBOOT_INITIAL_ENV = ""
# ---- ^ Dunfell and above workarounds.

# Improve code quality.
EXTRA_OEMAKE += 'KCFLAGS="-Werror"'

# To support defconfig segmentation
DEPENDS += "kern-tools-native"

# To support verified boot
require verified-boot.inc

# Reservice space in DTB for inject kek
EXTRA_OEMAKE += "${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", \
  'DTC_FLAGS="-S 5120"', "", d) }"

SPL_BINARY = "${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", \
  "spl/u-boot-spl.${UBOOT_SUFFIX}", "", d) }"
SPL_BINARYNAME = "u-boot-spl.${UBOOT_SUFFIX}"
SPL_IMAGE = "u-boot-spl-${MACHINE}-${PV}-${PR}.${UBOOT_SUFFIX}"
SPL_SYMLINK = "u-boot-spl-${MACHINE}.${UBOOT_SUFFIX}"

# Recovery U-Boot is only built when using verified-boot.
UBOOT_RECOVERY_BINARYNAME ?= "u-boot-recovery.${UBOOT_SUFFIX}"
UBOOT_RECOVERY_IMAGE ?= "u-boot-recovery-${MACHINE}-${PV}-${PR}.${UBOOT_SUFFIX}"
UBOOT_RECOVERY_SYMLINK ?= "u-boot-recovery-${MACHINE}.${UBOOT_SUFFIX}"

# Normalize the UBOOT_MACHINE "xxx_config" into "xxx_defconfig"
python () {
  uboot_machine = d.getVar("UBOOT_MACHINE", True)
  if not uboot_machine.endswith("_defconfig"):
    canonical_uboot_machine = uboot_machine.split("_")[0] + "_defconfig"
    bb.debug(1, "Change UBOOT_MACHINE from %s to Canonical %s " % (uboot_machine, canonical_uboot_machine))
    d.setVar("UBOOT_MACHINE", canonical_uboot_machine)
}

# Merge the defconfig segment and tweak for verified boot
do_configure:prepend() {
  do_configure_merge_and_tweak
}

do_configure_merge_and_tweak() {
  if echo ${PN} | grep -q "u-boot-tools"; then
    bbnote "Skip configure merge and tweak for ${PN}"
    return
  fi

  if [ -n "${UBOOT_CONFIG_BASE}" -a -e ${WORKDIR}/${UBOOT_MACHINE}.append  ]; then
    bbnote "Append ${UBOOT_MACHINE}.append to ${UBOOT_CONFIG_BASE} to generate ${UBOOT_MACHINE}"
    KCONFIG_CONFIG=${S}/configs/${UBOOT_MACHINE} \
      merge_config.sh -m ${S}/configs/${UBOOT_CONFIG_BASE} ${WORKDIR}/${UBOOT_MACHINE}.append
  fi
  bbnote "Turn off CONFIG_ASPEED_RECOVERY_BUILD for normal build"
  defconfig_option_off CONFIG_ASPEED_RECOVERY_BUILD ${S}/configs/${UBOOT_MACHINE}

  if [ "x${VERIFIED_BOOT}" != "x" ]; then
    bbnote "Use default setting of TEXT_BASE in Kconfig"
    defconfig_option_off CONFIG_SYS_TEXT_BASE ${S}/configs/${UBOOT_MACHINE}
    bbnote "Turn on CONFIG_SPL for verified-boot"
    defconfig_option_on CONFIG_SPL ${S}/configs/${UBOOT_MACHINE}
    bbnote "Turn off CONFIG_PFR_SUPPORT for verified-boot"
    defconfig_option_off CONFIG_PFR_SUPPORT ${S}/configs/${UBOOT_MACHINE}
  else
    bbnote "Turn off CONFIG_SPL for non-vboot"
    defconfig_option_off CONFIG_SPL ${S}/configs/${UBOOT_MACHINE}
  fi
}

# For u-boot only customized compile, intall and deploy task

# the do_compile will only update .scmversion when u-boot source changed
# the UBOOT_LOCALVERSION change won't update .scmversion because
# the .scmversion file already exists.
# Thus UBOOT_LOCALVERSION += "${OPENBMC_VERSION}" won't help
#
# prepend action which updates .scmversion with OPENBMC_VERSION
# to do_compile to make sure when OPENBMC_VERSION update get into u-boot
do_compile:prepend:bld-uboot() {
  echo " ${OPENBMC_VERSION}" > ${S}/.scmversion
  echo " ${OPENBMC_VERSION}" > ${B}/.scmversion
}

# build recovery u-boot when verified-boot enabled
do_compile:append:bld-uboot() {
  if [ "x${VERIFIED_BOOT}" != "x" ] ; then
    bbnote "Build the Recovery u-boot for verfifed boot"
    defconfig_option_on CONFIG_ASPEED_RECOVERY_BUILD ${S}/configs/${UBOOT_MACHINE}
    oe_runmake -C ${S} O=${B}-recovery ${UBOOT_MACHINE}
    oe_runmake -C ${S} O=${B}-recovery ${UBOOT_MAKE_TARGET}
  fi
}

do_install:append:bld-uboot() {
  if [ "x${VERIFIED_BOOT}" != "x" ]; then
    install -d ${D}/boot
    install -m 644 ${B}-recovery/${UBOOT_BINARY} ${D}/boot/${UBOOT_RECOVERY_IMAGE}
    ln -sf ${UBOOT_RECOVERY_IMAGE} ${D}/boot/${UBOOT_RECOVERY_BINARYNAME}
  fi
}

do_deploy:append:bld-uboot() {
  if [ "x${VERIFIED_BOOT}" != "x" ]; then
    install -d ${DEPLOYDIR}
    install -m 644 ${B}-recovery/${UBOOT_BINARY} ${DEPLOYDIR}/${UBOOT_RECOVERY_IMAGE}
    cd ${DEPLOYDIR}
    rm -f ${UBOOT_RECOVERY_BINARYNAME} ${UBOOT_RECOVERY_SYMLINK}
    ln -sf ${UBOOT_RECOVERY_IMAGE} ${UBOOT_RECOVERY_SYMLINK}
    ln -sf ${UBOOT_RECOVERY_IMAGE} ${UBOOT_RECOVERY_BINARYNAME}
  fi
}

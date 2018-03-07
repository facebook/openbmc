SUMMARY = "Universal Boot Loader for embedded devices"
HOMEPAGE = "http://www.denx.de/wiki/U-Boot/WebHome"
SECTION = "bootloader"
PROVIDES = "virtual/bootloader"

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=a2c678cfd4a4d97135585cad908541c6"

DEPENDS += "dtc-native bc-native"

SRCBRANCH = "openbmc/helium/v2016.07"
SRCREV = "AUTOINC"

SRC_URI = "git://github.com/theopolis/u-boot.git;branch=${SRCBRANCH};protocol=https \
           file://fw_env.config \
           file://fw_env.config.full \
          "

PV = "v2016.07"
S = "${WORKDIR}/git"

FILES_${PN} = "${sysconfdir}"

inherit uboot-config deploy
require verified-boot.inc

EXTRA_OEMAKE = 'CROSS_COMPILE=${TARGET_PREFIX} CC="${TARGET_PREFIX}gcc ${TOOLCHAIN_OPTIONS}" V=1'
EXTRA_OEMAKE += 'HOSTCC="${BUILD_CC} ${BUILD_CFLAGS} ${BUILD_LDFLAGS}"'

# Improve code quality.
EXTRA_OEMAKE += 'KCFLAGS="-Werror"'

PACKAGECONFIG ??= "openssl"
# u-boot will compile its own tools during the build, with specific
# configurations (aka when CONFIG_FIT_SIGNATURE is enabled) openssl is needed as
# a host build dependency.
PACKAGECONFIG[openssl] = ",,openssl-native"

# Allow setting an additional version string that will be picked up by the
# u-boot build system and appended to the u-boot version.  If the .scmversion
# file already exists it will not be overwritten.
UBOOT_LOCALVERSION ?= " "

# Some versions of u-boot use .bin and others use .img.  By default use .bin
# but enable individual recipes to change this value.
UBOOT_SUFFIX ??= "bin"
UBOOT_IMAGE ?= "u-boot-${MACHINE}-${PV}-${PR}.${UBOOT_SUFFIX}"
UBOOT_BINARY ?= "u-boot.${UBOOT_SUFFIX}"
UBOOT_SYMLINK ?= "u-boot-${MACHINE}.${UBOOT_SUFFIX}"
UBOOT_MAKE_TARGET ?= "all"

# Recovery U-Boot is only built when using verified-boot.
UBOOT_RECOVERY_BINARYNAME ?= "u-boot-recovery.${UBOOT_SUFFIX}"
UBOOT_RECOVERY_IMAGE ?= "u-boot-recovery-${MACHINE}-${PV}-${PR}.${UBOOT_SUFFIX}"
UBOOT_RECOVERY_SYMLINK ?= "u-boot-recovery-${MACHINE}.${UBOOT_SUFFIX}"

# Output the ELF generated. Some platforms can use the ELF file and directly
# load it (JTAG booting, QEMU) additionally the ELF can be used for debugging
# purposes.
UBOOT_ELF ?= ""
UBOOT_ELF_SUFFIX ?= "elf"
UBOOT_ELF_IMAGE ?= "u-boot-${MACHINE}-${PV}-${PR}.${UBOOT_ELF_SUFFIX}"
UBOOT_ELF_BINARY ?= "u-boot.${UBOOT_ELF_SUFFIX}"
UBOOT_ELF_SYMLINK ?= "u-boot-${MACHINE}.${UBOOT_ELF_SUFFIX}"

# Some versions of u-boot build an SPL (Second Program Loader) image that
# should be packaged along with the u-boot binary as well as placed in the
# deploy directory.  For those versions they can set the following variables
# to allow packaging the SPL.
SPL_BINARY ?= "spl/u-boot-spl.${UBOOT_SUFFIX}"
SPL_BINARYNAME ?= "u-boot-spl.${UBOOT_SUFFIX}"
SPL_IMAGE ?= "u-boot-spl-${MACHINE}-${PV}-${PR}.${UBOOT_SUFFIX}"
SPL_SYMLINK ?= "u-boot-spl-${MACHINE}.${UBOOT_SUFFIX}"

# Additional environment variables or a script can be installed alongside
# u-boot to be used automatically on boot.  This file, typically 'uEnv.txt'
# or 'boot.scr', should be packaged along with u-boot as well as placed in the
# deploy directory.  Machine configurations needing one of these files should
# include it in the SRC_URI and set the UBOOT_ENV parameter.
UBOOT_ENV_SUFFIX ?= "txt"
UBOOT_ENV ?= ""
UBOOT_ENV_BINARY ?= "${UBOOT_ENV}.${UBOOT_ENV_SUFFIX}"
UBOOT_ENV_IMAGE ?= "${UBOOT_ENV}-${MACHINE}-${PV}-${PR}.${UBOOT_ENV_SUFFIX}"
UBOOT_ENV_SYMLINK ?= "${UBOOT_ENV}-${MACHINE}.${UBOOT_ENV_SUFFIX}"

do_compile () {
    if [ "${@bb.utils.contains('DISTRO_FEATURES', 'ld-is-gold', 'ld-is-gold', '', d)}" = "ld-is-gold" ] ; then
        sed -i 's/$(CROSS_COMPILE)ld$/$(CROSS_COMPILE)ld.bfd/g' config.mk
    fi

    unset LDFLAGS
    unset CFLAGS
    unset CPPFLAGS

    if [ ! -e ${B}/.scmversion -a ! -e ${S}/.scmversion ]
    then
        echo ${UBOOT_LOCALVERSION} > ${B}/.scmversion
        echo ${OPENBMC_VERSION} >> ${B}/.scmversion
        echo ${UBOOT_LOCALVERSION} > ${S}/.scmversion
        echo ${OPENBMC_VERSION} >> ${S}/.scmversion
    fi

    if [ "x${UBOOT_CONFIG}" != "x" ]
    then
        for config in ${UBOOT_MACHINE}; do
            i=`expr $i + 1`;
            for type  in ${UBOOT_CONFIG}; do
                j=`expr $j + 1`;
                if [ $j -eq $i ]
                then
                    oe_runmake O=${config} ${config}
                    oe_runmake O=${config} ${UBOOT_MAKE_TARGET}
                    cp  ${S}/${config}/${UBOOT_BINARY}  ${S}/${config}/u-boot-${type}.${UBOOT_SUFFIX}
                fi
            done
            unset  j
        done
        unset  i
    else
        UBOOT_CONFIGNAME=${UBOOT_MACHINE}
        UBOOT_CONFIGNAME=$(echo ${UBOOT_CONFIGNAME} | sed -e 's/_config/_defconfig/')

        # Always turn off the recovery build.
        defconfig_option_off CONFIG_ASPEED_RECOVERY_BUILD ${B}/configs/${UBOOT_CONFIGNAME}

        if [ "x${VERIFIED_BOOT}" != "x" ] ; then
            defconfig_option_on CONFIG_SPL ${B}/configs/${UBOOT_CONFIGNAME}
            defconfig_option_on CONFIG_SPL_FIT_SIGNATURE ${B}/configs/${UBOOT_CONFIGNAME}
            defconfig_option_on CONFIG_OF_CONTROL ${B}/configs/${UBOOT_CONFIGNAME}
            defconfig_option_on CONFIG_OF_EMBED ${B}/configs/${UBOOT_CONFIGNAME}
        else
            defconfig_option_off CONFIG_SPL ${B}/configs/${UBOOT_CONFIGNAME}
            defconfig_option_off CONFIG_SPL_FIT_SIGNATURE ${B}/configs/${UBOOT_CONFIGNAME}
            defconfig_option_off CONFIG_OF_CONTROL ${B}/configs/${UBOOT_CONFIGNAME}
            defconfig_option_off CONFIG_OF_EMBED ${B}/configs/${UBOOT_CONFIGNAME}
        fi

        oe_runmake O=default ${UBOOT_MACHINE}
        DTC_FLAGS="-S 5120" oe_runmake O=default ${UBOOT_MAKE_TARGET}

        # Finally, the verified-boot builds a second 'recovery' U-Boot.
        if [ "x${VERIFIED_BOOT}" != "x" ] ; then
            defconfig_option_on CONFIG_ASPEED_RECOVERY_BUILD ${B}/configs/${UBOOT_CONFIGNAME}
            oe_runmake O=recovery ${UBOOT_MACHINE}
            oe_runmake O=recovery ${UBOOT_MAKE_TARGET}
        fi
    fi

}

do_install () {
    if [ -e ${WORKDIR}/fw_env.config ] ; then
        install -d ${D}${sysconfdir}
        install -m 644 ${WORKDIR}/fw_env.config ${D}${sysconfdir}/fw_env.config
    fi
}

do_deploy () {
    if [ "x${UBOOT_CONFIG}" != "x" ]
    then
        for config in ${UBOOT_MACHINE}; do
            i=`expr $i + 1`;
            for type in ${UBOOT_CONFIG}; do
                j=`expr $j + 1`;
                if [ $j -eq $i ]
                then
                    install -d ${DEPLOYDIR}
                    install ${S}/${config}/u-boot-${type}.${UBOOT_SUFFIX} ${DEPLOYDIR}/u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX}
                    cd ${DEPLOYDIR}
                    ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX} ${UBOOT_SYMLINK}-${type}
                    ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX} ${UBOOT_SYMLINK}
                    ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX} ${UBOOT_BINARY}-${type}
                    ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_SUFFIX} ${UBOOT_BINARY}
                fi
            done
            unset  j
        done
        unset  i
    else
        install -d ${DEPLOYDIR}
        install ${S}/default/${UBOOT_BINARY} ${DEPLOYDIR}/${UBOOT_IMAGE}
        cd ${DEPLOYDIR}
        rm -f ${UBOOT_BINARY} ${UBOOT_SYMLINK}
        ln -sf ${UBOOT_IMAGE} ${UBOOT_SYMLINK}
        ln -sf ${UBOOT_IMAGE} ${UBOOT_BINARY}
   fi

    if [ "x${UBOOT_ELF}" != "x" ]
    then
        if [ "x${UBOOT_CONFIG}" != "x" ]
        then
            for config in ${UBOOT_MACHINE}; do
                i=`expr $i + 1`;
                for type in ${UBOOT_CONFIG}; do
                    j=`expr $j + 1`;
                    if [ $j -eq $i ]
                    then
                        install ${S}/${config}/${UBOOT_ELF} ${DEPLOYDIR}/u-boot-${type}-${PV}-${PR}.${UBOOT_ELF_SUFFIX}
                        ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_ELF_SUFFIX} ${DEPLOYDIR}/${UBOOT_ELF_BINARY}-${type}
                        ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_ELF_SUFFIX} ${DEPLOYDIR}/${UBOOT_ELF_BINARY}
                        ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_ELF_SUFFIX} ${DEPLOYDIR}/${UBOOT_ELF_SYMLINK}-${type}
                        ln -sf u-boot-${type}-${PV}-${PR}.${UBOOT_ELF_SUFFIX} ${DEPLOYDIR}/${UBOOT_ELF_SYMLINK}
                    fi
                done
                unset j
            done
            unset i
        else
            install ${S}/default/${UBOOT_ELF} ${DEPLOYDIR}/${UBOOT_ELF_IMAGE}
            ln -sf ${UBOOT_ELF_IMAGE} ${DEPLOYDIR}/${UBOOT_ELF_BINARY}
            ln -sf ${UBOOT_ELF_IMAGE} ${DEPLOYDIR}/${UBOOT_ELF_SYMLINK}
        fi
    fi


     if [ "x${SPL_BINARY}" != "x" ]
     then
         if [ "x${UBOOT_CONFIG}" != "x" ]
         then
             for config in ${UBOOT_MACHINE}; do
                 i=`expr $i + 1`;
                 for type in ${UBOOT_CONFIG}; do
                     j=`expr $j + 1`;
                     if [ $j -eq $i ]
                     then
                         install ${S}/${config}/${SPL_BINARY} ${DEPLOYDIR}/${SPL_IMAGE}-${type}-${PV}-${PR}
                         rm -f ${DEPLOYDIR}/${SPL_BINARYNAME} ${DEPLOYDIR}/${SPL_SYMLINK}-${type}
                         ln -sf ${SPL_IMAGE}-${type}-${PV}-${PR} ${DEPLOYDIR}/${SPL_BINARYNAME}-${type}
                         ln -sf ${SPL_IMAGE}-${type}-${PV}-${PR} ${DEPLOYDIR}/${SPL_BINARYNAME}
                         ln -sf ${SPL_IMAGE}-${type}-${PV}-${PR} ${DEPLOYDIR}/${SPL_SYMLINK}-${type}
                         ln -sf ${SPL_IMAGE}-${type}-${PV}-${PR} ${DEPLOYDIR}/${SPL_SYMLINK}
                     fi
                 done
                 unset  j
             done
             unset  i
         elif [ "x${VERIFIED_BOOT}" != "x" ] ; then
             install ${S}/default/${SPL_BINARY} ${DEPLOYDIR}/${SPL_IMAGE}
             rm -f ${DEPLOYDIR}/${SPL_BINARYNAME} ${DEPLOYDIR}/${SPL_SYMLINK}
             ln -sf ${SPL_IMAGE} ${DEPLOYDIR}/${SPL_BINARYNAME}
             ln -sf ${SPL_IMAGE} ${DEPLOYDIR}/${SPL_SYMLINK}
         fi
     fi

    if [ "x${VERIFIED_BOOT}" != "x" ]; then
        install ${S}/recovery/${UBOOT_BINARY} ${DEPLOYDIR}/${UBOOT_RECOVERY_IMAGE}
        rm -f ${DEPLOYDIR}/${UBOOT_RECOVERY_BINARYNAME} ${DEPLOYDIR}/${UBOOT_RECOVERY_SYMLINK}
        ln -sf ${UBOOT_RECOVERY_IMAGE} ${DEPLOYDIR}/${UBOOT_RECOVERY_BINARYNAME}
        ln -sf ${UBOOT_RECOVERY_IMAGE} ${DEPLOYDIR}/${UBOOT_RECOVERY_SYMLINK}
    fi

    if [ "x${UBOOT_ENV}" != "x" ]
    then
        install ${WORKDIR}/${UBOOT_ENV_BINARY} ${DEPLOYDIR}/${UBOOT_ENV_IMAGE}
        rm -f ${DEPLOYDIR}/${UBOOT_ENV_BINARY} ${DEPLOYDIR}/${UBOOT_ENV_SYMLINK}
        ln -sf ${UBOOT_ENV_IMAGE} ${DEPLOYDIR}/${UBOOT_ENV_BINARY}
        ln -sf ${UBOOT_ENV_IMAGE} ${DEPLOYDIR}/${UBOOT_ENV_SYMLINK}
    fi
}

addtask deploy before do_build after do_compile

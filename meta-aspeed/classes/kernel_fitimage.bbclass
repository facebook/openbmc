# Common code for generating u-boot FIT binary images for u-boot.
# Also generate an image for the OpenBMC ROM and upgradable flash.
#
# Copyright (C) 2016-Present, Facebook, Inc.

def image_types_module(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro == 'fido' or distro == 'krogoth':
        return 'image_types_uboot'
    return 'image_types'

# Inherit u-boot classes if legacy uboot images are in use.
IMAGE_TYPE_MODULE = '${@image_types_module(d)}'
inherit ${IMAGE_TYPE_MODULE}

# Make Rocko images work just like they would in krogoth
# That means, let conversion on u-boot call into oe_mkimage so
# we may do our fit conversions.
# TODO:
# More up-stream-worthy method is to use the chaining of conversions.
CONVERSION_CMD_u-boot = "oe_mkimage ${IMAGE_NAME}.rootfs.${type} none"

require recipes-bsp/u-boot/verified-boot.inc

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
IMAGE_FSTYPES += "cpio.lzma.u-boot"

# U-Boot is placed at an absolute position relative to the start of the first
# U-Boot FIT. This value, like most others, depends on U-Boot machine
# configurations.
ROM_UBOOT_POSITION ?= "0x4000"
ROM_UBOOT_LOADADDRESS ?= "0x28084000"

# Set sizes and offsets in KB:
#   For kernel and rootfs: 16384
#   For kernel only: 2690
FLASH_SIZE ?= "16384"

UBOOT_SOURCE ?= "${DEPLOY_DIR_IMAGE}/u-boot-${MACHINE}.${UBOOT_SUFFIX}"
FIT_SOURCE ?= "${STAGING_DIR_HOST}/etc/fit-${MACHINE}.its"

FIT[vardepsexclude] = "DATETIME"
FIT ?= "fit-${MACHINE}-${DATETIME}.itb"
FIT_LINK ?= "fit-${MACHINE}.itb"

FLASH_IMAGE[vardepsexclude] += "DATETIME"
FLASH_IMAGE ?= "flash-${MACHINE}-${DATETIME}"
FLASH_IMAGE_LINK ?= "flash-${MACHINE}"

# ROM-based boot variables
UBOOT_RECOVERY_SOURCE ?= "${DEPLOY_DIR_IMAGE}/u-boot-recovery-${MACHINE}.${UBOOT_SUFFIX}"
UBOOT_SPL_SOURCE ?= "${DEPLOY_DIR_IMAGE}/u-boot-spl-${MACHINE}.${UBOOT_SUFFIX}"
UBOOT_FIT_SOURCE ?= "${STAGING_DIR_HOST}/etc/u-boot-fit-${MACHINE}.its"

UBOOT_FIT[vardepsexclude] = "DATETIME"
UBOOT_FIT ?= "u-boot-fit-${MACHINE}-${DATETIME}.itb"
UBOOT_FIT_LINK ?= "u-boot-fit-${MACHINE}.itb"

IMAGE_PREPROCESS_COMMAND += " generate_data_mount_dir ; "
IMAGE_POSTPROCESS_COMMAND += " flash_image_generate ; "

generate_data_mount_dir() {
    mkdir -p "${IMAGE_ROOTFS}/mnt/data"
}

flash_image_generate() {
    FIT_DESTINATION="${DEPLOY_DIR_IMAGE}/${FIT}"
    FLASH_IMAGE_DESTINATION="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE}"
    UBOOT_FIT_DESTINATION="${DEPLOY_DIR_IMAGE}/${UBOOT_FIT}"

    if [ ! -f $UBOOT_SOURCE ]; then
        echo "U-boot file ${UBOOT_SOURCE} does not exist"
        return 1
    fi

    if [ ! -f $FIT_DESTINATION ]; then
        echo "FIT binary file ${FIT_DESTINATION} does not exist"
        return 1
    fi

    rm -rf $FLASH_IMAGE_DESTINATION
    dd if=/dev/zero of=${FLASH_IMAGE_DESTINATION} bs=1k count=${FLASH_SIZE}

    if [ "x${VERIFIED_BOOT}" != "x" ] ; then
        FLASH_UBOOT_RECOVERY_OFFSET=84
        FLASH_UBOOT_OFFSET=512
        FLASH_FIT_OFFSET=896

        # Write the recovery U-Boot.
        dd if=${UBOOT_RECOVERY_SOURCE} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_UBOOT_RECOVERY_OFFSET} conv=notrunc

        # Write an intermediate FIT containing only U-Boot.
        dd if=${UBOOT_FIT_DESTINATION} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_UBOOT_OFFSET} conv=notrunc
        ln -sf ${UBOOT_FIT} ${DEPLOY_DIR_IMAGE}/${UBOOT_FIT_LINK}
    else
        FLASH_UBOOT_OFFSET=0
        FLASH_FIT_OFFSET=512

        # Write U-Boot directly to the start of the flash.
        dd if=${UBOOT_SOURCE} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_UBOOT_OFFSET} conv=notrunc
    fi

    dd if=${FIT_DESTINATION} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_FIT_OFFSET} conv=notrunc

    if [ "x${VERIFIED_BOOT}" != "x" ] ; then
      dd if=${UBOOT_SPL_SOURCE} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=0 conv=notrunc
    fi

    ln -sf ${FLASH_IMAGE} ${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_LINK}
    ln -sf ${FIT} ${DEPLOY_DIR_IMAGE}/${FIT_LINK}
}

oe_mkimage() {
    FIT_DESTINATION="${DEPLOY_DIR_IMAGE}/${FIT}"
    UBOOT_FIT_DESTINATION="${DEPLOY_DIR_IMAGE}/${UBOOT_FIT}"

    if [ "x${VERIFIED_BOOT}" != "x" ] ; then
      rm -f ${UBOOT_FIT_SOURCE}
      fitimage_emit_fit_header ${UBOOT_FIT_SOURCE}

      # Step 1: Prepare a firmware image section
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} imagestart
      fitimage_emit_section_firmware ${UBOOT_FIT_SOURCE} 1 "${UBOOT_SOURCE}" \
          ${ROM_UBOOT_LOADADDRESS} ${ROM_UBOOT_LOADADDRESS}
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} sectend

      # Step 2: Prepare a configurations section
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} confstart
      fitimage_emit_section_firmware_config ${UBOOT_FIT_SOURCE} 1
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} sectend
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} fitend

      mkimage -f ${UBOOT_FIT_SOURCE} -E -p ${ROM_UBOOT_POSITION} ${UBOOT_FIT_DESTINATION}
    fi

    KERNEL_FILE="${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}"
    RAMDISK_FILE="${DEPLOY_DIR_IMAGE}/$1"
    # Newer oe-core (>Krogoth) copies the image after this step.
    # Hence we need to use the path from ${IMGDEPLOYDIR}.
    if [ ! -e ${RAMDISK_FILE} ]; then
      RAMDISK_FILE="${IMGDEPLOYDIR}/$1"
    fi

    rm -f ${FIT_SOURCE}
    fitimage_emit_fit_header ${FIT_SOURCE}

    # Step 1: Prepare a kernel image section
    fitimage_emit_section_maint ${FIT_SOURCE} imagestart
    fitimage_emit_section_kernel ${FIT_SOURCE} 1 "${KERNEL_FILE}" "none" \
        ${UBOOT_LOADADDRESS} ${UBOOT_ENTRYPOINT}

    # Step 2: Prepare a ramdisk image section
    fitimage_emit_section_ramdisk ${FIT_SOURCE} 1 ${RAMDISK_FILE} "lzma"

    # Step 3: Prepare dtb image section (if needed)
    if test -n "${KERNEL_DEVICETREE}"; then
        dtbcount=1
        DFT_DTB_COUNTER=1
        for DTB in ${KERNEL_DEVICETREE}; do
            if echo ${DTB} | grep -q '/dts/'; then
                bbwarn "${DTB} contains the full path to the the dts file, but only the dtb name should be used."
                DTB=`basename ${DTB} | sed 's,\.dts$,.dtb,g'`
            fi
            DTB_PATH="${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}-${DTB}"
            fitimage_emit_section_dtb ${FIT_SOURCE} ${dtbcount} ${DTB_PATH}
            dtbcount=`expr ${dtbcount} + 1`
        done
    else
        DFT_DTB_COUNTER=""
    fi
    fitimage_emit_section_maint ${FIT_SOURCE} sectend

    # Step 4: Prepare a configurations section
    fitimage_emit_section_maint ${FIT_SOURCE} confstart
    fitimage_emit_section_config ${FIT_SOURCE} 1 1 ${DFT_DTB_COUNTER}
    fitimage_emit_section_maint ${FIT_SOURCE} sectend
    fitimage_emit_section_maint ${FIT_SOURCE} fitend

    # Step 5: Assemble the image
    mkimage -f ${FIT_SOURCE} ${FIT_DESTINATION}
}

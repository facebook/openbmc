# Common code for generating u-boot FIT binary images for u-boot.
# Also generate an image for the OpenBMC ROM and upgradable flash.
#
# Copyright (C) 2016-Present, Facebook, Inc.

inherit image_types_uboot
require recipes-bsp/u-boot/verified-boot.inc

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
IMAGE_FSTYPES += "cpio.lzma.u-boot"
UBOOT_IMAGE_LOADADDRESS ?= "0x80008000"
UBOOT_IMAGE_ENTRYPOINT ?= "0x80008000"
UBOOT_RAMDISK_LOADADDRESS ?= "0x80800000"

# Set sizes and offsets in KB:
#   For kernel and rootfs: 16384
#   For kernel only: 2690
FLASH_SIZE ?= "16384"
ROM_SIZE ?= "1024"
FLASH_UBOOT_OFFSET ?= "0"
FLASH_FIT_OFFSET ?= "512"

UBOOT_SOURCE ?= "${DEPLOY_DIR_IMAGE}/u-boot-${MACHINE}.${UBOOT_SUFFIX}"
FIT_SOURCE ?= "${STAGING_DIR_HOST}/etc/fit-${MACHINE}.its"

FIT[vardepsexclude] = "DATETIME"
FIT ?= "fit-${MACHINE}-${DATETIME}.itb"
FIT_LINK ?= "fit-${MACHINE}.itb"

FLASH_IMAGE[vardepsexclude] += "DATETIME"
FLASH_IMAGE ?= "flash-${MACHINE}-${DATETIME}"
FLASH_IMAGE_LINK ?= "flash-${MACHINE}"

# ROM-based boot variables
UBOOT_SPL_SOURCE ?= "${DEPLOY_DIR_IMAGE}/u-boot-spl-${MACHINE}"
UBOOT_FIT_SOURCE ?= "${STAGING_DIR_HOST}/etc/u-boot-fit-${MACHINE}.its"

UBOOT_FIT[vardepsexclude] = "DATETIME"
UBOOT_FIT ?= "u-boot-fit-${MACHINE}-${DATETIME}.itb"
UBOOT_FIT_LINK ?= "u-boot-fit-${MACHINE}.itb"

ROM_IMAGE[vardepsexclude] = "DATETIME"
ROM_IMAGE ?= "rom-${MACHINE}-${DATETIME}"
ROM_IMAGE_LINK ?= "rom-${MACHINE}"

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

    if [ "x${ROM_BOOT}" != "x" ] ; then
        # Write an intermediate FIT containing only U-Boot.
        dd if=${UBOOT_FIT_DESTINATION} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_UBOOT_OFFSET} conv=notrunc
        ln -sf ${UBOOT_FIT} ${DEPLOY_DIR_IMAGE}/${UBOOT_FIT_LINK}
    else
        # Write U-Boot directly to the start of the flash.
        dd if=${UBOOT_SOURCE} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_UBOOT_OFFSET} conv=notrunc
    fi

    dd if=${FIT_DESTINATION} of=${FLASH_IMAGE_DESTINATION} bs=1k seek=${FLASH_FIT_OFFSET} conv=notrunc

    if [ "x${ROM_BOOT}" != "x" ] ; then
      ROM_IMAGE_DESTINATION="${DEPLOY_DIR_IMAGE}/${ROM_IMAGE}"
      dd if=/dev/zero of=${ROM_IMAGE_DESTINATION} bs=1k count=${ROM_SIZE}

      dd if=${UBOOT_SPL_SOURCE} of=${ROM_IMAGE_DESTINATION} bs=1k seek=0 conv=notrunc
      ln -sf ${ROM_IMAGE} ${DEPLOY_DIR_IMAGE}/${ROM_IMAGE_LINK}
    fi

    ln -sf ${FLASH_IMAGE} ${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_LINK}
    ln -sf ${FIT} ${DEPLOY_DIR_IMAGE}/${FIT_LINK}
}

oe_mkimage() {
    FIT_DESTINATION="${DEPLOY_DIR_IMAGE}/${FIT}"
    UBOOT_FIT_DESTINATION="${DEPLOY_DIR_IMAGE}/${UBOOT_FIT}"

    if [ "x${ROM_BOOT}" != "x" ] ; then
      rm -f ${UBOOT_FIT_SOURCE}
      fitimage_emit_fit_header ${UBOOT_FIT_SOURCE}

      # Step 1: Prepare a firmware image section
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} imagestart
      fitimage_emit_section_firmware ${UBOOT_FIT_SOURCE} 1 "${UBOOT_SOURCE}" \
          "0x28008000" "0x28008000"
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} sectend

      # Step 2: Prepare a configurations section
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} confstart
      fitimage_emit_section_firmware_config ${UBOOT_FIT_SOURCE} 1
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} sectend
      fitimage_emit_section_maint ${UBOOT_FIT_SOURCE} fitend

      mkimage -f ${UBOOT_FIT_SOURCE} -E -p 0x8000 ${UBOOT_FIT_DESTINATION}
    fi

    KERNEL_FILE="${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}"
    RAMDISK_FILE="${DEPLOY_DIR_IMAGE}/$1"

    rm -f ${FIT_SOURCE}
    fitimage_emit_fit_header ${FIT_SOURCE}

    # Step 1: Prepare a kernel image section
    fitimage_emit_section_maint ${FIT_SOURCE} imagestart
    fitimage_emit_section_kernel ${FIT_SOURCE} 1 "${KERNEL_FILE}" "none" \
        ${UBOOT_IMAGE_LOADADDRESS} ${UBOOT_IMAGE_ENTRYPOINT}

    # Step 2: Prepare a ramdisk image section
    fitimage_emit_section_ramdisk ${FIT_SOURCE} 1 ${RAMDISK_FILE} "lzma" \
        ${UBOOT_RAMDISK_LOADADDRESS}
    fitimage_emit_section_maint ${FIT_SOURCE} sectend

    # Step 3: Prepare a configurations section
    fitimage_emit_section_maint ${FIT_SOURCE} confstart
    fitimage_emit_section_config ${FIT_SOURCE} 1 1
    fitimage_emit_section_maint ${FIT_SOURCE} sectend
    fitimage_emit_section_maint ${FIT_SOURCE} fitend

    # Step 4: Assemble the image
    if [ "x${VERIFIED_BOOT}" != "x" ]
    then
        # A feature could build the u-boot ROM
        # (Every build can build the ROM), this feature can decide to
        # add keys into the ROM

        # A more 'intense' feature can on-line sign the output build.
        # This can assume there are keys symlinked within your local
        # configuration directory.
        echo "You have not set up signing-keys."
        # exit 78
    fi
    mkimage -f ${FIT_SOURCE} ${FIT_DESTINATION}
}

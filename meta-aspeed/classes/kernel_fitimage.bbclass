# Common code for generating u-boot FIT binary images for u-boot.
# Also generate an image for the OpenBMC ROM and upgradable flash.
#
# Copyright (C) 2016-Present, Facebook, Inc.

inherit image_types_uboot
require recipes-bsp/verified-boot/verified-boot.inc

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
FLASH_UBOOT_OFFSET ?= "0"
FLASH_FIT_OFFSET ?= "512"

UBOOT_SOURCE ?= "${DEPLOY_DIR_IMAGE}/u-boot-${MACHINE}.${UBOOT_SUFFIX}"
FIT_SOURCE ?= "${DEPLOY_DIR_IMAGE}/fit-${MACHINE}-${DATETIME}.its"
FIT_SOURCE_LINK ?= "fit-${MACHINE}.its"
FIT_DESTINATION ?= "${DEPLOY_DIR_IMAGE}/fit-${MACHINE}-${DATETIME}.itb"
FIT_DESTINATION_LINK ?= "fit-${MACHINE}.itb"
FLASH_IMAGE ?= "flash-${MACHINE}-${DATETIME}"
FLASH_IMAGE_LINK ?= "flash-${MACHINE}"

IMAGE_PREPROCESS_COMMAND += " generate_data_mount_dir ; "
IMAGE_POSTPROCESS_COMMAND += " flash_image_generate ; "

generate_data_mount_dir() {
    mkdir -p "${IMAGE_ROOTFS}/mnt/data"
}

flash_image_generate() {
    BOOT_FILE="${DEPLOY_DIR_IMAGE}/u-boot.${UBOOT_SUFFIX}"
    FLASH_DESINTATION="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE}"

    if [ ! -f $UBOOT_SOURCE ]; then
        echo "U-boot file ${UBOOT_SOURCE} does not exist"
        return 1
    fi

    if [ ! -f $FIT_DESTINATION ]; then
        echo "FIT binary file ${FIT_DESTINATION} does not exist"
        return 1
    fi

    rm -rf $FLASH_DESINTATION
    echo "dd if=/dev/zero of=${FLASH_DESINTATION} bs=1k count=${FLASH_SIZE}"
    dd if=/dev/zero of=${FLASH_DESINTATION} bs=1k count=${FLASH_SIZE}

    echo "dd if=${UBOOT_SOURCE} of=${FLASH_DESINTATION} bs=1k seek=${FLASH_UBOOT_OFFSET} conv=notrunc"
    dd if=${UBOOT_SOURCE} of=${FLASH_DESINTATION} bs=1k seek=${FLASH_UBOOT_OFFSET} conv=notrunc

    echo "dd if=${FIT_DESTINATION} of=${FLASH_DESINTATION} bs=1k seek=${FLASH_FIT_OFFSET} conv=notrunc"
    dd if=${FIT_DESTINATION} of=${FLASH_DESINTATION} bs=1k seek=${FLASH_FIT_OFFSET} conv=notrunc

    DESTINATION_LINK="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_LINK}"
    ln -sf ${FLASH_IMAGE} ${DESTINATION_LINK}
    ln -sf ${FIT_SOURCE} ${FIT_SOURCE_LINK}
    ln -sf ${FIT_DESTINATION} ${FIT_DESTINATION_LINK}
}

oe_mkimage() {
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

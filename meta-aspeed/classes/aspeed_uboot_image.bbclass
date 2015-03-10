inherit image_types_uboot

# oe_mkimage() was defined in image_types_uboot. Howver, it does not consider
# the image load address and entry point. Override it here.

oe_mkimage () {
    mkimage -A ${UBOOT_ARCH} -O linux -T ramdisk -C $2 -n ${IMAGE_NAME} \
        -a ${UBOOT_IMAGE_LOADADDRESS} -e ${UBOOT_IMAGE_ENTRYPOINT} \
        -d ${DEPLOY_DIR_IMAGE}/$1 ${DEPLOY_DIR_IMAGE}/$1.u-boot
}

UBOOT_IMAGE_ENTRYPOINT ?= "0x42000000"
UBOOT_IMAGE_LOADADDRESS ?= "${UBOOT_IMAGE_ENTRYPOINT}"

# 24M
IMAGE_ROOTFS_SIZE = "24576"
# and don't put overhead behind my back
IMAGE_OVERHEAD_FACTOR = "1"

IMAGE_PREPROCESS_COMMAND += " generate_data_mount_dir ; "
IMAGE_POSTPROCESS_COMMAND += " flash_image_generate ; "

FLASH_IMAGE_NAME ?= "flash-${MACHINE}-${DATETIME}"
FLASH_IMAGE_LINK ?= "flash-${MACHINE}"
# 16M
FLASH_SIZE ?= "16384"
FLASH_UBOOT_OFFSET ?= "0"
# 512k
FLASH_KERNEL_OFFSET ?= "512"
# 3M
FLASH_ROOTFS_OFFSET ?= "3072"

flash_image_generate() {
  kernelfile="${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}"
  ubootfile="${DEPLOY_DIR_IMAGE}/u-boot.${UBOOT_SUFFIX}"
  # rootfs has to match the type defined in IMAGE_FSTYPES"
  rootfs="${DEPLOY_DIR_IMAGE}/${IMAGE_LINK_NAME}.cpio.lzma.u-boot"
  if [ ! -f $kernelfile ]; then
    echo "Kernel file ${kernelfile} does not exist"
    return 1
  fi
  if [ ! -f $ubootfile ]; then
    echo "U-boot file ${ubootfile} does not exist"
    return 1
  fi
  if [ ! -f $rootfs ]; then
    echo "Rootfs file ${rootfs} does not exist"
    return 1
  fi
  dst="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_NAME}"
  rm -rf $dst
  dd if=/dev/zero of=${dst} bs=1k count=${FLASH_SIZE}
  dd if=${ubootfile} of=${dst} bs=1k seek=${FLASH_UBOOT_OFFSET}
  dd if=${kernelfile} of=${dst} bs=1k seek=${FLASH_KERNEL_OFFSET}
  dd if=${rootfs} of=${dst} bs=1k seek=${FLASH_ROOTFS_OFFSET}
  dstlink="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_LINK}"
  rm -rf $dstlink
  ln -sf ${FLASH_IMAGE_NAME} $dstlink
}

generate_data_mount_dir() {
  mkdir -p "${IMAGE_ROOTFS}/mnt/data"
}

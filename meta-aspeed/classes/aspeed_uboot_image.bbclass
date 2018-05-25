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

oe_mkimage () {
    ramdisk="${DEPLOY_DIR_IMAGE}/$1"
    if [ ! -e ${ramdisk} ]; then
      ramdisk="${IMGDEPLOYDIR}/$1"
    fi
    mkimage -A ${UBOOT_ARCH} -O linux -T ramdisk -C $2 -n ${IMAGE_NAME} \
        -d ${ramdisk} ${ramdisk}.u-boot
}

# 24M
IMAGE_ROOTFS_SIZE = "24576"
# and don't put overhead behind my back
IMAGE_OVERHEAD_FACTOR = "1"

IMAGE_PREPROCESS_COMMAND += " generate_data_mount_dir ; "
IMAGE_POSTPROCESS_COMMAND += " flash_image_generate ; "

FLASH_IMAGE_NAME[vardepsexclude] += "DATETIME"
FLASH_IMAGE_NAME ?= "flash-${MACHINE}-${DATETIME}"
FLASH_IMAGE_LINK ?= "flash-${MACHINE}"

# 512k
FLASH_KERNEL_OFFSET ?= "512"
# 4.5M
FLASH_ROOTFS_OFFSET ?= "4608"

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
    new_rootfs="${IMGDEPLOYDIR}/${IMAGE_LINK_NAME}.cpio.lzma.u-boot"
    if [ ! -f $new_rootfs ]; then
      echo "Rootfs file ${rootfs} does not exist"
      return 1
    fi
    cp -f $new_rootfs ${DEPLOY_DIR_IMAGE}/
  fi
  dst="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_NAME}"
  rm -rf $dst
  dd if=${ubootfile} of=${dst} bs=1k
  dd if=${kernelfile} of=${dst} bs=1k seek=${FLASH_KERNEL_OFFSET}
  dd if=${rootfs} of=${dst} bs=1k seek=${FLASH_ROOTFS_OFFSET}
  dstlink="${DEPLOY_DIR_IMAGE}/${FLASH_IMAGE_LINK}"
  rm -rf $dstlink
  ln -sf ${FLASH_IMAGE_NAME} $dstlink
}

generate_data_mount_dir() {
  mkdir -p "${IMAGE_ROOTFS}/mnt/data"
}

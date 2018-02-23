inherit kernel_fitimage

# Base this image on core-image-minimal
include recipes-core/images/core-image-minimal.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
# However, the current u-boot does not have lzma enabled. Stick to gz
# until we generate a new u-boot image.
IMAGE_FSTYPES += "cpio.lzma.u-boot"
#UBOOT_IMAGE_ENTRYPOINT = "0x40800000"
UBOOT_IMAGE_ENTRYPOINT = "0x40008000"
UBOOT_RAMDISK_LOADADDRESS = "0x40800000"
UBOOT_IMAGE_LOADADDRESS = "0x40008000"
FLASH_ROOTFS_OFFSET = "4608"



# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  healthd \
  fbutils \
  fan-util \
  fscd \
  watchdog-ctrl \
  sensor-setup \
  ipmid \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ipmbd \
  bic-cached \
  bic-util \
  yosemite-sensors \
  sensor-util \
  sensor-mon \
  gpiod \
  gpiointrd \
  front-paneld \
  power-util \
  mterm\
  cfg-util \
  fw-util \
  fpc-util \
  me-util \
  log-util \
  lldp-util \
  spatula \
  openbmc-utils \
  conf-util \
  ncsi-util \
  "

IMAGE_FEATURES += " \
  ssh-server-openssh \
  tools-debug \
  "

DISTRO_FEATURES += " \
  ext2 \
  ipv6 \
  nfs \
  usbgadget \
  usbhost \
  "

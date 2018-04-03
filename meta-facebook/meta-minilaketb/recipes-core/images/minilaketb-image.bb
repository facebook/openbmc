# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).

# UBOOT_IMAGE_LOADADDRESS = "0x80008000"
# UBOOT_IMAGE_ENTRYPOINT = "0x80008000"
UBOOT_RAMDISK_LOADADDRESS = "0x80800000"

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  healthd \
  plat-utils \
  fan-util \
  fscd \
  watchdog-ctrl \
  sensor-setup \
  ipmid \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ipmbd \
  bic-cached \
  bic-util \
  ncsi-util \
  minilaketb-sensors \
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
  ipmi-util \
  guid-util \
  bios-util \
  threshold-util \
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

# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
IMAGE_FSTYPES += "cpio.lzma.u-boot"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  at93cx6-util \
  ast-mdio \
  bcm5396-util \
  bic-cached \
  bic-util \
  bitbang \
  cpldupdate \
  cpldupdate-i2c \
  crashdump \
  e2fsprogs \
  flashrom \
  front-paneld \
  fscd \
  fw-util \
  gpiod \
  ipmbd \
  ipmid \
  libcpldupdate-dll-echo \
  libcpldupdate-dll-gpio \
  lldp-util \
  log-util \
  mterm \
  openbmc-gpio \
  openbmc-utils \
  psu-util \
  sensor-setup \
  sensor-util \
  sensor-mon \
  spatula \
  threshold-util \
  trousers \
  tpm-tools \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
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

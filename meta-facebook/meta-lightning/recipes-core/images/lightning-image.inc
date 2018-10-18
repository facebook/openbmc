# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Changing the image compression from gz to lzma achieves 30% saving (~3M).
IMAGE_FSTYPES += "cpio.lzma.u-boot"
FLASH_ROOTFS_OFFSET = "4608"


NTP_PKGS = " \
  ntp \
  ntp-utils \
  sntp \
  ntpdate \
  "

# Include modules in rootfs
IMAGE_INSTALL += " \
  openbmc-utils \
  fbutils \
  ipmid \
  healthd \
  usb-console \
  sensor-util \
  sensor-mon \
  packagegroup-openbmc-base \
  packagegroup-openbmc-python3 \
  ${NTP_PKGS} \
  iproute2 \
  dhcp-client \
  fruid \
  log-util \
  fan-util \
  front-paneld \
  fpc-util \
  fscd \
  packagegroup-openbmc-rest3 \
  lldp-util \
  spatula \
  enclosure-util \
  fw-util \
  openbmc-gpio \
  switchtec-user \
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

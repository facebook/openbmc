# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

FLASH_ROOTFS_OFFSET = "4608"


NTP_PKGS = " \
  ntp \
  ntpq \
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

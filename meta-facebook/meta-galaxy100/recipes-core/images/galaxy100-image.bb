# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  at93cx6-util \
  bcm5396-util \
  bitbang \
  cpldupdate \
  flashrom \
  ipmid \
  lldp-util \
  mterm \
  memtester  \
  mkeeprom  \
  openbmc-utils \
  openbmc-gpio \
  po-eeprom \
  kcsd \
  spatula \
  stress  \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  watchdogd \
  systemd-networkd \
  udev-rules \
  "

IMAGE_INSTALL_remove += "init-ifupdown"
SYSVINIT_SCRIPTS_remove += "init-ifupdown"

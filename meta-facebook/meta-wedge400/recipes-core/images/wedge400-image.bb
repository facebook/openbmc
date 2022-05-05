# Copyright 2019-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require wedge400-image-layout.inc

require recipes-core/images/fb-openbmc-image.bb

PROVIDES += "wedge400orv3-image"

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  at93cx6-util \
  ast-mdio \
  bcm5396-util \
  bic-cache \
  bic-monitor \
  bic-util \
  bitbang \
  cpldupdate \
  cpldupdate-jtag \
  crashdump \
  e2fsprogs \
  fscd \
  flashrom \
  front-paneld \
  fw-util \
  i2craw \
  gbi2ctool \
  gpiocli \
  ipmbd \
  ipmid \
  ipmitool \
  libcpldupdate-dll-ast-jtag \
  libfruid \
  lldp-util \
  log-util-v2 \
  mdio-util \
  me-util \
  mterm \
  openbmc-gpio \
  openbmc-utils \
  pemd \
  pem-util \
  psu-util \
  sensor-util \
  sensor-mon \
  threshold-util \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  rackmon \
  psu-update \
  ftdicmd \
  "

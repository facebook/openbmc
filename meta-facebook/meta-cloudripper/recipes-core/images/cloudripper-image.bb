# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require cloudripper-image-layout.inc
require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  bic-cache \
  bic-monitor \
  bic-util \
  cpldupdate \
  cpldupdate-jtag \
  libcpldupdate-dll-ast-jtag \
  fio \
  flashrom \
  front-paneld \
  fscd \
  fw-util \
  i2craw \
  ipmid \
  ipmbd \
  ipmitool \
  log-util-v2 \
  lldp-util \
  mdio-util \
  mterm \
  openbmc-utils \
  psu-util \
  rest-api \
  sensor-util \
  sensor-mon \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  "

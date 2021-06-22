# Copyright 2020-present Delta Electronics, Inc. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require agc032a-image-layout.inc
require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  at93cx6-util \
  ast-mdio \
  bitbang \
  cpldupdate \
  cpldupdate-jtag \
  crashdump \
  e2fsprogs \
  fscd \
  fan-util \
  flashrom \
  fruid \
  gpiocli \
  healthd \
  ipmid \
  ipmitool \
  ipmi-util \
  libcpldupdate-dll-echo \
  libcpldupdate-dll-gpio \
  libcpldupdate-dll-ast-jtag \
  libfruid \
  libpldm \
  lldp-util \
  log-util-v2 \
  libncsi \
  ncsi-util \
  ncsid \
  kcsd \
  openbmc-gpio \
  openbmc-utils \
  psu-util \
  sensor-util \
  sensor-mon \
  threshold-util \
  wedge-eeprom \
  weutil-dhcp-id \
  "

IMAGE_INSTALL_remove += "libaggregate-sensor-ptest"

SERIAL_CONSOLES += "115200;ttyS4"

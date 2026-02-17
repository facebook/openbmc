# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require elbert-image-layout.inc

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
# Some notes about kcsd:
#   - if "ipmitool" failed with "/dev/ipmi* not found" error in x86, you
#     may need to load "ipmi_si" module in x86 with "ports=0xca2,0xca8"
#     module parameter.
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  ast-mdio \
  cpldupdate \
  libcpldupdate-dll-ioctl \
  fio \
  flashrom \
  fscd \
  front-paneld \
  led-controld \
  lldp-util \
  mterm \
  openbmc-utils \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  kcsd \
  ipmid \
  ipmitool \
  guid-util \
  psu-util \
  sensor-util \
  sensor-mon \
  threshold-util \
  "

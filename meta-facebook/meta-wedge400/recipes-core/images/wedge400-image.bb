# Copyright 2019-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

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
  cpldupdate-jtag \
  crashdump \
  e2fsprogs \
  fscd \
  flashrom \
  front-paneld \
  fw-util \
  gbi2ctool \
  gpiocli \
  gpiod \
  ipmbd \
  ipmid \
  ipmitool \
  libcpldupdate-dll-ast-jtag \
  libfruid \
  lldp-util \
  log-util \
  me-util \
  mterm \
  openbmc-gpio \
  openbmc-utils \
  pemd \
  pem-util \
  psu-util \
  sensor-util \
  sensor-mon \
  spatula \
  threshold-util \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  rackmon \
  "

def tpm_tools_recipe(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  if distro == 'rocko':
    return 'tpm2.0-tools'
  return 'tpm2-tools'

IMAGE_INSTALL += "${@tpm_tools_recipe(d)}"

# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fuji-image-layout.inc
require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  ipmid \
  ipmbd \
  ipmitool \
  bic-cache \
  bic-util \
  bitbang \
  cpldupdate \
  cpldupdate-i2c \
  cpldupdate-jtag \
  libcpldupdate-dll-gpio \
  libcpldupdate-dll-ast-jtag \
  fio \
  flashrom \
  front-paneld \
  fw-util \
  bic-monitor \
  openbmc-utils \
  sensor-setup \
  usb-console \
  lldp-util \
  log-util-v2 \
  mterm \
  sensor-util \
  sensor-mon \
  threshold-util \
  fscd \
  wedge-eeprom \
  weutil-dhcp-id \
  libpsu \
  psu-util \
  mdio-util \
  usb-fpga-util \
  fbmc-snapshot \
  "

def tpm_tools_recipe(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  if distro == 'rocko':
    return 'tpm2.0-tools'
  return 'tpm2-tools'

IMAGE_INSTALL += "${@tpm_tools_recipe(d)}"

# Copyright 2019-present Facebook. All Rights Reserved.

#
# u-boot binary is ~500KB, so let's adjust u-boot max size and fit image
# location as below:
#   - u-boot partition: 0 - 0xDF000 (892KB, 913,408 bytes)
#   - u-boot-env partition: 0xE0000 - 0xFFFFF (128KB)
#   - FIT image starts at: 0x100000 (1MB)
#
UBOOT_PART_MAX_BYTES = "913408"
UBOOT_CKSUM_OFFSET_KB = "892"
FLASH_FIT_OFFSET_KB = "1024"

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
  spatula \
  threshold-util \
  usb-console \
  wedge-eeprom \
  weutil-dhcp-id \
  rackmon \
  mtd-utils-ubifs \
  "

def tpm_tools_recipe(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  if distro == 'rocko':
    return 'tpm2.0-tools'
  return 'tpm2-tools'

IMAGE_INSTALL += "${@tpm_tools_recipe(d)}"

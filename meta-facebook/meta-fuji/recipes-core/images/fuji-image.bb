# Copyright 2020-present Facebook. All Rights Reserved.

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
  ipmid \
  ipmbd \
  ipmitool \
  bic-cache \
  bic-util \
  bitbang \
  cpldupdate \
  cpldupdate-i2c \
  libcpldupdate-dll-gpio \
  libcpldupdate-dll-ast-jtag \
  flashrom \
  front-paneld \
  fw-util \
  bic-monitor \
  openbmc-utils \
  usb-console \
  lldp-util \
  log-util-v2 \
  mterm \
  sensor-util \
  sensor-mon \
  threshold-util \
  spatula \
  fscd \
  wedge-eeprom \
  weutil-dhcp-id \
  libpsu \
  psu-util \
  mdio-util \
  usb-fpga-util \
  "

def tpm_tools_recipe(d):
  distro = d.getVar('DISTRO_CODENAME', True)
  if distro == 'rocko':
    return 'tpm2.0-tools'
  return 'tpm2-tools'

IMAGE_INSTALL += "${@tpm_tools_recipe(d)}"

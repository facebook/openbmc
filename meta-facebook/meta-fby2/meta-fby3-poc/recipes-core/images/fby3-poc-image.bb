# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"

# Include modules in rootfs
IMAGE_INSTALL += " \
  healthd \
  plat-utils \
  fan-util \
  fscd \
  watchdog-ctrl \
  sensor-setup \
  ipmid \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ipmbd \
  bic-cached \
  bic-util \
  ncsi-util \
  sensor-util \
  sensor-mon \
  power-util \
  mterm\
  cfg-util \
  fw-util \
  fpc-util \
  me-util \
  crashdump \
  log-util \
  lldp-util \
  spatula \
  openbmc-utils \
  ipmi-util \
  guid-util \
  bios-util \
  threshold-util \
  ncsid \
  vboot-utils \
  slot-util \
  cpldupdate-poc \
  i2craw \
  libusb1 \
  vrupdate-poc \
  fruid-util-remote \
  bic-remote-update-poc \
  m2cpldupdate-poc \
  fby2-sensors \
  setup-gpio \
  "

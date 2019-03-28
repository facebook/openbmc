# Copyright 2018-present Facebook. All Rights Reserved.

inherit kernel_fitimage

require recipes-core/images/fb-openbmc-image.bb

# The offset must match with the offsets defined in
# dev-spi-cmm.c. Rootfs starts from 4.5M
FLASH_ROOTFS_OFFSET = "4608"
# plat-utils
# sensor-setup
# bic-cached
# bic-util
# fby2-sensors
# gpiod
# gpiointrd
# front-paneld
# hsvc-util
# fpc-util
# imc-util
# me-util
# crashdump
# guid-util
# sensordump
# enclosure-util
# nvme-util
# ipmbd

# Include modules in rootfs
IMAGE_INSTALL += " \
  healthd \
  fan-util \
  fscd \
  ipmid \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  fruid \
  ncsi-util \
  sensor-util \
  sensor-mon \
  power-util \
  mterm\
  cfg-util \
  fw-util \
  log-util \
  lldp-util \
  spatula \
  openbmc-utils \
  ipmi-util \
  asd \
  asd-test \
  bios-util \
  threshold-util \
  libncsi \
  ncsid \
  vboot-utils \
  libpldm \
  "

IMAGE_FEATURES += " \
  ssh-server-openssh \
  tools-debug \
  "

DISTRO_FEATURES += " \
  ext2 \
  ipv6 \
  nfs \
  usbgadget \
  usbhost \
  "

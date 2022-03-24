# Copyright 2018-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require fby3-image-layout.inc

IMAGE_FSTYPES:remove = "cpio.lzma.u-boot"
IMAGE_FSTYPES += "cpio.zst.u-boot"

require recipes-core/images/fb-openbmc-image.bb

# sensor-setup
# hsvc-util
# fpc-util
# imc-util
# sensordump
# enclosure-util
# nvme-util

# Include modules in rootfs
IMAGE_INSTALL += " \
  attest-util \
  healthd \
  fan-util \
  fscd \
  ipmbd \
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
  log-util-v2 \
  lldp-util \
  openbmc-utils \
  ipmi-util \
  asd \
  asd-test \
  bios-util \
  threshold-util \
  libncsi \
  vboot-utils \
  plat-utils \
  i2craw \
  bic-util \
  me-util \
  bic-cached \
  guid-util \
  gpiod \
  front-paneld \
  gpiointrd \
  crashdump \
  show-sys-config \
  slot-util \
  enclosure-util \
  fpc-util \
  at \
  ncsid-v2 \
  throttle-util \
  name-util \
  nvme-util \
  usb2jtag-util \
  lrzsz \
  mctp-util \
  usbutils \
  syseventgen-util \
  usbip \
  exar1420 \
  "

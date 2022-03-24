# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require grandcanyon-image-layout.inc

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  packagegroup-openbmc-rest3 \
  setup-gpio \
  ipmbd \
  ipmid \
  ipmitool \
  expander-util \
  mterm \
  plat-utils \
  fw-util \
  fruid \
  name-util \
  log-util-v2 \
  openbmc-utils \
  fan-util \
  fscd \
  guid-util \
  sensor-util \
  sensor-mon \
  fpc-util \
  front-paneld \
  i2craw \
  healthd \
  cfg-util \
  threshold-util \
  libncsi \
  ncsid-v2 \
  ncsi-util \
  ipmb-util \
  exp-cached \
  bic-cached \
  power-util \
  usbutils \
  usb-console \
  gpiointrd \
  crashdump \
  me-util \
  throttle-util \
  bios-util \
  ipmi-util \
  gpiod \
  snapshot-util \
  enclosure-util \
  vboot-utils \
  obmc-libpldm \
  asd \
  asd-test \
  bic-util \
  iocd \
  otp \
  mctp-util \
  modprobe \
  "

# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require halfdome-image-layout.inc
require recipes-core/images/fb-zstd-rootfs.inc

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
  mterm \
  plat-utils \
  fw-util \
  fruid \
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
  bic-cached \
  power-util \
  usbutils \
  gpiointrd \
  throttle-util \
  bios-util \
  nvme-util \
  show-sys-config \
  bic-util \
  usb2jtag-util \
  at \
  gpiod \
  lldp-util \
  ipmi-util \
  vboot-utils \
  slot-util \
  enclosure-util \
  name-util \
  lrzsz \
  mctp-util \
  jbi \
  dimm-util \
  attest-util \
  psb-util \
  apml-util \
  prot-util \
  crashdump-amd \
  "

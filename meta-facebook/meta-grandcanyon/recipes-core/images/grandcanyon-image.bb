# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require grandcanyon-image-layout.inc

require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
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
  "

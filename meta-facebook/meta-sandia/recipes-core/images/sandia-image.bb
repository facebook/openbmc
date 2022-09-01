# Copyright 2020-present Facebook. All Rights Reserved.

require recipes-core/images/fbobmc-image-meta.inc
require sandia-image-layout.inc
require recipes-core/images/fb-openbmc-image.bb
 
# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  lldp-util \
  mterm \
  openbmc-utils \
  usb-console \
  "

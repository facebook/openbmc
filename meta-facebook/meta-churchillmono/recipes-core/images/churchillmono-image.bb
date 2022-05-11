# Copyright (c) 2022 Cisco Systems Inc.
# Copyright (c) Meta Platforms, Inc. and affiliates.

require recipes-core/images/fbobmc-image-meta.inc
require churchillmono-image-layout.inc
require recipes-core/images/fb-openbmc-image.bb

# Include modules in rootfs
IMAGE_INSTALL += " \
  packagegroup-openbmc-base \
  packagegroup-openbmc-net \
  packagegroup-openbmc-python3 \
  dhcp-client \
  mterm \
  openbmc-utils \
  "

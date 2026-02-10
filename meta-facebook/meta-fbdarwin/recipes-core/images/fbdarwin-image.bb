# Copyright 2022-present Facebook. All Rights Reserved.

require recipes-core/images/fboss-lite-image.inc
require fbdarwin-image-layout.inc

IMAGE_INSTALL += " \
  fio \
  prefdl-eeprom \
  serfmon-cache \
  show-tech \
  bmc-eeprom-checker \
  mdio-us-mac \
  "

#
# IPMI is Not supported in fbdarwin.
#
IMAGE_INSTALL:remove = " \
  ipmi-lite \
  kcsd \
  "

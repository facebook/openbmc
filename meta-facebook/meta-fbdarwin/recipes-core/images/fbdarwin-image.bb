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

remove_systemd_osc_context() {
  rm -f ${IMAGE_ROOTFS}${sysconfdir}/profile.d/80-systemd-osc-context.sh
  rm -f ${IMAGE_ROOTFS}${libdir}/systemd/profile.d/80-systemd-osc-context.sh
  # This prevents systemd from recreating the symlink at runtime
  rm -f ${IMAGE_ROOTFS}${libdir}/tmpfiles.d/20-systemd-osc-context.conf
}

ROOTFS_POSTPROCESS_COMMAND += "remove_systemd_osc_context; "

#
# IPMI is Not supported in fbdarwin.
#
IMAGE_INSTALL:remove = " \
  ipmi-lite \
  kcsd \
  "

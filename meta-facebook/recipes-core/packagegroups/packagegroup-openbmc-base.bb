SUMMARY = "Facebook OpenBMC base packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  fw-util \
  gpiocli \
  i2c-tools \
  kernel-modules \
  lmsensors-sensors \
  logrotate \
  os-release \
  packagegroup-openbmc-tpm \
  passwd-util \
  rsyslog \
  sudo \
  tzdata \
  u-boot \
  u-boot-fw-utils \
  wdtcli \
  "

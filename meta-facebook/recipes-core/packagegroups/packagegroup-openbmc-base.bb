SUMMARY = "Facebook OpenBMC base packages"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

RDEPENDS:${PN} += " \
  fw-util \
  obmc-dump \
  gpiocli \
  i2c-tools \
  kernel-modules \
  lmsensors-sensors \
  logrotate \
  os-release \
  packagegroup-openbmc-tpm \
  packagegroup-openbmc-emmc \
  packagegroup-openbmc-pfr \
  passwd-util \
  rsyslog \
  sudo \
  tzdata \
  u-boot-fw-utils \
  wdtcli \
  udev-rules \
  "

SUMMARY = "Facebook OpenBMC eMMC Package Group"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  e2fsprogs \
  util-linux \
  mmc-utils \
  "

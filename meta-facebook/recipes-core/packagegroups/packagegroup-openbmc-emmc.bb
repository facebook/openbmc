SUMMARY = "Facebook OpenBMC eMMC Package Group"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

EMMC_REQUIRED_PACKAGES = "\
  e2fsprogs \
  util-linux \
  mmc-utils \
  blktrace \
  fio \
  mmc-raw \
"

RDEPENDS_${PN} += "\
  ${@bb.utils.contains('MACHINE_FEATURES', 'emmc', '${EMMC_REQUIRED_PACKAGES}', '', d)} \
  "

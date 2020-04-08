SUMMARY = "Facebook OpenBMC PFR Package Group"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

PFR_REQUIRED_PACKAGES = "\
  pfr-util \
  jbi \
"

RDEPENDS_${PN} += "\
  ${@bb.utils.contains('MACHINE_FEATURES', 'pfr', '${PFR_REQUIRED_PACKAGES}', '', d)} \
  "

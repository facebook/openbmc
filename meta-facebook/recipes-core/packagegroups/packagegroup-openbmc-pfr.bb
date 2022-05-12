SUMMARY = "Facebook OpenBMC PFR Package Group"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

PFR_REQUIRED_PACKAGES = "\
  pfr-util \
  jbi \
"

RDEPENDS:${PN} += "\
  ${@bb.utils.contains('MACHINE_FEATURES', 'pfr', '${PFR_REQUIRED_PACKAGES}', '', d)} \
  "

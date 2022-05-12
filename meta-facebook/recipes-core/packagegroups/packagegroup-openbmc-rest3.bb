SUMMARY = "Facebook OpenBMC RESTful packages"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

RDEPENDS:${PN} += " \
  packagegroup-openbmc-python3 \
  rest-api \
  "

SUMMARY = "Facebook OpenBMC RESTful packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  packagegroup-openbmc-python3 \
  rest-api \
  "

SUMMARY = "Facebook OpenBMC RESTful packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  bottle \
  cherryPy \
  packagegroup-openbmc-python \
  rest-api \
  "

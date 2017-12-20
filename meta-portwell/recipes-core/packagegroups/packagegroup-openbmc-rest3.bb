SUMMARY = "Facebook OpenBMC RESTful packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  aiohttp \
  async-timeout \
  bottle \
  chardet \
  multidict \
  packagegroup-openbmc-python3 \
  rest-api \
  yarl \
  "

SUMMARY = "Facebook OpenBMC Network packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS:${PN} += " \
  dhcp-client \
  iproute2 \
  ntp \
  ntp-utils \
  ntpdate \
  sntp \
  "

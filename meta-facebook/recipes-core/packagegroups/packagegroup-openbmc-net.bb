SUMMARY = "Facebook OpenBMC Network packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  dhcp-client \
  iproute2 \
  ntp \
  ntpdate \
  ntp-utils \
  sntp \
  "

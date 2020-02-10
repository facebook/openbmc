SUMMARY = "Facebook OpenBMC Network packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  bind-utils \
  curl \
  dhcp-client \
  iproute2 \
  libndp \
  ntp \
  ntpdate \
  ntpq \
  openssh-sftp-server \
  sntp \
  "

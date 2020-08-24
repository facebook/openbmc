SUMMARY = "Facebook OpenBMC Network packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  bind-utils \
  curl \
  iproute2 \
  libndp \
  ntpdate \
  openssh-sftp-server \
  "

RDEPENDS_${PN}_append = " \
    ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '', 'ntp ntpq sntp dhcp-client', d)} \
    "

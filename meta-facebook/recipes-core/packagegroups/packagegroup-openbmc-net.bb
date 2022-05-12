SUMMARY = "Facebook OpenBMC Network packages"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

RDEPENDS:${PN} += " \
  bind-utils \
  curl \
  iproute2 \
  libndp \
  ntpdate \
  openssh-sftp-server \
  "

def openbmc_net_dhcpclient(d):
    distro = d.getVar('DISTRO_CODENAME', True)
    if distro in [ 'rocko', 'dunfell' ]:
        return "dhcp-client"
    return "dhcpcd"

RDEPENDS:${PN}:append = " \
    ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', \
                         '', \
                         'ntp ntpq sntp ' + openbmc_net_dhcpclient(d), d)} \
    "

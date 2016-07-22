SUMMARY = "Facebook OpenBMC Python packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  python-core \
  python-io \
  python-json \
  python-shell \
  python-subprocess \
  python-argparse \
  python-ctypes \
  python-datetime \
  python-email \
  python-threading \
  python-mime \
  python-pickle \
  python-misc \
  python-netserver \
  python-syslog \
  "

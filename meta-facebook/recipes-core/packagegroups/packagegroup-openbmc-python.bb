SUMMARY = "Facebook OpenBMC Python packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  python-argparse \
  python-core \
  python-ctypes \
  python-datetime \
  python-email \
  python-io \
  python-json \
  python-mime \
  python-misc \
  python-netserver \
  python-pickle \
  python-shell \
  python-subprocess \
  python-syslog \
  python-threading \
  "

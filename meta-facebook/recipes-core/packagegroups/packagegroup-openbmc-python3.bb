SUMMARY = "Facebook OpenBMC Python packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  python3-core \
  python3-compression \
  python3-enum \
  python3-crypt \
  python3-asyncio \
  python3-multiprocessing \
  python3-io \
  python3-json \
  python3-shell \
  python3-subprocess \
  python3-argparse \
  python3-ctypes \
  python3-datetime \
  python3-email \
  python3-threading \
  python3-mime \
  python3-pickle \
  python3-misc \
  python3-netserver \
  python3-syslog \
  "

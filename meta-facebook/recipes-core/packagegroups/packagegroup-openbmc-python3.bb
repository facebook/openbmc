SUMMARY = "Facebook OpenBMC Python packages"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += " \
  python3-asyncio \
  python3-compression \
  python3-core \
  python3-crypt \
  python3-ctypes \
  python3-datetime \
  python3-email \
  python3-fcntl \
  python3-io \
  python3-json \
  python3-mmap \
  python3-mime \
  python3-misc \
  python3-multiprocessing \
  python3-netserver \
  python3-pickle \
  python3-shell \
  python3-syslog \
  python3-threading \
  python3-pkgutil \ 
  python3-unittest \
  python3-typing \
"

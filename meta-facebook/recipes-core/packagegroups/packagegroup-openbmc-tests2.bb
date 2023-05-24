SUMMARY = "Facebook OpenBMC Packages for Tests2 support"

LICENSE = "GPL-2.0-or-later"
PR = "r1"

inherit packagegroup

RDEPENDS:${PN} += " \
    python3 \
    python3-unittest \
    python3-json \
    python3-syslog \
    "

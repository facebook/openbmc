FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-Add-queued-boot-progress-support.patch \
"

DEPENDS += "phosphor-logging"
DEPENDS += "libusb1"

EXTRA_OEMESON:append = " -Dqueued-boot-progress=enabled"
EXTRA_OEMESON:append = " -Dtransport-interface=i2c"
EXTRA_OEMESON:append = " -Dpoll-interval=50"
EXTRA_OEMESON:append = " -Ddevice-bus=70,71"
EXTRA_OEMESON:append = " -Ddevice-address=56,57"

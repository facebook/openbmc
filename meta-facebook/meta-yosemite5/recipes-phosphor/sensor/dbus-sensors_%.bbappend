FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-nvme-use-configurable-error-retry-delay.patch \
"

EXTRA_OEMESON:append = " -Ddisable-in2-alarm-event=true"

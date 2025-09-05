FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

SRC_URI:append:openbmc-fb-lf = " \
    file://0001-transport-Improve-time-validation-in-pldm_transport_.patch  \
"

EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem=meta"

FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

SRC_URI:append:openbmc-fb-lf = " \
    file://0001-json-Catch-NoConfigFound-in-compatIntfAdded.patch \
"

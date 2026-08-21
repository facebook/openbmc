FILESEXTRAPATHS:prepend:openbmc-fb-lf := "${THISDIR}/files:"

EXTRA_OEMESON:append:openbmc-fb-lf = " -Doem=meta"

# Allow LIBPLDM_API_TESTING testing API.
# Drop this and patch 0001 when 93303 lands upstream.
EXTRA_OEMESON:remove:openbmc-fb-lf = " -Dabi=deprecated,stable"
EXTRA_OEMESON:append:openbmc-fb-lf = " -Dabi=deprecated,stable,testing"

SRC_URI:append:openbmc-fb-lf = " \
    file://0001-dsp-platform-Add-tagged-numeric-sensor-decode-API.patch \
"

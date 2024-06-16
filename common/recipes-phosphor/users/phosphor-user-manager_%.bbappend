FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-user-mgr-Add-max_system_users-as-a-build-time-config.patch \
    file://0002-user-mgr-Allow-non-IPMI-usernames-with-dot-.-charact.patch \
"

# Bump maximum amount of system users
EXTRA_OEMESON:append = " -Dmax_system_users=1000"

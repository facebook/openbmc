FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Bump maximum amount of system users
EXTRA_OEMESON:append = " -Dmax_system_users=1000"

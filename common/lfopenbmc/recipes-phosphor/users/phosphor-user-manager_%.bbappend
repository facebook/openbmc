FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Bump maximum amount of system users
EXTRA_OEMESON:append = " -Dmax_system_users=1000"

# Grow getgrnam_r() buffer on ERANGE so large groups (e.g. redfish) do not
# lose their members -> empty UserPrivilege -> 403. Submitted upstream:
# https://gerrit.openbmc.org/c/openbmc/phosphor-user-manager/+/93053
SRC_URI += "file://0001-user_mgr-grow-getgrnam_r-buffer-on-ERANGE-for-large-.patch"

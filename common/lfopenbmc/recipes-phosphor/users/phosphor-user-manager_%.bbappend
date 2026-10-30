FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Bump maximum amount of system users
EXTRA_OEMESON:append = " -Dmax_system_users=1000"

# Fail hardened builds if upstream renames or changes PACKAGECONFIG[root-user-mgmt],
# rather than silently leaving root user management enabled.
python () {
    if bb.utils.contains('DISTRO_FEATURES', 'fb-hardened', True, False, d):
        flag = d.getVarFlag('PACKAGECONFIG', 'root-user-mgmt') or ''
        enabled = (d.getVar('PACKAGECONFIG') or '').split()
        if '-Droot_user_mgmt=disabled' not in flag or 'root-user-mgmt' in enabled:
            bb.fatal('fb-hardened: root_user_mgmt is not disabled; did upstream change PACKAGECONFIG[root-user-mgmt]?')
}

# Disable root user management in hardened builds, so root (UID 0) is left out
# of the users User.Manager serves over D-Bus. Unlike phosphor-no-root-login,
# this does not block root logins.
PACKAGECONFIG:remove = "${@bb.utils.contains('DISTRO_FEATURES', 'fb-hardened', 'root-user-mgmt', '', d)}"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://init \
            file://sshd_config \
           "

PR .= ".3"

RDEPENDS_${PN} += "bash"

do_configure_append() {
    sed -ri "s/__OPENBMC_VERSION__/${OPENBMC_VERSION}/g" sshd_config

    if ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false', d)}; then
        # Read SSH keys from the persistent store. The init script on
        # the SysV variant does this by creating symlinks at startup
        # time. For systemd, it's easier to modify the configuration
        # file to the right location. This will also make sshdgenkeys
        # create the keys here on <new devices.  I hate myself for
        # calling this "configuration"
        sed -i 's:.*HostKey.*\(/etc\):HostKey /mnt/data\1:' sshd_config
        sed -i '/ssh_host_key/d' sshd_config
    fi
}

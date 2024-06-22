FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://init \
            file://sshd_config \
            file://ssh_idle_logout.sh \
           "

PR .= ".3"

RDEPENDS:${PN} += "bash"
SSH_IDLE_TIMEOUT ?= "1800"

do_configure:append() {
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

do_install:append() {
  files="${D}${sysconfdir}/ssh/sshd_config"
  [ -e "${D}${sysconfdir}/ssh/sshd_config_readonly" ] && files="$files ${D}${sysconfdir}/ssh/sshd_config_readonly"
  sed -ri "s/__OPENBMC_VERSION__/${OPENBMC_VERSION}/g" $files
}

do_install:append() {
    install -d ${D}/${sysconfdir}/profile.d
    if [ "${SSH_IDLE_TIMEOUT}" -ne "0" ]; then
        install -m 644 ../ssh_idle_logout.sh ${D}/${sysconfdir}/profile.d/ssh_idle_logout.sh
        sed -i 's/__SSH_TMOUT__/${SSH_IDLE_TIMEOUT}/g' ${D}/${sysconfdir}/profile.d/ssh_idle_logout.sh
    fi
}

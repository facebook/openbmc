FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
PACKAGECONFIG:remove = "gnutls"
DEPENDS:append = " curl libgcrypt"
PTEST_ENABLED = ""

# This patch to initrd doesn't apply to the latest rsyslog in Yocto Kirkstone.
# Eventually we'll need to have a distro switch in here, but for now, we can
# just turn it off for systemd platforms.
SRC_URI += " \
    ${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '', \
        'file://0001-initd-use-flock-on-start-stop-daemon.patch;patchdir=${WORKDIR}', \
        d)} \
"

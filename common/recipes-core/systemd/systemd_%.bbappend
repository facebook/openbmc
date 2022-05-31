FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG:openbmc-fb = " \
        coredump \
        hostnamed \
        kmod \
        networkd \
        randomseed \
        resolved \
        sysusers \
        timedated \
        timesyncd \
        xz \
        "
# openbmc/openbmc also has 'pam' but we don't use it.

EXTRA_OEMESON += "-Ddns-servers=''"

ALTERNATIVE:${PN} += "init"
ALTERNATIVE_TARGET[init] = "${rootlibexecdir}/systemd/systemd"
ALTERNATIVE_LINK_NAME[init] = "${base_sbindir}/init"
ALTERNATIVE_PRIORITY[init] ?= "300"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/policy.conf:"

SRC_URI += " \
        file://coredump.conf \
        file://journald-maxlevel.conf \
        file://lastlog.conf \
        file://lock.conf \
"

do_install:append() {
    install -m 644 -D ${WORKDIR}/journald-maxlevel.conf ${D}${systemd_unitdir}/journald.conf.d/maxlevel.conf

    # The journal will store its logs in tmpfs and yocto's default will kill the BMC
    sed -i -e 's/.*RuntimeMaxUse.*/RuntimeMaxUse=20M/' ${D}${sysconfdir}/systemd/journald.conf
    sed -i -e 's/.*ForwardToSyslog.*/ForwardToSyslog=yes/' ${D}${sysconfdir}/systemd/journald.conf

    # Create /var/log/{wtmp,lastlog} at boot
    install -m 644 -D ${WORKDIR}/lastlog.conf ${D}${sysconfdir}/tmpfiles.d/lastlog.conf

    # Create /run/lock at boot
    install -m 644 -D ${WORKDIR}/lock.conf ${D}${sysconfdir}/tmpfiles.d/lock.conf

    # systemd 234 (rocko) does not support RequiredForOnline=no.
    sed -i 's@ExecStart.*@\0 --ignore=eth0.4088@' ${D}${systemd_unitdir}/system/systemd-networkd-wait-online.service

    # On OpenBMC long lived files are kept in /tmp and /var/tmp
    sed -E -i -e 's@(.*/tmp 1777 root root).*@\1 -@' ${D}${libdir}/tmpfiles.d/tmp.conf

    # Override coredump configuration
    install -m 644 -D ${WORKDIR}/coredump.conf ${D}${sysconfdir}/systemd/coredump.conf
}

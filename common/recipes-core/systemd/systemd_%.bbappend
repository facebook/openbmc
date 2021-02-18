FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

FILES_${PN}-catalog-extralocales = \
            "${exec_prefix}/lib/systemd/catalog/*.*.catalog"
PACKAGES =+ "${PN}-catalog-extralocales"

PACKAGECONFIG = " \
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

ALTERNATIVE_${PN} += "init"
ALTERNATIVE_TARGET[init] = "${rootlibexecdir}/systemd/systemd"
ALTERNATIVE_LINK_NAME[init] = "${base_sbindir}/init"
ALTERNATIVE_PRIORITY[init] ?= "300"

FILESEXTRAPATHS_prepend := "${THISDIR}/policy.conf:"

SRC_URI += " \
        file://journald-maxlevel.conf \
"

do_install_append() {
    install -m 644 -D ${WORKDIR}/journald-maxlevel.conf ${D}${systemd_unitdir}/journald.conf.d/maxlevel.conf

    # systemd 234 (rocko) does not support RequiredForOnline=no.
    sed -i 's@ExecStart.*@\0 --ignore=eth0.4088@' ${D}${systemd_unitdir}/system/systemd-networkd-wait-online.service
}

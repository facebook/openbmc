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

# Remove cryptsetup, etc for encrypted disks (from TPM recipe).
PACKAGECONFIG:remove = "cryptsetup cryptsetup-plugins"

EXTRA_OEMESON += "-Ddns-servers=''"
# Disable 'storage target mode' for size savings.
EXTRA_OEMESON:append:openbmc-fb-master = " -Dstoragetm=false"

# Disable systemd shell profile drop-ins (including the OSC context script) 
# to prevent terminal garbage on serial consoles.
EXTRA_OEMESON += "-Dshellprofiledir=no"

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
    install -m 644 -D ${UNPACKDIR}/journald-maxlevel.conf ${D}${systemd_unitdir}/journald.conf.d/maxlevel.conf

    sed -i -e 's/.*ForwardToSyslog.*/ForwardToSyslog=yes/' ${D}${sysconfdir}/systemd/journald.conf

    # Create /var/log/{wtmp,lastlog} at boot
    install -m 644 -D ${UNPACKDIR}/lastlog.conf ${D}${sysconfdir}/tmpfiles.d/lastlog.conf

    # Create /run/lock at boot
    install -m 644 -D ${UNPACKDIR}/lock.conf ${D}${sysconfdir}/tmpfiles.d/lock.conf

    # systemd 234 (rocko) does not support RequiredForOnline=no.
    sed -i 's@ExecStart.*@\0 --ignore=eth0.4088 --any --timeout=480s@' ${D}${systemd_unitdir}/system/systemd-networkd-wait-online.service

    # On OpenBMC long lived files are kept in /tmp and /var/tmp
    sed -E -i -e 's@(.*/tmp 1777 root root).*@\1 -@' ${D}${libdir}/tmpfiles.d/tmp.conf

    # Override coredump configuration
    install -m 644 -D ${UNPACKDIR}/coredump.conf ${D}${sysconfdir}/systemd/coredump.conf
}

do_install:append:openbmc-fb() {

    # This code is already in meta-phosphor's bbappend.  Add it here because it
    # is also relevant for fb-openbmc.

    # A number of udev devices would unlikely be present on a BMC and have large
    # helper executables associated with them.  Delete both the helpers and the
    # rules.
    for f in cdrom_id dmi_memory_id fido_id iocost v4l_id; do
        rm ${D}${libdir}/udev/${f}
    done
    for f in 60-cdrom_id.rules 70-memory.rules 60-fido-id.rules 90-iocost.rules 60-persistent-v4l.rules; do
        rm ${D}${libdir}/udev/rules.d/${f}
    done

    # Delete a few executables we don't need in order to make more room in the image.
    for f in systemd-growfs systemd-sleep; do
        rm ${D}${libdir}/systemd/$f
    done

}

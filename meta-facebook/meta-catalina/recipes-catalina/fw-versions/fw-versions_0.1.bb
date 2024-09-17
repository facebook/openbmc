FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "Firmware version populators"
DESCRIPTION = "Scripts to retrieve firmware versions and update settingsd"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

S = "${WORKDIR}"

NIC_INSTANCES="0:1"

FW_TOOLS = "\
    bmc-tpm,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    nic0,network-wait-ipv6-ll@eth0.service,multi-user.target,multi-user.target,0 \
    nic1,network-wait-ipv6-ll@eth1.service,multi-user.target,multi-user.target,0 \
    pdb-vr-aux,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    pdb-cpld,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    pdb-vr-n1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    pdb-vr-n2,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    scm-cpld,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-bmc-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-cpld-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-cpu-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-cpu-1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-erot-bmc-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-erot-cpu-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-erot-cpu-1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-erot-fpga-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-erot-fpga-1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-fpga-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-fpga-1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-gpu-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-fw-gpu-1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-inforom-gpu-0,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
    hmc-hgx-inforom-gpu-1,catalina-sys-init.service,multi-user.target,multi-user.target,0 \
"

LOCAL_URI = " \
    file://fw-versions@.service \
    ${@ ' '.join([ f"file://" + x.split(',')[0] for x in d.getVar('FW_TOOLS', True).split() ])} \
"

RDEPENDS:${PN}:append = " \
    bash \
    fw-util \
    hmc-util \
"

FILES:${PN}:append = " \
    ${systemd_system_unitdir} \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}${libexecdir}/${BPN}

    for f in ${FW_TOOLS}; do
        SERVICE=$(echo "$f" | cut -d, -f1)
        AFTER=$(echo "$f" | cut -d, -f2 | sed 's/_/ /g')
        BEFORE=$(echo "$f" | cut -d, -f3 | sed 's/_/ /g')
        WANTEDBY=$(echo "$f" | cut -d, -f4 | sed 's/_/ /g')

        install -m 0755 ${S}/$SERVICE ${D}${libexecdir}/${BPN}/$SERVICE

        install -m 0644 ${S}/fw-versions@.service \
            ${D}${systemd_system_unitdir}/fw-versions-$SERVICE@.service

        sed -i ${D}${systemd_system_unitdir}/fw-versions-$SERVICE@.service \
            -e "s/{SERVICE}/$SERVICE/g" \
            -e "s/{AFTER}/$AFTER/g" \
            -e "s/{BEFORE}/$BEFORE/g" \
            -e "s/{WANTEDBY}/$WANTEDBY/g"
    done
}

def fw_version_services(d):
    services = []

    for f in d.getVar('FW_TOOLS', True).split():
        (service, afters, befores, wantedbys, instances) = f.split(',')

        for i in instances.split(":"):
            for s in wantedbys.split('_'):
                wantedby_str = s.replace("%i", str(i))

                services.append(
                    f"{wantedby_str}:fw-versions-{service}@{i}.service:fw-versions-{service}@.service"
                )

    return " ".join(services)

pkg_postinst:${PN}:append() {
    for s in ${@fw_version_services(d)}; do
        TARGET=$(echo "$s" | cut -d: -f1)
        INSTANCE=$(echo "$s" | cut -d: -f2)
        SERVICE=$(echo "$s" | cut -d: -f3)

        mkdir -p "$D${systemd_system_unitdir}/$TARGET.wants"
        ln -s "../$SERVICE" "$D${systemd_system_unitdir}/$TARGET.wants/$INSTANCE"
    done
}

pkg_prerm:${PN}:append() {
    for s in ${@fw_version_services(d)}; do
        TARGET=$(echo "$s" | cut -d: -f1)
        INSTANCE=$(echo "$s" | cut -d: -f2)

        rm "$D${systemd_system_unitdir}/$TARGET.wants/$INSTANCE"
    done
}

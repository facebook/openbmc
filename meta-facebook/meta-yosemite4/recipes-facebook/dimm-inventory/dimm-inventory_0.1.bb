FILESEXTRAPATHS:append := "${THISDIR}/files:"

SUMMARY = "DIMM Inventory Populator"
DESCRIPTION = "Read DIMM SPD data via dimm-util and populate D-Bus inventory"

PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

HOST_INSTANCES = "${@d.getVar('OBMC_HOST_INSTANCES', True).replace(' ', ':')}"

S = "${UNPACKDIR}"
LOCAL_URI = " \
    file://dimm-inventory \
    file://dimm-inventory@.service \
"

RDEPENDS:${PN}:append = " \
    bash \
    dimm-util \
    jq \
"

FILES:${PN}:append = " \
    ${systemd_system_unitdir} \
"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -d ${D}${libexecdir}/${BPN}

    install -m 0755 ${UNPACKDIR}/dimm-inventory ${D}${libexecdir}/${BPN}/dimm-inventory
    install -m 0644 ${UNPACKDIR}/dimm-inventory@.service ${D}${systemd_system_unitdir}/
}

def dimm_inventory_services(d):
    services = []
    for i in d.getVar('OBMC_HOST_INSTANCES', True).split():
        for target in ["obmc-host-start@{}.target".format(i),
                        "obmc-host-reboot@{}.target".format(i)]:
            services.append("{}:dimm-inventory@{}.service:dimm-inventory@.service".format(
                target, i))
    return " ".join(services)

pkg_postinst:${PN}:append() {
    for s in ${@dimm_inventory_services(d)}; do
        TARGET=$(echo "$s" | cut -d: -f1)
        INSTANCE=$(echo "$s" | cut -d: -f2)
        SERVICE=$(echo "$s" | cut -d: -f3)

        mkdir -p "$D${systemd_system_unitdir}/$TARGET.wants"
        ln -s "../$SERVICE" "$D${systemd_system_unitdir}/$TARGET.wants/$INSTANCE"
    done
}

pkg_prerm:${PN}:append() {
    for s in ${@dimm_inventory_services(d)}; do
        TARGET=$(echo "$s" | cut -d: -f1)
        INSTANCE=$(echo "$s" | cut -d: -f2)

        rm -f "$D${systemd_system_unitdir}/$TARGET.wants/$INSTANCE"
    done
}

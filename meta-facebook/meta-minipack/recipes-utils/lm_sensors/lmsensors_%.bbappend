
FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://minipack.conf \
            file://0001-disable-bus-substitution.patch \
            file://adm1278-PIM16O.conf \
            file://adm1278-PIM16Q.conf \
            file://adm1278-FCM-T.conf \
            file://adm1278-FCM-B.conf \
            file://adm1278-SCM.conf \
           "

do_install_append() {
    install -d ${D}${sysconfdir}/sensors.d
    install -m 644 ../minipack.conf ${D}${sysconfdir}/sensors.d/minipack.conf
    install -d ${D}${sysconfdir}/sensors.d/custom
    install -m 644 ../adm1278-PIM16O.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-PIM16O.conf
    install -m 644 ../adm1278-PIM16Q.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-PIM16Q.conf
    install -m 644 ../adm1278-FCM-T.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-FCM-T.conf
    install -m 644 ../adm1278-FCM-B.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-FCM-B.conf
    install -m 644 ../adm1278-SCM.conf ${D}${sysconfdir}/sensors.d/custom/adm1278-SCM.conf
}

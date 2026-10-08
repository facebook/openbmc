FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-configuration-schema-extend-MCTP-target-members.patch \
    file://0002-configuration-schema-mctp-target-add-powerstate-prop.patch \
    file://0003-configurations-santabarbara-add-MB-ADI-VR-sensors.patch \
    file://0004-configuration-schema-add-MPQ82D00-PMBus-device-suppo.patch \
    file://0005-perform_scan-Extract-restorePersistedConfigurations.patch \
    file://0006-perform_scan-Fix-rescan-retaining-removed-configs.patch \
    file://0007-fru-device-avoid-recreating-unchanged-FRU-interfaces.patch \
    file://0008-fru-device-fix-redundant-rescans-on-dbus-property-ch.patch \
    file://0009-fru-device-defer-D-Bus-object-registration-to-preven.patch \
"

do_install:append() {
    rm -f ${D}${datadir}/${PN}/configurations/mtjade.json
    rm -f ${D}${datadir}/${PN}/configurations/mtjefferson_*.json
    rm -f ${D}${datadir}/${PN}/configurations/mtmitchell_*.json
}

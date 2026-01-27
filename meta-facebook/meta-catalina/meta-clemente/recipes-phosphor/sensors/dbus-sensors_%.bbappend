FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append:clemente = " \
    file://3001-clemente-dbus-sensors-Ignore-zero-HGX_GPU-_ENERGY_J.patch \
    file://3002-Add-sensor-reading-events.patch \
    file://3003-Ignore-smbpbi-sensor-event-report.patch \
"
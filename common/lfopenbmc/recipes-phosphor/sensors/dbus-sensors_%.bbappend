FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-PSUSensor-add-ina233-support.patch \
    file://0002-PSUSensor-add-adm1281-support.patch \
    file://0003-DeviceMgmt-fix-device-not-found.patch \
    file://0004-PSUSensor-Fix-error-for-decimal-part-of-scalefactor.patch \
    file://0005-Add-structured-logging-for-Threshold-events.patch \
    file://0006-psusensor-fixed-not-activate-for-multiple-power-stat.patch \
    file://0007-psusensors-fixed-multiple-power-state-issue.patch \
    file://0008-psu-support-gpio-bridge.patch \
    file://0009-meta-facebook-ventura-add-SCM-sensor-offset.patch \
    file://0010-PWMSensor-synchronize-hardware-PWM-with-D-Bus-proper.patch \
    file://0011-leakdetector-check-event-before-event_read.patch \
    file://0012-Add-delay-before-leak-event-log-to-prevent-AC-OFF-gl.patch \
    file://0013-PSUSensor-add-property-to-sensorTable-and-labelMatch.patch \
    file://0014-PSUSensor-add-support-for-mp2925-and-mp2929.patch \
    file://0015-hwmontemp-Remove-newly-created-sensor-from-sensorsCh.patch \
    file://0016-psusensor-skip-sensor-reads-during-firmware-updates.patch \
    file://0017-Add-sensor-reading-events.patch \
"

SRC_URI:append:fb-compute-multihost = " \
    file://0200-Utils-support-powerState-for-multi-node-system.patch \
    file://0201-Avoid-recreating-hwmon-temp-when-blade-cycle.patch \
    file://0202-meta-facebook-yosemite4-Disable-in2_alarm-event.patch \
    file://0203-Add-retry-attempts-configuration-for-fan-sensors.patch \
"

SRC_URI:append:clemente = " \
    file://3002-clemente-dbus-sensors-Ignore-zero-HGX_GPU-_ENERGY_J-.patch \
    file://3003-Ignore-smbpbi-sensor-event-report.patch \
"

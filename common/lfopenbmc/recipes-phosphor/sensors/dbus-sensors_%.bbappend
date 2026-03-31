FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-DeviceMgmt-fix-device-not-found.patch \
    file://0002-PSUSensor-Fix-error-for-decimal-part-of-scalefactor.patch \
    file://0003-Add-structured-logging-for-Threshold-events.patch \
    file://0004-psusensor-fixed-not-activate-for-multiple-power-stat.patch \
    file://0005-psusensors-fixed-multiple-power-state-issue.patch \
    file://0006-psu-support-gpio-bridge.patch \
    file://0007-meta-facebook-ventura-add-SCM-sensor-offset.patch \
    file://0008-PWMSensor-synchronize-hardware-PWM-with-D-Bus-proper.patch \
    file://0009-leakdetector-check-event-before-event_read.patch \
    file://0010-Add-delay-before-leak-event-log-to-prevent-AC-OFF-gl.patch \
    file://0011-PSUSensor-add-property-to-sensorTable-and-labelMatch.patch \
    file://0012-hwmontemp-Remove-newly-created-sensor-from-sensorsCh.patch \
    file://0013-psusensor-skip-sensor-reads-during-firmware-updates.patch \
    file://0014-meta-facebook-yosemite4-Disable-in2_alarm-event.patch \
    file://0015-Implement-valve-monitor-service.patch \
    file://0016-psusensor-Add-support-for-per-sensor-PollRate-config.patch \
    file://0017-SmbpbiSensor-Fix-invalid-data-check-size-for-tempera.patch \
"

SRC_URI:append:fb-compute-multihost = " \
    file://0200-Utils-support-powerState-for-multi-node-system.patch \
    file://0201-Avoid-recreating-hwmon-temp-when-blade-cycle.patch \
    file://0202-Add-retry-attempts-configuration-for-fan-sensors.patch \
"

PACKAGECONFIG[valvemonitor] = "-Dvalve-monitor=enabled, -Dvalve-monitor=disabled"

SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'valvemonitor', \
                                               'xyz.openbmc_project.valvemonitor.service', \
                                               '', d)}"

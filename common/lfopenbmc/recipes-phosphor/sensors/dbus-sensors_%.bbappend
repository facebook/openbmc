FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-DeviceMgmt-fix-device-not-found.patch \
    file://0002-Add-structured-logging-for-Threshold-events.patch \
    file://0003-psusensor-fixed-not-activate-for-multiple-power-stat.patch \
    file://0004-psusensors-fixed-multiple-power-state-issue.patch \
    file://0005-psu-support-gpio-bridge.patch \
    file://0006-meta-facebook-ventura-add-SCM-sensor-offset.patch \
    file://0007-PWMSensor-synchronize-hardware-PWM-with-D-Bus-proper.patch \
    file://0008-leakdetector-check-event-before-event_read.patch \
    file://0009-Add-delay-before-leak-event-log-to-prevent-AC-OFF-gl.patch \
    file://0010-PSUSensor-add-property-to-sensorTable-and-labelMatch.patch \
    file://0011-hwmontemp-Remove-newly-created-sensor-from-sensorsCh.patch \
    file://0012-psusensor-skip-sensor-reads-during-firmware-updates.patch \
    file://0013-meta-facebook-yosemite4-Disable-in2_alarm-event.patch \
    file://0014-valve-monitor-request-for-directory.patch \
    file://0015-Implement-valve-monitor-service.patch \
    file://0016-psusensor-Add-support-for-per-sensor-PollRate-config.patch \
    file://0017-SmbpbiSensor-Fix-invalid-data-check-size-for-tempera.patch \
    file://0018-mctpreactor-Support-configuration-for-USB-MCTP-devic.patch \
    file://0019-dbus-sensors-implement-in-place-threshold-updates-to.patch \
    file://0020-mctp-add-PowerState-check-in-MCTPReactor.patch \
    file://0021-CableMonitor-increase-reconcile-delay-to-60s.patch \
"

SRC_URI:append:ventura2 = " \
    file://0101-valve-monitor-add-analog-valve-support.patch \
    file://0102-valve-monitor-delay-analog-valve-feedback-monitoring.patch \
"

SRC_URI:append:fb-compute-multihost = " \
    file://0200-Utils-support-powerState-for-multi-node-system.patch \
    file://0201-Avoid-recreating-hwmon-temp-when-blade-cycle.patch \
    file://0202-Add-retry-attempts-configuration-for-fan-sensors.patch \
"

SRC_URI:append:santabarbara = " \
    file://0301-Thresholds-Add-option-to-log-thresholds-on-second-hi.patch \
"

PACKAGECONFIG[valvemonitor] = "-Dvalve-monitor=enabled, -Dvalve-monitor=disabled"

SYSTEMD_SERVICE:${PN} += "${@bb.utils.contains('PACKAGECONFIG', 'valvemonitor', \
                                               'xyz.openbmc_project.valvemonitor.service', \
                                               '', d)}"

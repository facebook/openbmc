FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-PSUSensor-add-ina233-support.patch \
    file://0002-PSUSensor-add-adm1281-support.patch \
    file://0003-DeviceMgmt-fix-device-not-found.patch \
    file://0004-PSUSensor-Fix-error-for-decimal-part-of-scalefactor.patch \
    file://0005-Add-structured-logging-for-Threshold-events.patch \
    file://0006-psusensor-fixed-not-activate-for-multiple-power-stat.patch \
    file://0007-psusensors-fixed-multiple-power-state-issue.patch \
"

SRC_URI:append:fb-compute-multihost = " \
    file://0200-Utils-support-powerState-for-multi-node-system.patch \
    file://0201-Avoid-recreating-hwmon-temp-when-blade-cycle.patch \
    file://0202-meta-facebook-yosemite4-Disable-in2_alarm-event.patch \
    file://0203-Add-retry-attempts-configuration-for-fan-sensors.patch \
    file://0204-pwm-sensor-Align-Target-with-user-intent.patch \
"

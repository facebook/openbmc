FILESEXTRAPATHS:prepend := "${THISDIR}/dbus-sensors:"
SRC_URI += " \
            file://0001-Fansensor-Support-ast26xx-pwm-tach.patch \
            file://0002-Fansensor-remove-SOC-info.patch \
           "

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = "\
    file://support_pwm_snr.patch \
    file://support-auxiliary-bus-type.patch \
    file://0002-fix-fan-enable-attribute.patch \
    file://0003-modfiy-Sensors-precision-values.patch \
"

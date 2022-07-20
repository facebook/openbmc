FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = "\
    file://support_pwm_snr.patch \
    file://support-auxiliary-bus-type.patch \
"

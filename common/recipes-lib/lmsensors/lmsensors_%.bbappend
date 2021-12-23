FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = "\
    file://support_pwm_snr.patch \
"

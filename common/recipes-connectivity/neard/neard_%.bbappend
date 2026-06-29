FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-ndef-handle-MIMEPayload-field-in-a-sv-message.patch \
    file://0002-nfctype5-fix-Type-5-vendor-specific-OPTION-flag-hand.patch \
"


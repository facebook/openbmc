SUMMARY = "Facebook OpenBMC TPM Package Group"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS:${PN}:append:mf-tpm1 = " tpm-tools trousers"
RDEPENDS:${PN}:append:mf-tpm2 = " tpm2-tools libtss2-tcti-device"

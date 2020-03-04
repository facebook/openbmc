SUMMARY = "Facebook OpenBMC TPM Package Group"

LICENSE = "GPLv2"
PR = "r1"

inherit packagegroup

RDEPENDS_${PN} += "\
  ${@bb.utils.contains('MACHINE_FEATURES', 'tpm1', 'tpm-tools', '', d)} \
  ${@bb.utils.contains('MACHINE_FEATURES', 'tpm1', 'trousers', '', d)} \
  \
  ${@bb.utils.contains('MACHINE_FEATURES', 'tpm2', 'tpm2-tools', '', d)} \
  "

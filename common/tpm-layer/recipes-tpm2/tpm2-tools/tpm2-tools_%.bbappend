# Copyright 2022-present Facebook. All Rights Reserved.

RDEPENDS:${PN}:remove = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '', 'tpm2-abrmd', d)}" 

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# meta-phosphor already supplies this backport when that layer is present.
SRC_URI:append = "${@' file://0001-use-asn1-getters.patch' if d.getVar('PV') == '5.7' and 'phosphor-layer' not in d.getVar('BBFILE_COLLECTIONS').split() else ''}"

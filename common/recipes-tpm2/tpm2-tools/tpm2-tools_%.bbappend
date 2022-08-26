# Copyright 2022-present Facebook. All Rights Reserved.

RDEPENDS:${PN}:remove = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', '', 'tpm2-abrmd', d)}" 

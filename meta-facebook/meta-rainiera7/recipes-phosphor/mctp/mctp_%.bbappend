FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# This is fixed with upstream commit 0b8772db but we need to manually
# port it in to handle the yocto update.
FILES:${PN}:append = " \
    ${systemd_system_unitdir}/rainier-mctp-i3c@.service \
    "

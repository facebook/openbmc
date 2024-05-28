FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

require u-boot-version-override.inc

SRC_URI:append:openbmc-fb-lf = "\
        file://0501-mem_test-Support-to-save-mtest-result-in-environm.patch \
        file://0502-configs-openbmc-Revise-size-of-environment-and-us.patch \
        "

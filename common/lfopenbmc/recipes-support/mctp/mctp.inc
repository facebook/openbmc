FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# Shared by the mctp (tools) and mctpd (daemon) recipes, which both build
# from the same upstream source. All patches apply cleanly to both; the
# mctpd.c changes are simply uncompiled into the tools build, while the
# lib/tomlc99 fix in 0504 is required by both.
SRC_URI += " \
    file://0500-Support-get-static-eid-config-from-entity-manager.patch \
    file://0501-Add-method-for-setting-up-MCTP-EID-by-config-path.patch \
    file://0502-mctpd-fix-mctpd-crash-issue.patch \
    file://0503-mctpd-Implement-retry-mechanism-for-static-EID-setup.patch \
    file://0504-Fix-const-compiler-error-and-format-error.patch \
"

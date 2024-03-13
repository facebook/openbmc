FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0500-Support-get-static-eid-config-from-entity-manager.patch \
    file://0501-Modified-the-type-of-NetworkId-to-uint32_t.patch \
    file://0502-mctpd-Add-AssignEndpointStatic-dbus-interface.patch \
"

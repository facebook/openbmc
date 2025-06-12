FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0100-Set-request-timeout-to-40s-on-bmcweb-http-http_conne.patch \
    file://0101-Up-the-max-connections-from-200-to-1000.patch \
"

EXTRA_OEMESON:append = " -Dredfish-updateservice-use-dbus=enabled"

# Enable PDI-generated Redfish Message Registries
EXTRA_OEMESON:append = " -Dredfish-allow-dbus-messages-mapping=enabled"
# Patches for PDI-generated Redfish Message Registries
SRC_URI:append = " \
    file://0200-registries-generate-registries_selector.hpp.patch \
    file://0201-Add-generation-for-Vendor-registries-and-a-DBus-to-R.patch \
    file://0202-registries-generate-from-phosphor-dbus-interfaces.patch \
    file://0203-Add-required-MemberId-to-the-EventRecord-definition.patch \
    file://0204-Map-DBus-event-to-RF-MessageID-and-parse-args.patch \
    file://0205-Show-mapped-and-raw-Dbus-messages-in-Log-services.patch \
    file://0206-Fix-JSON-fields-not-being-extracted-in-SensorsAsyncR.patch \
"

# Dependency variables for PDI-generated Redfish Message Registries
DEPENDS += " \
    ${PYTHON_PN}-requests-native \
    phosphor-dbus-interfaces \
"
inherit python3native

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0100-Set-request-timeout-to-40s-on-bmcweb-http-http_conne.patch \
    file://0101-Up-the-max-connections-from-200-to-1000.patch \
    file://0102-parsing-add-parseTrustedStringAsJson-for-internal-pa.patch \
    file://0103-parsing-increase-maxValues-to-5000-for-telemetry.patch \
"

EXTRA_OEMESON:append = " -Dredfish-updateservice-use-dbus=enabled"

# S546922: Disable HTTP2 as it's not working with cert auth
EXTRA_OEMESON:append = " -Dhttp2=disabled"

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
"

# Dependency variables for PDI-generated Redfish Message Registries
DEPENDS += " \
    ${PYTHON_PN}-requests-native \
    phosphor-dbus-interfaces \
"
inherit python3native

SRC_URI:append:fb-compute-multihost = " \
    file://0300-Multi-host-HW-inventory-add-processor-subtree.patch \
    file://0301-Multi-host-HW-inventory-add-memory-subtree.patch \
"

SRC_URI:append = " \
    file://0350-Store-Log-Entry-URI-for-Redfish-event.patch \
"

# Drive state management - Redfish Drive.Reset action
SRC_URI:append = " \
    file://0400-storage-Add-Drive.Reset-action-with-association-base.patch \
"

# CPER support
SRC_URI:append = " \
    file://0410-LogEntry-Add-support-for-parsing-and-presenting-CPER.patch \
    file://0411-Event-carry-the-CPER-section-onto-the-pushed-EventRe.patch \
"

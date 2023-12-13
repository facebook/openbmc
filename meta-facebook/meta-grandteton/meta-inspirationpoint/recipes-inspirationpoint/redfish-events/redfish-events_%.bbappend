FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LOCAL_URI += " \
    file://redfish-events-hgx.cfg \
    file://redfish-events-ubb.cfg \
    file://run-redfish-events.sh \
  "
CONFIGS = "redfish-events-hgx.cfg redfish-events-ubb.cfg"
DEFAULT_CONFIG = "redfish-events-hgx.cfg"
RDEPENDS:${PN} += "bash"

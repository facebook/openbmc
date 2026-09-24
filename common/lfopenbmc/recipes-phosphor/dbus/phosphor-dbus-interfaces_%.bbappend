FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://0001-State-add-generic-CPER-notification-events.patch \
    file://0002-CPER-add-generic-event-for-CPER-errors.patch \
    file://0003-Control-Port-Add-interface-for-monitoring-control.patch \
    file://0004-State.Drive-Add-On-Off-transitions-and-namespace-pat.patch \
    file://0005-State.Drive-Add-NotAllowed-error-to-RequestedDriveTr.patch \
"

do_write_config[depends] = ""

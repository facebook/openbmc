FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PV = "1.3.3"
SRCREV = "ecc0f1ba4fca96f3d7ef060b6c53f2e0bcfe829f"

SRC_URI += " \
    file://0001-Nima-rough-draft-http-proxy-support-for-raw-socket.patch \
    "

inherit meson pkgconfig

SUMMARY = "Calculator Example"
DESCRIPTION = "An example of writing an sdbusplus daemon"
SECTION = "base"
PR = "r1"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = " \
    file://main.cpp;beginline=1;endline=1;md5=4f8ec6e05e980b2ef6a75510c6499cca \
"

LOCAL_URI = " \
    file://meson.build \
    file://main.cpp \
    file://regenerate-bindings.sh \
    file://yaml \
    file://gen \
"

DEPENDS += " \
    ${PYTHON_PN}-sdbus++-native \
    sdbusplus \
"

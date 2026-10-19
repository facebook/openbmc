FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " \
    file://0001-bindings-cxx-add-optional-c-bindings-using-nlohmann.patch \
    file://0002-Add-support-for-MTIA-CPERs.patch \
    "

DEPENDS:append = " nlohmann-json"

EXTRA_OEMESON += " \
    -Dbindings-cxx=enabled \
"

LINUX_VERSION_EXTENSION = "-fbal"

COMPATIBLE_MACHINE = "angelslanding"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://fbal.cfg"

# remove the following when we landing the change into kernel
SRC_URI += "file://NEW_META_FLASH_LAYOUT.patch"

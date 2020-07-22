LINUX_VERSION_EXTENSION = "-fbep"

COMPATIBLE_MACHINE = "emeraldpools"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://fbep.cfg"

# remove the following when we landing the change into kernel
SRC_URI += "file://NEW_META_FLASH_LAYOUT.patch"

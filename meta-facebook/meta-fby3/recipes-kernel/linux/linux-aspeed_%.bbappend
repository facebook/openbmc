LINUX_VERSION_EXTENSION = "-fby3"

COMPATIBLE_MACHINE = "fby3"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
SRC_URI += "file://fby3.cfg"

# remove the following when we landing the change into kernel
# after parallel fby3-pvt machine conf replace offical fby3 machine conf
FBY3_PVT_NEW_LAYOUT_PATCH ??= ""
SRC_URI += "${FBY3_PVT_NEW_LAYOUT_PATCH}"


FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

# Only 4.1 will use this defconfig. later kernel's defconfig is stored in
# meta-aspeed layer.
SRC_URI += "file://defconfig \
           "

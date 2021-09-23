FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# opkg is the primary user of libarchive and we don't use these two compression
# types, so remove them to reduce the space usage.
PACKAGECONFIG:remove = "bz2 lzo"

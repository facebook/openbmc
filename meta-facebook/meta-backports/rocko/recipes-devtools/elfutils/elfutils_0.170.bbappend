FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://0001-Ensure-that-packed-structs-follow-the-gcc-memory-lay.patch \
            file://0009-libelf-Mark-both-fsize-and-msize-with-const-attribut.patch \
           "
EXTRA_OEMAKE_class-native += "CFLAGS=-w"

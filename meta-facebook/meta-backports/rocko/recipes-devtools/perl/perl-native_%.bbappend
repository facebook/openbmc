DEPENDS += "libnsl2-native"
LDFLAGS += "-L${STAGING_LIBDIR_NATIVE}/nsl"

do_compile_prepend() {
    sed -i 's/^CCFLAGS = /CCFLAGS = -fPIC /' Makefile
}

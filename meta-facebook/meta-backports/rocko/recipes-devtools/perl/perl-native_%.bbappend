DEPENDS += "libnsl2-native"
LDFLAGS += "-L${STAGING_LIBDIR_NATIVE}/nsl"
CFLAGS_append_class-native = " -fPIC -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

do_configure_append_class-native() {
    sed -i "s/^ccflags='\(.*\)'/ccflags='-fPIC \1'/" config.sh
    sed -i 's/^myccflags="/myccflags="-fPIC /' cflags.SH
}

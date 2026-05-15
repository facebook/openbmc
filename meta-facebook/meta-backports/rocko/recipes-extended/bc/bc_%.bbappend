
BUILD_CFLAGS_append = " -Wno-error=implicit-function-declaration -Wno-error=builtin-declaration-mismatch"

do_configure_append() {
    sed -i '1i #include <string.h>' ${S}/bc/load.c
    sed -i '1i #include <string.h>' ${S}/bc/scan.c
    sed -i '1i #include <string.h>' ${S}/bc/util.c
}

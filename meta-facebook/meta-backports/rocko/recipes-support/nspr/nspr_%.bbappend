
CFLAGS_append = " -Wno-error=incompatible-pointer-types"

do_configure_append() {
    sed -i '1i #include <unistd.h>' ${S}/pr/tests/sel_spd.c
    sed -i '1i #include <getopt.h>' ${S}/pr/tests/testfile.c
}

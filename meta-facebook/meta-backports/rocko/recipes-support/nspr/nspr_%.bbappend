
do_configure_append() {
    sed -i '1i #include <unistd.h>' ${S}/pr/tests/sel_spd.c
}

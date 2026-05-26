EXTRA_OECONF_append = " --disable-raw"
EXTRA_OECONF_append_class-native = " --without-ncursesw --without-ncurses"
CFLAGS_append_class-native = " -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

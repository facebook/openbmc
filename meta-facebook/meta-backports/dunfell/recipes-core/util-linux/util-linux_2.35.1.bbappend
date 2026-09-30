EXTRA_OECONF:append = " --disable-raw"

# util-linux-native fails under the gcc 14 CI toolchain: misc-utils/kill.c calls
# pidfd_open()/pidfd_send_signal() without declarations, and gcc 14 makes
# -Wimplicit-function-declaration an error by default. Keep it (and the related
# gcc-14 default-error diagnostics) non-fatal for the native build; the target
# build uses the older cross gcc. (rocko's util-linux append already does this.)
CFLAGS:append:class-native = " -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

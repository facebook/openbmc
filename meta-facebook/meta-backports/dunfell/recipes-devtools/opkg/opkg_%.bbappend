# opkg-native fails under the gcc 14 CI toolchain: the libsolv solver
# (libopkg/solvers/libsolv/opkg_solver_libsolv.c) passes an incompatible
# function pointer to pool_setdebugcallback(), and gcc 14 makes
# -Wincompatible-pointer-types an error by default. libsolv is the default
# PACKAGECONFIG solver, so that file is built. Keep the gcc-14 default-error
# diagnostics non-fatal for the native build; the target build uses the older
# cross gcc.
#
# opkg-native is a BBCLASSEXTEND of opkg_0.4.2.bb, so this is opkg_%.bbappend
# scoped with _class-native (a opkg-native_%.bbappend would match nothing).
CFLAGS_append_class-native = " -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion"

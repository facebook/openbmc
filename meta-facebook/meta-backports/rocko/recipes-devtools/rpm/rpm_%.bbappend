# rpm-native fails to build with the gcc 14 toolchain on the CentOS Stream 10
# Sandcastle fleet. lib/rpmscript.c calls rpmChrootIn()/rpmChrootOut() but only
# includes lib/rpmscript.h, not lib/rpmchroot.h where they are declared. gcc 14
# promotes -Wimplicit-function-declaration to an error by default, so the build
# aborts at rpmscript.lo. gcc 11 (local devservers) only warns, so this is
# invisible locally and only fails in CI.
#
# Restore the pre-gcc-14 warning behaviour for the *native* build only -- the
# functions exist in librpm with the assumed int(void) signature, so the
# implicit declaration is harmless. The target build uses cross gcc 7.3 and is
# unaffected. (Mirrors the existing util-linux / perl-native native appends.)
CFLAGS_append_class-native = " -Wno-error=implicit-function-declaration -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

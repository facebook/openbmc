# rpm-native (4.14.2.1) fails under the gcc 14 CI toolchain: lib/rpmscript.c
# calls rpmChrootIn()/rpmChrootOut() without including lib/rpmchroot.h, and gcc
# 14 makes -Wimplicit-function-declaration an error by default. gcc 11 only
# warns, so it builds on older devservers. Keep it non-fatal for the native
# build only; the target build uses the older cross gcc.
#
# Mirrors the rocko rpm backport (rocko rpm is 4.13.90, same rpmscript.c issue).
# rpm-native is a BBCLASSEXTEND of rpm_4.14.2.1.bb, so this is rpm_%.bbappend
# scoped with _class-native.
CFLAGS_append_class-native = " -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

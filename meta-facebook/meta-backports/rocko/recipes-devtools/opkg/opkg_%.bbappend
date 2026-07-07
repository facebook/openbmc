# Proactive (preventive): rocko opkg (0.3.5) also builds opkg-native with the
# libsolv solver and now rebuilds it from source under the gcc 14 CI toolchain
# (native sstate is host-keyed). gcc 14 promotes -Wincompatible-pointer-types
# and friends to errors by default; the dunfell opkg 0.4.2 libsolv solver hits
# this (pool_setdebugcallback incompatible pointer). Keep those non-fatal for
# the native build here too. Harmless if 0.3.5's solver code doesn't trip them.
#
# opkg-native is a BBCLASSEXTEND of opkg_0.3.5.bb, so this is opkg_%.bbappend
# scoped with _class-native.
CFLAGS_append_class-native = " -Wno-error=incompatible-pointer-types -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion"

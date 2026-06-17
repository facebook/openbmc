# rsync-native fails to build under the gcc 14 toolchain (CentOS Stream 10 CI):
# gcc 14 treats several previously-warning diagnostics as errors. Keep them as
# warnings for the native build only -- the target build uses the older cross gcc
# and is unaffected. (Mirrors the existing rpm / util-linux native appends.)
CFLAGS_append_class-native = " -Wno-error=implicit-fallthrough -Wno-error=deprecated-declarations -Wno-error=return-type -Wno-error=array-parameter -Wno-error=sign-compare -Wno-error=implicit-function-declaration"

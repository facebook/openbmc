EXTRA_OECONF_append_class-native = " --without-brotli --disable-threaded-resolver"
LDFLAGS_append_class-native = " -Wl,-rpath,${STAGING_LIBDIR_NATIVE}"

do_configure_prepend_class-native() {
    export LD_LIBRARY_PATH="${STAGING_LIBDIR_NATIVE}:${LD_LIBRARY_PATH}"
    sed -i '/one or more libs available at link-time are not available run-time/d' ${S}/acinclude.m4
}

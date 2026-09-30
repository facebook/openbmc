# perl-native (perl-cross) misdetects directory functions under the gcc 14 CI
# toolchain. gcc 14 promotes -Wimplicit-function-declaration (and friends) to
# errors by default, so perl-cross's configure probes -- e.g.
# "checkfunc d_readdir 'readdir'" -- fail to *compile* and get recorded as
# absent (d_readdir=undef, i_dirent=undef). miniperl is then built without
# opendir/readdir and dies at build time running Makefile.PL with:
#   Unsupported directory function "opendir" called at .../MM_Unix.pm
#
# Keep those diagnostics non-fatal for the native build so the probes compile
# and detect correctly. perl-cross feeds CFLAGS into ccflags and uses ccflags
# for probe compiles (cnf/configure_tool.sh: "setfromenv ccflags CFLAGS";
# cnf/configure__f.sh: "run $cc $ccflags ... -c try.c"). Native only -- the
# target build uses the older cross gcc. Mirrors the rocko perl-native append.
#
# dunfell perl-native is a BBCLASSEXTEND of perl_5.30.1.bb (not a separate
# recipe as on rocko), so this is perl_%.bbappend scoped with _class-native.
CFLAGS_append_class-native = " -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion -Wno-error=incompatible-pointer-types"

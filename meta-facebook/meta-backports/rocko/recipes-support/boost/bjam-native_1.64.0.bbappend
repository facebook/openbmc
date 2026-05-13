# Boost 1.64.0's jam engine C sources use implicit function declarations
# and other pre-C99 idioms that modern GCC (13+) rejects as errors.
# The "cc" toolset (unlike "gcc") honors CC and CFLAGS in build.sh.
do_compile() {
    CC=gcc \
    CFLAGS="-std=gnu89 -Wno-error=implicit-function-declaration -Wno-error=implicit-int -Wno-error=int-conversion -Wno-error=incompatible-pointer-types" \
        ./bootstrap.sh --with-toolset=cc
}

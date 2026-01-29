SRC_URI:remove = "file://older-glibc-symbols.patch"

SRC_URI = "git://git.yoctoproject.org/pseudo;branch=master;protocol=https \
           file://fallback-passwd \
           file://fallback-group \
"

SRCREV = "750362cc7b9fa58dffccd95d919b435c6d8ac614"
S = "${WORKDIR}/git"
PV = "1.9.3+git"


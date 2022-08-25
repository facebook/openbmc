PACKAGECONFIG[libxml2] = ""
PACKAGECONFIG[xfs] = ""
PACKAGECONFIG[vnc-png] = "--enable-vnc --enable-png,--disable-vnc --disable-png,libpng"
PACKAGECONFIG[ust] = "--enable-trace-backend=ust,,lttng-ust"

SRC_URI:remove:class-target = "file://cross.patch"
SRC_URI:remove:class-nativesdk = "file://cross.patch"

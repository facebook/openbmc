DESCRIPTION = "Util for checking with Bridge IC on Minipack"

CFLAGS += "-DPLATFORM_MINIPACK -lminipack_gpio"
DEPENDS += "libminipack-gpio"
RDEPENDS_${PN} += "libminipack-gpio"

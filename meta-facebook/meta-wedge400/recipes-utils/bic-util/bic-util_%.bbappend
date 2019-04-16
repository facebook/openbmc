DESCRIPTION = "Util for checking with Bridge IC on Wedge400"

CFLAGS += "-DPLATFORM_WEDGE400 -lwedge400_gpio"
DEPENDS += "libwedge400-gpio"
RDEPENDS_${PN} += "libwedge400-gpio"

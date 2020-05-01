FILESEXTRAPATHS_prepend := "${THISDIR}/files/pal:"

DEPENDS += "libgpio-ctrl libnm libpeci libobmc-sensors libobmc-pmbus"
RDEPENDS_${PN} += "libgpio-ctrl libnm libpeci libobmc-sensors libobmc-pmbus"
LDFLAGS += "-lgpio-ctrl -lnm -lpeci -lobmc-sensors -lobmc-pmbus"

SUMMARY = "Easy-to-use Modbus RTU and Modbus ASCII implementation for Python"
HOMEPAGE = "https://github.com/pyhys/minimalmodbus"
AUTHOR = "Jonas Berg"
LICENSE = "Apache-2.0"

PYPI_PACKAGE = "minimalmodbus"
LIC_FILES_CHKSUM = "file://LICENSE;md5=27da4ba4e954f7f4ba8d1e08a2c756c4"
SRC_URI[md5sum] = "3fe320f7be761b6a2c3373257c431c31"
SRC_URI[sha256sum] = "cf873a2530be3f4b86467c3e4d47b5f69fd345d47451baca4adbf59e2ac36d00"

RDEPENDS:${PN} += "python3-core python3-pyserial"

inherit pypi setuptools3
# TODO Use python_flit_core instead of setuptools3 for newer Yocto releases

# Handle that there is no setup.py in the project
# TODO Remove this once we use python_flit_core
do_configure:prepend() {
cat > ${S}/setup.py <<-EOF
from setuptools import setup

setup(
    name="${PYPI_PACKAGE}",
    version="${PV}",
    license="${LICENSE}",
)
EOF
}

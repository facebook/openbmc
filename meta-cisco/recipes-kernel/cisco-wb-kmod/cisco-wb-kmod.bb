SUMMARY = "cisco-wb-kmod kernel modules"
LICENSE = "CISCO_2021v1"
LIC_FILES_CHKSUM = "file://${COMPANY_COMMON_LICENSES}/CISCO_2021v1;md5=ff6bb3e79f80c8e5a25a9620cdc180b2"

inherit module

PR = "r0"
PV = "1.6"

SRC_URI = " \
    file://Makefile \
    git://git@github.com:/Cisco-open-NOS/cisco-wb-kmod.git;subpath=src/drivers;branch=main;rev=e4c871fdc07f453701de40937a319760175958e5;protocol=ssh \
"

S = "${WORKDIR}"
MODULES_MODULE_SYMVERS_LOCATION = "drivers"

KERNEL_MODULE_AUTOLOAD += " \
    cisco-util \
    cisco-fpga-bmc \
    cisco-fpga-info \
    cisco-fpga-pseq \
"

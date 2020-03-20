# Copyright 2018-present Facebook. All Rights Reserved.

### ZSTD support taken from latest image_types.bbclass ###
# Once we move to a Yocto version that includes this in image_types.bbclass,
# we should remove it.
IMAGE_TYPES += "cpio.zst"
COMPRESSIONTYPES += "zst"

ZSTD_COMPRESSION_LEVEL="-19"

CONVERSION_CMD_zst = "zstd -f -k -T0 -c ${ZSTD_COMPRESSION_LEVEL} ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type} > ${IMAGE_NAME}${IMAGE_NAME_SUFFIX}.${type}.zst"
CONVERSION_DEPENDS_zst = "zstd-native"
### end ZSTD support ###

# Base this image on core-image-minimal
require recipes-core/images/core-image-minimal.bb

ROOTFS_POSTPROCESS_COMMAND_append += " openbmc_rootfs_fixup; "

OPENBMC_HOSTNAME ?= "bmc"

openbmc_rootfs_fixup() {
    # hostname
    if [ "${OPENBMC_HOSTNAME}" != "" ]; then
        echo ${OPENBMC_HOSTNAME} > ${IMAGE_ROOTFS}/etc/hostname
    else
        echo ${MACHINE} > ${IMAGE_ROOTFS}/etc/hostname
    fi

    # version
    echo "OpenBMC Release ${OPENBMC_VERSION}" > ${IMAGE_ROOTFS}/etc/issue
    echo >> ${IMAGE_ROOTFS}/etc/issue
    echo "OpenBMC Release ${OPENBMC_VERSION} %h" > ${IMAGE_ROOTFS}/etc/issue.net
    echo >> ${IMAGE_ROOTFS}/etc/issue.net

    # Remove all *.pyc files
    find ${IMAGE_ROOTFS} -type f -name "*.pyc" -exec rm -f {} \;
}

IMAGE_FEATURES_append = " \
    ssh-server-openssh \
    tools-debug \
    "

PACKAGE_EXCLUDE += "\ 
    gnutls \
    perl \
    "
# Nearly every package that uses TLS/SSL RRECOMMENDS the CA certificate
# package.  Since we should not be initiating SSL traffic from a BMC
# to outside of our own network, there is no value in all of these
# certificates.  Add it to the BAD_RECOMMENDATIONS so it is not installed
# unless there is a strict need for it.
BAD_RECOMMENDATIONS += " \
    ca-certificates \
    "

# Some packages have unit-tests but are not installed directly into an
# image.  Add these to the NATIVE_UNIT_TESTS variable and cause the image
# to DEPEND on it so the package is built and unit-tested.
NATIVE_UNIT_TESTS += " \
    pypartition-native \
    rest-api-native \
    "
DEPENDS += "${NATIVE_UNIT_TESTS}"

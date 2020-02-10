# Copyright 2018-present Facebook. All Rights Reserved.

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

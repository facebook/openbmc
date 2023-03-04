ROOTFS_POSTPROCESS_COMMAND += "openbmc_rootfs_fixup;"

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

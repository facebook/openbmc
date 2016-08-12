# Copyright 2016-present Facebook. All Rights Reserved.
SUMMARY = "Verified boot features for U-boot"
HOMEPAGE = "http://www.denx.de/wiki/U-Boot/WebHome"
SECTION = "bootloaders"
PR = "r0"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://certificate-store.dts;beginline=4;endline=16;md5=da35978751a9d71b73679307c4d296ec"

DEPENDS += "dtc-native"

SRC_URI = "file://certificate-store.dts \
           file://placebo \
           "
S = "${WORKDIR}"

require verified-boot.inc

# Append the key directory to the source files.
SRC_URI_append += '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "file://${VERIFIED_BOOT_KEYS}", "", d)} '
SRC_URI_append_u-boot += '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "file://${CERTIFICATE_STORE}", "", d)} '

do_configure () {
    if [ "x${VERIFIED_BOOT}" = "x" ]
    then
        return
    fi

    # Check that the required VERIFIED_BOOT_KEYNAME path exists.
    if [ ! -d "${VERIFIED_BOOT_KEYS}" ]
    then
        echo "Cannot find VERIFIED_BOOT_KEYS directory: ${VERIFIED_BOOT_KEYS}"
        echo "To use verified-boot, please configure a keydir and keypair."
        exit 1
    fi

    KEYPAIR="${VERIFIED_BOOT_KEYS}/${VERIFIED_BOOT_KEYNAME}.key"
    if [ ! -f "${KEYPAIR}" ]
    then
        echo "Cannot find configured keypair: ${KEYPAIR}"
        echo "To use verified-boot please configure a keydir and keypair."
        exit 1
    fi

    # Remove any artifacts from previous configure/compiles.
    rm -f certificate-store.dtb
}

do_compile () {
    if [ "x${VERIFIED_BOOT}" = "x" ]
    then
        return
    fi

    # Install the empty certificate store into the image output.
    # This empty store will have public keys injected when the firmware is
    # compiled (mkimage stage).
    dtc certificate-store.dts -o certificate-store.dtb -O dtb

    # Create a 'placebo' FIT configuration and compile to a semi-blank FIT.
    # This FIT will be signed, the keys will be extracted, and it will be deleted.
    # If the u-boot mkimage tool could create DTBs will public keys this step
    # would not be required.
    fitimage_emit_placebo
    mkimage -f placebo.its placebo.fit

    # Using the placebo FIT, which requested signing of from the configured key,
    # extract the public keys into the DTB.
    # This process could be improved with direct DTB writing.
    mkimage -k "${VERIFIED_BOOT_KEYS}" -r -K certificate-store.dtb -F placebo.fit

    # Clean up the resultant FIT.
    rm -f placebo.fit
}

do_install () {
    if [ "x${VERIFIED_BOOT}" = "x" ]
    then
        return
    fi

    install certificate-store.dtb "${CERTIFICATE_STORE}"
    ln -sf "${CERTIFICATE_STORE}" "${CERTIFICATE_STORE_LINK}"
}

# IMAGE_FEATURES control content of the core reference images
#
# Available IMAGE_FEATURES:
#
# - verified-boot      - Enables a verified boot in u-boot for the AST2400
# - verified-boot-sign - Enables an online sign of the FIT
#
# For both of these features a public key is needed.
# The public key should be configured within the build ./conf directory and
# should be specified within the ./conf/local configuration.
#
# VERIFIED_BOOT_KEYS ??= "keys"
# VERIFIED_BOOT_KEYNAME ?= "dev"
#
IMAGE_FEATURES[validitems] += " verified-boot verified-boot-sign "
VERIFIED_BOOT = '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "True", "", d)}'
VERIFIED_BOOT_SIGN = '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot-sign", "True", "", d)}'
VERIFIED_BOOT_KEYS ??= '${@os.getenv("PWD", "/tmp")}/conf/keys'
VERIFIED_BOOT_KEYNAME ??= "dev"

CERTIFICATE_STORE ?= "${DEPLOY_DIR_IMAGE}/certificate-store-${DATETIME}.dtb"
CERTIFICATE_STORE_LINK ?= "${DEPLOY_DIR_IMAGE}/certificate-store.dtb"

# Append OEMAKE flags for u-boot.
# These Kbuild options tell u-boot to expect an 'embedded' DTB, we use this
# to inject signing public keys into the u-boot firmware.
EXTRA_OEMAKE_append_pn-u-boot = ' CONFIG_OF_EMBED=${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "y", "", d)} '
EXTRA_OEMAKE_append_pn-u-boot += ' CONFIG_OF_CONTROL=${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "y", "", d)} '
EXTRA_OEMAKE_append_pn-u-boot += ' EXT_DTB=${CERTIFICATE_STORE} '

# Use a placebo FIT for extracted public keys.
VERIFIED_BOOT_PLACEBO ?= "placebo.its"

# Emit the fitImage ITS header
# $1 - FIT source file
fitimage_emit_fit_header() {
    cat << EOF >> ${1}
/dts-v1/;

/ {
        description = "U-Boot fitImage for ${DISTRO_NAME}/${PV}/${MACHINE}";
        #address-cells = <1>;
EOF
}

# Emit the fitImage section bits
# $1 - FIT source file
# $2 - section type:
#   imagestart - image section start
#   confstart  - configuration section start
#   sectend    - section end
#   fitend     - fitimage end
fitimage_emit_section_maint() {
    SOURCE=$1
    case $2 in
    imagestart)
        cat << EOF >> ${SOURCE}

        images {
EOF
    ;;
    confstart)
        cat << EOF >> ${SOURCE}

        configurations {
EOF
    ;;
    sectend)
        cat << EOF >> ${SOURCE}
    };
EOF
    ;;
    fitend)
        cat << EOF >> ${SOURCE}
};
EOF
    ;;
    esac
}

#
# Emit the fitImage ITS kernel section
#
# $1 - FIT source
# $2 - Image counter
# $3 - Path to kernel image
# $4 - Compression type
# $5 - Load address
# $6 - Entry point
fitimage_emit_section_kernel() {
    HASH_ALGORITHM="sha1"
    SOURCE=$1
    IMAGE_COUNTER=$2
    IMAGE_KERNEL=$3
    IMAGE_COMPRESSION=$4
    LOAD_ADDRESS=$5
    ENTRY_POINT=$6

    cat << EOF >> ${SOURCE}
        kernel@${IMAGE_COUNTER} {
            description = "Linux kernel";
            data = /incbin/("${IMAGE_KERNEL}");
            type = "kernel";
            arch = "${UBOOT_ARCH}";
            os = "linux";
            compression = "${IMAGE_COMPRESSION}";
            load = <${LOAD_ADDRESS}>;
            entry = <${ENTRY_POINT}>;
            hash@1 {
                algo = "${HASH_ALGORITHM}";
            };
        };
EOF
}

#
# Emit the fitImage ITS ramdisk section
#
# $1 - FIT source
# $2 - Image counter
# $3 - Path to ramdisk image
# $4 - Compression type
# $5 - Load address
fitimage_emit_section_ramdisk() {
    HASH_ALGORITHM="sha1"
    SOURCE=$1
    IMAGE_COUNTER=$2
    IMAGE_RAMDISK=$3
    IMAGE_COMPRESSION=$4
    LOAD_ADDRESS=$5

    cat << EOF >> ${SOURCE}
        ramdisk@${IMAGE_COUNTER} {
            description = "RAMDISK";
            data = /incbin/("${IMAGE_RAMDISK}");
            type = "ramdisk";
            arch = "${UBOOT_ARCH}";
            compression = "${IMAGE_COMPRESSION}";
            load = <${LOAD_ADDRESS}>;
            hash@1 {
                algo = "${HASH_ALGORITHM}";
            };
        };
EOF
}

#
# Emit the fitImage ITS configuration section
#
# $1 - FIT source
# $2 - Linux kernel ID
# $3 - Ramdisk image ID
fitimage_emit_section_config() {
    SOURCE=$1
    conf_csum="sha1"

    # Test if we have any DTBs at all
    if [ -z "${3}" ] ; then
        conf_desc="Boot Linux kernel"
        ramdisk_line=""
    else
        conf_desc="Boot Linux kernel with ramdisk"
        ramdisk_line="ramdisk = \"ramdisk@${3}\";"
    fi
    kernel_line="kernel = \"kernel@${2}\";"

    cat << EOF >> ${SOURCE}
        default = "conf@1";
        conf@1 {
            description = "${conf_desc}";
            ${kernel_line}
            ${ramdisk_line}
            hash@1 {
                algo = "${conf_csum}";
            };
            signature@1 {
                algo = "sha1,rsa2048";
                key-name-hint = "${VERIFIED_BOOT_KEYNAME}";
                sign-images = "kernel", "ramdisk";
            };
        };
EOF
}

fitimage_emit_placebo() {
    rm -f ${VERIFIED_BOOT_PLACEBO}
    fitimage_emit_fit_header ${VERIFIED_BOOT_PLACEBO}
    fitimage_emit_section_maint ${VERIFIED_BOOT_PLACEBO} imagestart
    fitimage_emit_section_kernel "${VERIFIED_BOOT_PLACEBO}" 1 "./placebo" "none" 0 0
    fitimage_emit_section_ramdisk "${VERIFIED_BOOT_PLACEBO}" 1 "./placebo" "none" 0
    fitimage_emit_section_maint ${VERIFIED_BOOT_PLACEBO} sectend
    fitimage_emit_section_maint ${VERIFIED_BOOT_PLACEBO} confstart
    fitimage_emit_section_config ${VERIFIED_BOOT_PLACEBO} 1 1
    fitimage_emit_section_maint ${VERIFIED_BOOT_PLACEBO} sectend
    fitimage_emit_section_maint ${VERIFIED_BOOT_PLACEBO} fitend
}

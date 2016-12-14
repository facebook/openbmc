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
IMAGE_FEATURES[validitems] += "rom-boot verified-boot verified-boot-sign"
ROM_BOOT = '${@bb.utils.contains("IMAGE_FEATURES", "rom-boot", "True", "", d)}'
VERIFIED_BOOT = '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "True", "", d)}'
VERIFIED_BOOT_SIGN = '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot-sign", "True", "", d)}'
VERIFIED_BOOT_KEYS ??= '${@os.getenv("PWD", "/tmp")}/conf/keys'
VERIFIED_BOOT_KEYNAME ??= "dev"
VERIFIED_BOOT_ALGORITHM ??= "sha256,rsa4096"

CERTIFICATE_STORE ?= "certificate-store.dtb"

# Append the key directory to the source files.
SRC_URI_append += '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "file://${VERIFIED_BOOT_KEYS}", "", d)} '

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
# Emit the fitImage ITS firmware section
#
# $1 - FIT source
# $2 - Image counter
# $3 - Path to U-Boot image
# $4 - Load address
# $5 - Entry point
fitimage_emit_section_firmware() {
    HASH_ALGORITHM="sha1"
    SOURCE=$1
    IMAGE_COUNTER=$2
    IMAGE_FIRMWARE=$3
    LOAD_ADDRESS=$4
    ENTRY_POINT=$5

    cat << EOF >> ${SOURCE}
        firmware@${IMAGE_COUNTER} {
            description = "u-boot";
            data = /incbin/("${IMAGE_FIRMWARE}");
            type = "firmware";
            arch = "${UBOOT_ARCH}";
            os = "u-boot";
            compression = "none";
            load = <${LOAD_ADDRESS}>;
            entry = <${ENTRY_POINT}>;
            hash@1 {
                algo = "${HASH_ALGORITHM}";
            };
        };
EOF
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
# Emit the fitImage ITS firmware configuration section
#
# $1 - FIT source
# $2 - Firmware ID
fitimage_emit_section_firmware_config() {
    SOURCE=$1
    conf_csum="sha1"
    conf_desc="Boot U-Boot"

    firmware_line="firmware = \"firmware@${2}\";"

    cat << EOF >> ${SOURCE}
        default = "conf@1";
        conf@1 {
            description = "${conf_desc}";
            ${firmware_line}
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
                algo = "${VERIFIED_BOOT_ALGORITHM}";
                key-name-hint = "${VERIFIED_BOOT_KEYNAME}";
                sign-images = "kernel", "ramdisk";
            };
        };
EOF
}

fitimage_emit_placebo() {
    VERIFIED_BOOT_PLACEBO=${1}
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

uboot_option_on() {
  OPTION=${1}
  SOURCE=${2}
  sed -i "s/^# ${OPTION}=/${OPTION}=/g" ${SOURCE}
}

uboot_option_off() {
  OPTION=${1}
  SOURCE=${2}
  sed -i "s/^${OPTION}=/# ${OPTION}=/g" ${SOURCE}
}

# Copyright 2019-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA

# IMAGE_FEATURES control content of the core reference images
#
# Available IMAGE_FEATURES:
#
# - verified-boot      - Enables a verified boot in u-boot for the AST2500
#

IMAGE_FEATURES[validitems] += "verified-boot"

VERIFIED_BOOT = '${@bb.utils.contains("IMAGE_FEATURES", "verified-boot", "True", "", d)}'

VERIFIED_BOOT_ALGORITHM ??= "sha256,rsa4096"
VERIFIED_BOOT_HASH_ALGORITHM ??= "sha256"
VERIFIED_BOOT_KEYNAME ??= "subordinate"

# Emit the fitImage ITS header
# $1 - FIT source file
fitimage_emit_fit_header() {
    mkdir -p $(dirname ${1})
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
fitimage_emit_section_firmware() {
    SOURCE=$1
    IMAGE_COUNTER=$2
    IMAGE_FIRMWARE=$3

    cat << EOF >> ${SOURCE}
        firmware@${IMAGE_COUNTER} {
            description = "u-boot";
            data = /incbin/("${IMAGE_FIRMWARE}");
            type = "firmware";
            arch = "${UBOOT_ARCH}";
            os = "u-boot";
            compression = "none";
            hash@1 {
                algo = "${VERIFIED_BOOT_HASH_ALGORITHM}";
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
                algo = "${VERIFIED_BOOT_HASH_ALGORITHM}";
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
fitimage_emit_section_ramdisk() {
    SOURCE=$1
    IMAGE_COUNTER=$2
    IMAGE_RAMDISK=$3
    IMAGE_COMPRESSION=$4

    cat << EOF >> ${SOURCE}
        ramdisk@${IMAGE_COUNTER} {
            description = "RAMDISK";
            data = /incbin/("${IMAGE_RAMDISK}");
            type = "ramdisk";
            os = "linux";
            arch = "${UBOOT_ARCH}";
            compression = "${IMAGE_COMPRESSION}";
            hash@1 {
                algo = "${VERIFIED_BOOT_HASH_ALGORITHM}";
            };
        };
EOF
}

#
# Emit the fitImage ITS DTB section
#
# $1 ... FIT source
# $2 ... Image counter
# $3 ... Path to DTB image
fitimage_emit_section_dtb() {
    cat << EOF >> ${1}
        fdt@${2} {
            description = "Flattened Device Tree blob";
            data = /incbin/("${3}");
            type = "flat_dt";
            arch = "${UBOOT_ARCH}";
            compression = "none";
            hash@1 {
                algo = "${VERIFIED_BOOT_HASH_ALGORITHM}";
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
            signature@1 {
                algo = "${VERIFIED_BOOT_ALGORITHM}";
                key-name-hint = "${VERIFIED_BOOT_KEYNAME}";
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
# $4 - DTB image ID
fitimage_emit_section_config() {
    SOURCE=$1
    conf_desc="Boot Linux kernel"

    # Test if we have ramdisk
    if [ -z "${3}" ] ; then
        ramdisk_line=""
    else
        conf_desc="${conf_desc} with ramdisk"
        ramdisk_line="ramdisk = \"ramdisk@${3}\";"
    fi

    # Test if we have any DTBs at all
    if [ -z "${4}" ] ; then
        fdt_line=""
    else
        conf_desc="${conf_desc} and device tree blob"
        fdt_line="fdt = \"fdt@${4}\";"
    fi
    kernel_line="kernel = \"kernel@${2}\";"

    cat << EOF >> ${SOURCE}
        default = "conf@1";
        conf@1 {
            description = "${conf_desc}";
            ${kernel_line}
            ${ramdisk_line}
            ${fdt_line}
            signature@1 {
                algo = "${VERIFIED_BOOT_ALGORITHM}";
                key-name-hint = "${VERIFIED_BOOT_KEYNAME}";
                sign-images = "kernel", "ramdisk";
            };
        };
EOF
}

defconfig_option_on() {
  OPTION=${1}
  SOURCE=${2}
  sed -i "s/^# ${OPTION}=/${OPTION}=/g" ${SOURCE}
  sed -i "s/^# ${OPTION} is not set/${OPTION}=y/g" ${SOURCE}
}

defconfig_option_off() {
  OPTION=${1}
  SOURCE=${2}
  sed -i "s/^${OPTION}=/# ${OPTION}=/g" ${SOURCE}
}

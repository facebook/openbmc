#!/usr/bin/env python3

"""Script to create PLDM FW update package"""

import argparse
import binascii
import enum
import json
import math
import os
import struct
import sys
from datetime import datetime

from bitarray import bitarray
from bitarray.util import ba2int

string_types = dict(
    [
        ("Unknown", 0),
        ("ASCII", 1),
        ("UTF8", 2),
        ("UTF16", 3),
        ("UTF16LE", 4),
        ("UTF16BE", 5),
    ]
)

initial_descriptor_type_name_length = {
    0x0000: ["PCI Vendor ID", 2],
    0x0001: ["IANA Enterprise ID", 4],
    0x0002: ["UUID", 16],
    0x0003: ["PnP Vendor ID", 3],
    0x0004: ["ACPI Vendor ID", 4],
}

descriptor_type_name_length = {
    0x0000: ["PCI Vendor ID", 2],
    0x0001: ["IANA Enterprise ID", 4],
    0x0002: ["UUID", 16],
    0x0003: ["PnP Vendor ID", 3],
    0x0004: ["ACPI Vendor ID", 4],
    0x0100: ["PCI Device ID", 2],
    0x0101: ["PCI Subsystem Vendor ID", 2],
    0x0102: ["PCI Subsystem ID", 2],
    0x0103: ["PCI Revision ID", 1],
    0x0104: ["PnP Product Identifier", 4],
    0x0105: ["ACPI Product Identifier", 4],
}


class ComponentOptions(enum.IntEnum):
    """
    Enum to represent ComponentOptions
    """

    ForceUpdate = 0
    UseComponentCompStamp = 1


def check_string_length(string):
    """Check if the length of the string is not greater than 255."""
    if len(string) > 255:
        sys.exit("ERROR: Max permitted string length is 255")


def write_pkg_release_date_time(pldm_fw_up_pkg, release_date_time):
    """
    Write the timestamp into the package header. The timestamp is formatted as
    series of 13 bytes defined in DSP0240 specification.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
            release_date_time: Package Release Date Time
    """
    time = release_date_time.time()
    date = release_date_time.date()
    us_bytes = time.microsecond.to_bytes(3, byteorder="little")
    pldm_fw_up_pkg.write(
        struct.pack(
            "<hBBBBBBBBHB",
            0,
            us_bytes[0],
            us_bytes[1],
            us_bytes[2],
            time.second,
            time.minute,
            time.hour,
            date.day,
            date.month,
            date.year,
            0,
        )
    )


def write_package_version_string(pldm_fw_up_pkg, metadata):
    """
    Write PackageVersionStringType, PackageVersionStringLength and
    PackageVersionString to the package header.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
            metadata: metadata about PLDM FW update package
    """
    # Hardcoded string type to ASCII
    string_type = string_types["ASCII"]
    package_version_string = metadata["PackageHeaderInformation"][
        "PackageVersionString"
    ]
    check_string_length(package_version_string)
    format_string = "<BB" + str(len(package_version_string)) + "s"
    pldm_fw_up_pkg.write(
        struct.pack(
            format_string,
            string_type,
            len(package_version_string),
            package_version_string.encode("ascii"),
        )
    )


def write_component_bitmap_bit_length(pldm_fw_up_pkg, metadata):
    """
    ComponentBitmapBitLength in the package header indicates the number of bits
    that will be used represent the bitmap in the ApplicableComponents field
    for a matching device. The value shall be a multiple of 8 and be large
    enough to contain a bit for each component in the package. The number of
    components in the JSON file is used to populate the bitmap length.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
            metadata: metadata about PLDM FW update package

        Returns:
            ComponentBitmapBitLength: number of bits that will be used
            represent the bitmap in the ApplicableComponents field for a
            matching device
    """
    # The script supports upto 32 components now
    max_components = 32
    bitmap_multiple = 8

    num_components = len(metadata["ComponentImageInformationArea"])
    if num_components > max_components:
        sys.exit("ERROR: only upto 32 components supported now")
    component_bitmap_bit_length = bitmap_multiple * math.ceil(
        num_components / bitmap_multiple
    )
    pldm_fw_up_pkg.write(struct.pack("<H", int(component_bitmap_bit_length)))
    return component_bitmap_bit_length


def write_pkg_header_info(pldm_fw_up_pkg, metadata):
    """
    ComponentBitmapBitLength in the package header indicates the number of bits
    that will be used represent the bitmap in the ApplicableComponents field
    for a matching device. The value shall be a multiple of 8 and be large
    enough to contain a bit for each component in the package. The number of
    components in the JSON file is used to populate the bitmap length.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
            metadata: metadata about PLDM FW update package

        Returns:
            ComponentBitmapBitLength: number of bits that will be used
            represent the bitmap in the ApplicableComponents field for a
            matching device
    """
    uuid = metadata["PackageHeaderInformation"]["PackageHeaderIdentifier"]
    package_header_identifier = bytearray.fromhex(uuid)
    pldm_fw_up_pkg.write(package_header_identifier)

    package_header_format_revision = metadata["PackageHeaderInformation"][
        "PackageHeaderFormatVersion"
    ]
    # Size will be computed and updated subsequently
    package_header_size = 0
    pldm_fw_up_pkg.write(
        struct.pack("<BH", package_header_format_revision, package_header_size)
    )

    release_date_time_str = metadata["PackageHeaderInformation"].get(
        "PackageReleaseDateTime", None
    )

    if release_date_time_str is not None:
        formats = [
            "%Y-%m-%dT%H:%M:%S",
            "%Y-%m-%d %H:%M:%S",
            "%d/%m/%Y %H:%M:%S",
        ]
        release_date_time = None
        for fmt in formats:
            try:
                release_date_time = datetime.strptime(
                    release_date_time_str, fmt
                )
                break
            except ValueError:
                pass
        if release_date_time is None:
            sys.exit("Can't parse release date '%s'" % release_date_time_str)
    else:
        release_date_time = datetime.now()

    write_pkg_release_date_time(pldm_fw_up_pkg, release_date_time)

    component_bitmap_bit_length = write_component_bitmap_bit_length(
        pldm_fw_up_pkg, metadata
    )
    write_package_version_string(pldm_fw_up_pkg, metadata)
    return component_bitmap_bit_length


def get_applicable_components(device, components, component_bitmap_bit_length):
    """
    This function figures out the components applicable for the device and sets
    the ApplicableComponents bitfield accordingly.

        Parameters:
            device: device information
            components: list of components in the package
            component_bitmap_bit_length: length of the ComponentBitmapBitLength

        Returns:
            The ApplicableComponents bitfield
    """
    applicable_components_list = device["ApplicableComponents"]
    applicable_components = bitarray(
        component_bitmap_bit_length, endian="little"
    )
    applicable_components.setall(0)
    for component_index in applicable_components_list:
        if 0 <= component_index < len(components):
            applicable_components[component_index] = 1
        else:
            sys.exit("ERROR: Applicable Component index not found.")
    return applicable_components


def prepare_record_descriptors(descriptors):
    """
    This function processes the Descriptors and prepares the RecordDescriptors
    section of the the firmware device ID record.

        Parameters:
            descriptors: Descriptors entry

        Returns:
            RecordDescriptors, DescriptorCount
    """
    record_descriptors = bytearray()
    vendor_defined_desc_type = 65535
    vendor_desc_title_str_type_len = 1
    vendor_desc_title_str_len_len = 1
    descriptor_count = 0

    for descriptor in descriptors:
        descriptor_type = descriptor["DescriptorType"]
        if descriptor_count == 0:
            if (
                initial_descriptor_type_name_length.get(descriptor_type)
                is None
            ):
                sys.exit("ERROR: Initial descriptor type not supported")
        else:
            if (
                descriptor_type_name_length.get(descriptor_type) is None
                and descriptor_type != vendor_defined_desc_type
            ):
                sys.exit("ERROR: Descriptor type not supported")

        if descriptor_type == vendor_defined_desc_type:
            vendor_desc_title_str = descriptor[
                "VendorDefinedDescriptorTitleString"
            ]
            vendor_desc_data = descriptor["VendorDefinedDescriptorData"]
            check_string_length(vendor_desc_title_str)
            vendor_desc_title_str_type = string_types["ASCII"]
            descriptor_length = (
                vendor_desc_title_str_type_len
                + vendor_desc_title_str_len_len
                + len(vendor_desc_title_str)
                + len(bytearray.fromhex(vendor_desc_data))
            )
            format_string = "<HHBB" + str(len(vendor_desc_title_str)) + "s"
            record_descriptors.extend(
                struct.pack(
                    format_string,
                    descriptor_type,
                    descriptor_length,
                    vendor_desc_title_str_type,
                    len(vendor_desc_title_str),
                    vendor_desc_title_str.encode("ascii"),
                )
            )
            record_descriptors.extend(bytearray.fromhex(vendor_desc_data))
            descriptor_count += 1
        else:
            descriptor_type = descriptor["DescriptorType"]
            descriptor_data = descriptor["DescriptorData"]
            descriptor_length = len(bytearray.fromhex(descriptor_data))
            if (
                descriptor_length
                != descriptor_type_name_length.get(descriptor_type)[1]
            ):
                err_string = (
                    "ERROR: Descriptor type - "
                    + descriptor_type_name_length.get(descriptor_type)[0]
                    + " length is incorrect"
                )
                sys.exit(err_string)
            format_string = "<HH"
            record_descriptors.extend(
                struct.pack(format_string, descriptor_type, descriptor_length)
            )
            record_descriptors.extend(bytearray.fromhex(descriptor_data))
            descriptor_count += 1
    return record_descriptors, descriptor_count


def write_fw_device_identification_area(
    pldm_fw_up_pkg, metadata, component_bitmap_bit_length
):
    """
    Write firmware device ID records into the PLDM package header

    This function writes the DeviceIDRecordCount and the
    FirmwareDeviceIDRecords into the firmware update package by processing the
    metadata JSON. Currently there is no support for optional
    FirmwareDevicePackageData.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
            metadata: metadata about PLDM FW update package
            component_bitmap_bit_length: length of the ComponentBitmapBitLength
    """
    # The spec limits the number of firmware device ID records to 255
    max_device_id_record_count = 255
    devices = metadata["FirmwareDeviceIdentificationArea"]
    device_id_record_count = len(devices)
    if device_id_record_count > max_device_id_record_count:
        sys.exit(
            "ERROR: there can be only upto 255 entries in the                "
            " FirmwareDeviceIdentificationArea section"
        )

    # DeviceIDRecordCount
    pldm_fw_up_pkg.write(struct.pack("<B", device_id_record_count))

    for device in devices:
        # RecordLength size
        record_length = 2

        # DescriptorCount
        record_length += 1

        # DeviceUpdateOptionFlags
        device_update_option_flags = bitarray(32, endian="little")
        device_update_option_flags.setall(0)
        # Continue component updates after failure
        supported_device_update_option_flags = [0]
        for option in device["DeviceUpdateOptionFlags"]:
            if option not in supported_device_update_option_flags:
                sys.exit("ERROR: unsupported DeviceUpdateOptionFlag entry")
            device_update_option_flags[option] = 1
        record_length += 4

        # ComponentImageSetVersionStringType supports only ASCII for now
        component_image_set_version_string_type = string_types["ASCII"]
        record_length += 1

        # ComponentImageSetVersionStringLength
        component_image_set_version_string = device[
            "ComponentImageSetVersionString"
        ]
        check_string_length(component_image_set_version_string)
        record_length += len(component_image_set_version_string)
        record_length += 1

        # Optional FirmwareDevicePackageData not supported now,
        # FirmwareDevicePackageDataLength is set to 0x0000
        fw_device_pkg_data_length = 0
        record_length += 2

        # ApplicableComponents
        components = metadata["ComponentImageInformationArea"]
        applicable_components = get_applicable_components(
            device, components, component_bitmap_bit_length
        )
        applicable_components_bitfield_length = round(
            len(applicable_components) / 8
        )
        record_length += applicable_components_bitfield_length

        # RecordDescriptors
        descriptors = device["Descriptors"]
        record_descriptors, descriptor_count = prepare_record_descriptors(
            descriptors
        )
        record_length += len(record_descriptors)

        format_string = (
            "<HBIBBH"
            + str(applicable_components_bitfield_length)
            + "s"
            + str(len(component_image_set_version_string))
            + "s"
        )
        pldm_fw_up_pkg.write(
            struct.pack(
                format_string,
                record_length,
                descriptor_count,
                ba2int(device_update_option_flags),
                component_image_set_version_string_type,
                len(component_image_set_version_string),
                fw_device_pkg_data_length,
                applicable_components.tobytes(),
                component_image_set_version_string.encode("ascii"),
            )
        )
        pldm_fw_up_pkg.write(record_descriptors)


def get_component_comparison_stamp(component):
    """
    Get component comparison stamp from metadata file.

    This function checks if ComponentOptions field is having value 1. For
    ComponentOptions 1, ComponentComparisonStamp value from metadata file
    is used and Default value 0xFFFFFFFF is used for other Component Options.

    Parameters:
        component: Component image info
    Returns:
        component_comparison_stamp: Component Comparison stamp
    """
    component_comparison_stamp = 0xFFFFFFFF
    if (
        int(ComponentOptions.UseComponentCompStamp)
        in component["ComponentOptions"]
    ):
        # Use FD vendor selected value from metadata file
        if "ComponentComparisonStamp" not in component.keys():
            sys.exit(
                "ERROR: ComponentComparisonStamp is required"
                " when value '1' is specified in ComponentOptions field"
            )
        else:
            try:
                tmp_component_cmp_stmp = int(
                    component["ComponentComparisonStamp"], 16
                )
                if 0 < tmp_component_cmp_stmp < 0xFFFFFFFF:
                    component_comparison_stamp = tmp_component_cmp_stmp
                else:
                    sys.exit(
                        "ERROR: Value for ComponentComparisonStamp "
                        " should be  [0x01 - 0xFFFFFFFE] when "
                        "ComponentOptions bit is set to"
                        "'1'(UseComponentComparisonStamp)"
                    )
            except ValueError:  # invalid hext format
                sys.exit("ERROR: Invalid hex for ComponentComparisonStamp")
    return component_comparison_stamp


def write_component_image_info_area(pldm_fw_up_pkg, metadata, image_files):
    """
    Write component image information area into the PLDM package header

    This function writes the ComponentImageCount and the
    ComponentImageInformation into the firmware update package by processing
    the metadata JSON.

    Parameters:
        pldm_fw_up_pkg: PLDM FW update package
        metadata: metadata about PLDM FW update package
        image_files: component images
    """
    components = metadata["ComponentImageInformationArea"]
    # ComponentImageCount
    pldm_fw_up_pkg.write(struct.pack("<H", len(components)))
    component_location_offsets = []
    # ComponentLocationOffset position in individual component image
    # information
    component_location_offset_pos = 12

    for component in components:
        # Record the location of the ComponentLocationOffset to be updated
        # after appending images to the firmware update package
        component_location_offsets.append(
            pldm_fw_up_pkg.tell() + component_location_offset_pos
        )

        # ComponentClassification
        component_classification = component["ComponentClassification"]
        if component_classification < 0 or component_classification > 0xFFFF:
            sys.exit(
                "ERROR: ComponentClassification should be [0x0000 - 0xFFFF]"
            )

        # ComponentIdentifier
        component_identifier = component["ComponentIdentifier"]
        if component_identifier < 0 or component_identifier > 0xFFFF:
            sys.exit("ERROR: ComponentIdentifier should be [0x0000 - 0xFFFF]")

        # ComponentComparisonStamp
        component_comparison_stamp = get_component_comparison_stamp(component)

        # ComponentOptions
        component_options = bitarray(16, endian="little")
        component_options.setall(0)
        supported_component_options = [0, 1, 2]
        for option in component["ComponentOptions"]:
            if option not in supported_component_options:
                sys.exit(
                    "ERROR: unsupported ComponentOption in                   "
                    " ComponentImageInformationArea section"
                )
            component_options[option] = 1

        # RequestedComponentActivationMethod
        requested_component_activation_method = bitarray(16, endian="little")
        requested_component_activation_method.setall(0)
        supported_requested_component_activation_method = [0, 1, 2, 3, 4, 5]
        for option in component["RequestedComponentActivationMethod"]:
            if option not in supported_requested_component_activation_method:
                sys.exit(
                    "ERROR: unsupported RequestedComponent                    "
                    "    ActivationMethod entry"
                )
            requested_component_activation_method[option] = 1

        # ComponentLocationOffset
        component_location_offset = 0
        # ComponentSize
        component_size = 0
        # ComponentVersionStringType
        component_version_string_type = string_types["ASCII"]
        # ComponentVersionStringlength
        # ComponentVersionString
        component_version_string = component["ComponentVersionString"]
        check_string_length(component_version_string)

        format_string = "<HHIHHIIBB" + str(len(component_version_string)) + "s"
        pldm_fw_up_pkg.write(
            struct.pack(
                format_string,
                component_classification,
                component_identifier,
                component_comparison_stamp,
                ba2int(component_options),
                ba2int(requested_component_activation_method),
                component_location_offset,
                component_size,
                component_version_string_type,
                len(component_version_string),
                component_version_string.encode("ascii"),
            )
        )

    index = 0
    pkg_header_checksum_size = 4
    start_offset = pldm_fw_up_pkg.tell() + pkg_header_checksum_size
    # Update ComponentLocationOffset and ComponentSize for all the components
    for offset in component_location_offsets:
        file_size = os.stat(image_files[index]).st_size
        pldm_fw_up_pkg.seek(offset)
        pldm_fw_up_pkg.write(struct.pack("<II", start_offset, file_size))
        start_offset += file_size
        index += 1
    pldm_fw_up_pkg.seek(0, os.SEEK_END)


def write_pkg_header_checksum(pldm_fw_up_pkg):
    """
    Write PackageHeaderChecksum into the PLDM package header.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
    """
    pldm_fw_up_pkg.seek(0)
    package_header_checksum = binascii.crc32(pldm_fw_up_pkg.read())
    pldm_fw_up_pkg.seek(0, os.SEEK_END)
    pldm_fw_up_pkg.write(struct.pack("<I", package_header_checksum))


def update_pkg_header_size(pldm_fw_up_pkg):
    """
    Update PackageHeader in the PLDM package header. The package header size
    which is the count of all bytes in the PLDM package header structure is
    calculated once the package header contents is complete.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
    """
    pkg_header_checksum_size = 4
    file_size = pldm_fw_up_pkg.tell() + pkg_header_checksum_size
    pkg_header_size_offset = 17
    # Seek past PackageHeaderIdentifier and PackageHeaderFormatRevision
    pldm_fw_up_pkg.seek(pkg_header_size_offset)
    pldm_fw_up_pkg.write(struct.pack("<H", file_size))
    pldm_fw_up_pkg.seek(0, os.SEEK_END)


def append_component_images(pldm_fw_up_pkg, image_files):
    """
    Append the component images to the firmware update package.

        Parameters:
            pldm_fw_up_pkg: PLDM FW update package
            image_files: component images
    """
    for image in image_files:
        with open(image, "rb") as file:
            for line in file:
                pldm_fw_up_pkg.write(line)


def main():
    """Create PLDM FW update (DSP0267) package based on a JSON metadata file"""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "pldmfwuppkgname", help="Name of the PLDM FW update package"
    )
    parser.add_argument("metadatafile", help="Path of metadata JSON file")
    parser.add_argument(
        "images",
        nargs="+",
        help=(
            "One or more firmware image paths, in the same order as           "
            " ComponentImageInformationArea entries"
        ),
    )

    args = parser.parse_args()
    image_files = args.images
    with open(args.metadatafile) as file:
        try:
            metadata = json.load(file)
        except ValueError:
            sys.exit("ERROR: Invalid metadata JSON file")

    # Validate the number of component images
    if len(image_files) != len(metadata["ComponentImageInformationArea"]):
        sys.exit(
            "ERROR: number of images passed != number of entries            "
            " in ComponentImageInformationArea"
        )

    try:
        with open(args.pldmfwuppkgname, "w+b") as pldm_fw_up_pkg:
            component_bitmap_bit_length = write_pkg_header_info(
                pldm_fw_up_pkg, metadata
            )
            write_fw_device_identification_area(
                pldm_fw_up_pkg, metadata, component_bitmap_bit_length
            )
            write_component_image_info_area(
                pldm_fw_up_pkg, metadata, image_files
            )
            update_pkg_header_size(pldm_fw_up_pkg)
            write_pkg_header_checksum(pldm_fw_up_pkg)
            append_component_images(pldm_fw_up_pkg, image_files)
            pldm_fw_up_pkg.close()
    except BaseException:
        pldm_fw_up_pkg.close()
        os.remove(args.pldmfwuppkgname)
        raise


if __name__ == "__main__":
    main()

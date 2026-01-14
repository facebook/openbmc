// mmc_pldm_package_rewrapper.cpp
#include "libpldm/firmware_update.h"
#include "libpldm/utils.h"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

static std::ifstream openInputPackage(const std::string& inputPackagePath)
{
    std::ifstream package(inputPackagePath,
                          std::ifstream::in | std::ifstream::binary);
    if (!package.good())
    {
        throw std::runtime_error("Failed to open input file: " +
                                 inputPackagePath);
    }
    return package;
}

static std::ofstream createOutputPackage(const std::string& inputPackagePath)
{
    std::filesystem::path newPackagePath(inputPackagePath + "_re_wrapped");
    std::ofstream newPackage(newPackagePath,
                             std::ofstream::out | std::ofstream::binary);
    if (!newPackage.good())
    {
        throw std::runtime_error("Failed to create output file: " +
                                 newPackagePath.string());
    }

    std::error_code ec;
    std::filesystem::permissions(
        newPackagePath,
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
            std::filesystem::perms::group_read,
        std::filesystem::perm_options::replace, ec);

    return newPackage;
}

static std::vector<uint8_t> initNewDescriptorFor(uint8_t slotNumber)
{
    // Use slot number as ASCII string, stored in an ASCII model-number-short-string
    slotNumber = slotNumber + 1; // Convert 0-based to 1-based slot number
    const auto slotStr = std::to_string(slotNumber);

    std::vector<uint8_t> newDescriptorData{0x07, 0x01, 0x0a, 0x00};

    for (char c : slotStr)
    {
        newDescriptorData.push_back(static_cast<uint8_t>(c));
    }

    const size_t targetSize =
        (sizeof(pldm_descriptor_tlv) - 1) +
        PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING_LENGTH;

    while (newDescriptorData.size() < targetSize)
    {
        newDescriptorData.push_back(0);
    }

    return newDescriptorData;
}

static size_t skipOriginalDescriptors(uint8_t descriptorCount,
                                      const std::vector<uint8_t>& packageHeader,
                                      size_t processedDataIdx)
{
    for (uint8_t i = 0; i < descriptorCount; ++i)
    {
        if (processedDataIdx + 4 > packageHeader.size())
        {
            throw std::runtime_error("Header too small when parsing descriptors");
        }

        const uint16_t currDescriptorLen =
            (static_cast<uint16_t>(packageHeader.at(processedDataIdx + 2)) |
             (static_cast<uint16_t>(packageHeader.at(processedDataIdx + 3)) << 8));

        processedDataIdx += 2 /*DescriptorType*/ + 2 /*DescriptorLength*/ +
                            currDescriptorLen;

        if (processedDataIdx > packageHeader.size())
        {
            throw std::runtime_error("Descriptor parsing ran past header buffer");
        }
    }
    return processedDataIdx;
}

static void outputMMCPackageWithSlotNumberInHeader(
    uint8_t slotNumber, const std::string& inputPackagePath)
{
    auto package = openInputPackage(inputPackagePath);
    auto newPackage = createOutputPackage(inputPackagePath);

    std::vector<uint8_t> headerInfoBuf(sizeof(pldm_package_header_information));
    package.seekg(0);
    package.read(reinterpret_cast<char*>(headerInfoBuf.data()),
                 headerInfoBuf.size());
    if (!package.good())
    {
        throw std::runtime_error("Failed to read package header information");
    }

    auto* pkgHeaderInfo =
        reinterpret_cast<pldm_package_header_information*>(headerInfoBuf.data());

    const size_t originalPkgHeaderSize = pkgHeaderInfo->package_header_size;
    if (originalPkgHeaderSize < sizeof(pldm_package_header_information) + 4)
    {
        throw std::runtime_error("Invalid package_header_size");
    }

    const auto newDescriptorData = initNewDescriptorFor(slotNumber);

    std::vector<uint8_t> packageHeader(originalPkgHeaderSize + newDescriptorData.size());
    package.seekg(0);
    package.read(reinterpret_cast<char*>(packageHeader.data()),
                 originalPkgHeaderSize);
    if (!package.good())
    {
        throw std::runtime_error("Failed to read full original package header");
    }

    pkgHeaderInfo =
        reinterpret_cast<pldm_package_header_information*>(packageHeader.data());

    pkgHeaderInfo->package_header_size += newDescriptorData.size();

    size_t processedDataIdx = 0;

    processedDataIdx += sizeof(pldm_package_header_information) +
                        pkgHeaderInfo->package_version_string_length +
                        1 /*DeviceIDRecordCount*/;

    if (processedDataIdx + sizeof(pldm_firmware_device_id_record) > packageHeader.size())
    {
        throw std::runtime_error("Invalid header: cannot locate DeviceIDRecord");
    }

    auto* recordInfo = reinterpret_cast<pldm_firmware_device_id_record*>(
        packageHeader.data() + processedDataIdx);

    processedDataIdx += sizeof(pldm_firmware_device_id_record) +
                        static_cast<size_t>(std::ceil(
                            static_cast<double>(pkgHeaderInfo->component_bitmap_bit_length) /
                            8.0)) +
                        recordInfo->comp_image_set_version_string_length;

    if (processedDataIdx > packageHeader.size())
    {
        throw std::runtime_error("Invalid header: version/bitmap exceeds buffer");
    }

    processedDataIdx = skipOriginalDescriptors(recordInfo->descriptor_count,
                                               packageHeader, processedDataIdx);

    recordInfo->record_length += newDescriptorData.size();
    recordInfo->descriptor_count += 1;

    packageHeader.insert(packageHeader.begin() + processedDataIdx,
                         newDescriptorData.begin(), newDescriptorData.end());
    processedDataIdx += newDescriptorData.size();

    constexpr uint8_t COMPONENT_IMAGE_COUNT_LENGTH = 2;
    constexpr uint8_t COMPONENT_LOCATION_OFFSET = 12;

    const size_t locationIdx =
        processedDataIdx + COMPONENT_IMAGE_COUNT_LENGTH + COMPONENT_LOCATION_OFFSET;
    if (locationIdx >= packageHeader.size())
    {
        throw std::runtime_error("Invalid header: component location offset index OOB");
    }

    packageHeader.at(locationIdx) =
        static_cast<uint8_t>(packageHeader.at(locationIdx) + newDescriptorData.size());

    // Recompute header checksum (CRC32 over header_size - 4)
    pkgHeaderInfo =
        reinterpret_cast<pldm_package_header_information*>(packageHeader.data());
    const size_t newHeaderSize = pkgHeaderInfo->package_header_size;

    if (newHeaderSize < 4 || newHeaderSize > packageHeader.size())
    {
        throw std::runtime_error("Invalid new package_header_size after insertion");
    }

    const uint32_t checkSum =
        pldm_edac_crc32(packageHeader.data(), newHeaderSize - 4);

    packageHeader.at(newHeaderSize - 4) = static_cast<uint8_t>(checkSum & 0xFF);
    packageHeader.at(newHeaderSize - 3) = static_cast<uint8_t>((checkSum >> 8) & 0xFF);
    packageHeader.at(newHeaderSize - 2) = static_cast<uint8_t>((checkSum >> 16) & 0xFF);
    packageHeader.at(newHeaderSize - 1) = static_cast<uint8_t>((checkSum >> 24) & 0xFF);

    newPackage.write(reinterpret_cast<const char*>(packageHeader.data()),
                     newHeaderSize);

    const auto packageSize = std::filesystem::file_size(inputPackagePath);
    if (packageSize < originalPkgHeaderSize)
    {
        throw std::runtime_error("Invalid package: file smaller than header size");
    }

    package.seekg(static_cast<std::streamoff>(originalPkgHeaderSize));
    std::vector<uint8_t> packageData(
        static_cast<size_t>(packageSize - originalPkgHeaderSize));
    package.read(reinterpret_cast<char*>(packageData.data()),
                 static_cast<std::streamsize>(packageData.size()));
    if (!package.good() && !package.eof())
    {
        throw std::runtime_error("Failed to read remaining package payload");
    }

    newPackage.write(reinterpret_cast<const char*>(packageData.data()),
                     static_cast<std::streamsize>(packageData.size()));
}

static void tryReWrapMMCUpdatePackage(uint8_t slotNumber,
                                      const std::string& inputPackagePath)
{
    try
    {
        outputMMCPackageWithSlotNumberInHeader(slotNumber, inputPackagePath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to re-wrap the MMC update package: " << e.what()
                  << std::endl;
    }
}

int main(int argc, char** argv)
{
    std::string imagePath{};
    uint8_t slotNumber = 0;

    CLI::App app{
        "Re-wrap PLDM MMC firmware update package. Output: ${input_file}_re_wrapped"};
    app.require_subcommand(1);

    auto mmc = app.add_subcommand(
        "mmc", "Re-wrap the MMC firmware update package for a specific slot.");

    mmc->add_option("-f,--file", imagePath, "Path of MMC PLDM update package.")
        ->required()
        ->check(CLI::ExistingFile);

    mmc->add_option("-s,--slot", slotNumber, "Slot number to embed in header.")
        ->required()
        ->check(CLI::Range(0, 3));

    mmc->callback([&]() { tryReWrapMMCUpdatePackage(slotNumber, imagePath); });

    CLI11_PARSE(app, argc, argv);
    return 0;
}

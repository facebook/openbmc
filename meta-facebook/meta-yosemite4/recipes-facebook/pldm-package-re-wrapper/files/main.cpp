#include "libpldm/firmware_update.h"
#include "libpldm/utils.h"

#include <math.h>

#include <CLI/CLI.hpp>

#include <fstream>
#include <iostream>
#include <unordered_set>

std::ifstream openInputPackage(const std::string& inputPackagePath)
{
    std::ifstream package(inputPackagePath,
                          std::ifstream::in | std::ifstream::binary);
    if (!package.good())
    {
        package.close();
        throw std::runtime_error("Failed to open file");
    }
    return package;
}

std::ofstream createOutputPackage(const std::string& inputPackagePath)
{
    std::filesystem::path newPackagePath(inputPackagePath + "_re_wrapped");
    std::ofstream newPackage(newPackagePath,
                             std::ofstream::out | std::ofstream::binary);
    std::filesystem::perms permission = std::filesystem::perms::owner_read |
                                        std::filesystem::perms::owner_write |
                                        std::filesystem::perms::group_read;
    std::filesystem::permissions(newPackagePath, permission);
    if (newPackage.bad())
    {
        newPackage.close();
        throw std::runtime_error("Failed to create file");
    }

    return newPackage;
}

std::vector<uint8_t> initNewDescriptorFor(uint8_t slotNumber)
{
    auto newDescriptor = std::to_string(slotNumber);
    std::vector<uint8_t> newDescriptorData{0x07, 0x01, 0x0a, 0x00};

    std::for_each(newDescriptor.begin(), newDescriptor.end(), [&](auto& c) {
        newDescriptorData.push_back(static_cast<uint8_t>(c));
    });

    while (newDescriptorData.size() <
           sizeof(pldm_descriptor_tlv) - 1 +
               PLDM_FWUP_ASCII_MODEL_NUMBER_SHORT_STRING_LENGTH)
    {
        newDescriptorData.push_back(0);
    }

    return newDescriptorData;
}

size_t skipOriginalDescriptors(uint8_t descriptor_count,
                               const std::vector<uint8_t>& packageHeader,
                               size_t processedDataIdx)
{
    for (unsigned int i = 0; i < descriptor_count; i++)
    {
        uint16_t currDescriptorLen =
            ((uint16_t)packageHeader.at(processedDataIdx +
                                        sizeof(uint16_t) /*DescriptorType*/ + 1)
             << 8) |
            packageHeader.at(processedDataIdx +
                             sizeof(uint16_t) /*DescriptorType*/);
        processedDataIdx += sizeof(uint16_t) /*DescriptorType*/ +
                            sizeof(uint16_t) /*DescriptorLength*/ +
                            currDescriptorLen;
    }

    return processedDataIdx;
}

void outputBICPackageWithSlotNumberInHeader(uint8_t slotNumber,
                                            const std::string& inputPackagePath)
{
    auto package = openInputPackage(inputPackagePath);
    auto newPackage = createOutputPackage(inputPackagePath);
    size_t processedDataIdx = 0;

    std::vector<uint8_t> packageHeader(sizeof(pldm_package_header_information));
    package.seekg(0);
    package.read(reinterpret_cast<char*>(packageHeader.data()),
                 sizeof(pldm_package_header_information));
    auto pkgHeaderInfo = reinterpret_cast<pldm_package_header_information*>(
        packageHeader.data());
    auto originalPkgHeaderSize = pkgHeaderInfo->package_header_size;

    auto newDescriptorData = initNewDescriptorFor(slotNumber);
    packageHeader.resize(originalPkgHeaderSize + newDescriptorData.size());
    package.seekg(0);
    package.read(reinterpret_cast<char*>(packageHeader.data()),
                 originalPkgHeaderSize);

    // Assign the data again to prevent memory reallocation caused by resize
    pkgHeaderInfo = reinterpret_cast<pldm_package_header_information*>(
        packageHeader.data());
    pkgHeaderInfo->package_header_size += newDescriptorData.size();

    // Assume the DeviceIDRecordCount is 1 because it's for BIC update.
    processedDataIdx += sizeof(pldm_package_header_information) +
                        pkgHeaderInfo->package_version_string_length +
                        1 /*1 byte from DeviceIDRecordCount*/;

    auto recordInfo = reinterpret_cast<pldm_firmware_device_id_record*>(
        packageHeader.data() + processedDataIdx);

    processedDataIdx += sizeof(pldm_firmware_device_id_record) +
                        ceil(pkgHeaderInfo->component_bitmap_bit_length / 8) +
                        recordInfo->comp_image_set_version_string_length;
    processedDataIdx = skipOriginalDescriptors(recordInfo->descriptor_count,
                                               packageHeader, processedDataIdx);

    recordInfo->record_length += newDescriptorData.size();
    recordInfo->descriptor_count += 1;

    packageHeader.insert(packageHeader.begin() + processedDataIdx,
                         newDescriptorData.begin(), newDescriptorData.end());
    processedDataIdx += newDescriptorData.size();

    constexpr uint8_t COMPONENT_IMAGE_COUNT_LENGTH = 2;
    constexpr uint8_t COMPONENT_LOCATION_OFFSET = 12;
    packageHeader.at(processedDataIdx + COMPONENT_IMAGE_COUNT_LENGTH +
                     COMPONENT_LOCATION_OFFSET) += newDescriptorData.size();

    auto checkSum = crc32(packageHeader.data(),
                          pkgHeaderInfo->package_header_size - 4);
    packageHeader.at(pkgHeaderInfo->package_header_size - 4) = checkSum;
    packageHeader.at(pkgHeaderInfo->package_header_size - 3) = checkSum >> 8;
    packageHeader.at(pkgHeaderInfo->package_header_size - 2) = checkSum >> 16;
    packageHeader.at(pkgHeaderInfo->package_header_size - 1) = checkSum >> 24;
    newPackage.write(reinterpret_cast<const char*>(packageHeader.data()),
                     pkgHeaderInfo->package_header_size);

    package.seekg(originalPkgHeaderSize);
    // Write remaing package data
    auto packageSize = std::filesystem::file_size(inputPackagePath);
    package.seekg(originalPkgHeaderSize);
    std::vector<uint8_t> packageData(packageSize - originalPkgHeaderSize);
    package.read(reinterpret_cast<char*>(packageData.data()),
                 packageData.size());
    newPackage.write(reinterpret_cast<const char*>(packageData.data()),
                     packageData.size());

    package.close();
    newPackage.close();
}

void tryReWrapBICUpdatePackage(uint8_t slotNumber,
                               const std::string& inputPackagePath)
{
    try
    {
        outputBICPackageWithSlotNumberInHeader(slotNumber, inputPackagePath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to re-wrap the update package, error: " << e.what()
                  << std::endl;
        return;
    }
}

void outputCXLPackageWithSlotNumberInHeader(uint8_t slotNumber,
                                            uint8_t instanceNum,
                                            const std::string& inputPackagePath)
{
    const std::map<uint8_t, uint8_t> instanceToComponentId = {{1, 5}, {2, 6}};
    auto package = openInputPackage(inputPackagePath);
    auto newPackage = createOutputPackage(inputPackagePath);
    size_t processedDataIdx = 0;

    std::vector<uint8_t> packageHeader(sizeof(pldm_package_header_information));
    package.seekg(0);
    package.read(reinterpret_cast<char*>(packageHeader.data()),
                 sizeof(pldm_package_header_information));
    auto pkgHeaderInfo = reinterpret_cast<pldm_package_header_information*>(
        packageHeader.data());
    auto originalPkgHeaderSize = pkgHeaderInfo->package_header_size;

    // Load the whole package header and leave space for new descriptor
    auto newDescriptorData = initNewDescriptorFor(slotNumber);
    packageHeader.resize(originalPkgHeaderSize + newDescriptorData.size());
    package.seekg(0);
    package.read(reinterpret_cast<char*>(packageHeader.data()),
                 originalPkgHeaderSize);

    // Assign the data again to prevent memory reallocation caused by resize
    pkgHeaderInfo = reinterpret_cast<pldm_package_header_information*>(
        packageHeader.data());
    pkgHeaderInfo->package_header_size += newDescriptorData.size();

    // Assume the DeviceIDRecordCount is 1 because it's for CXL update.
    processedDataIdx += sizeof(pldm_package_header_information) +
                        pkgHeaderInfo->package_version_string_length +
                        1 /*1 byte from DeviceIDRecordCount*/;

    auto recordInfo = reinterpret_cast<pldm_firmware_device_id_record*>(
        packageHeader.data() + processedDataIdx);

    processedDataIdx += sizeof(pldm_firmware_device_id_record) +
                        ceil(pkgHeaderInfo->component_bitmap_bit_length / 8) +
                        recordInfo->comp_image_set_version_string_length;
    processedDataIdx = skipOriginalDescriptors(recordInfo->descriptor_count,
                                               packageHeader, processedDataIdx);

    recordInfo->record_length += newDescriptorData.size();
    recordInfo->descriptor_count += 1;

    packageHeader.insert(packageHeader.begin() + processedDataIdx,
                         newDescriptorData.begin(), newDescriptorData.end());
    processedDataIdx += newDescriptorData.size();

    constexpr uint8_t COMPONENT_IMAGE_COUNT_LENGTH = 2;
    constexpr uint8_t COMPONENT_ID_OFFSET = 2;
    constexpr uint8_t COMPONENT_LOCATION_OFFSET = 12;
    packageHeader.at(processedDataIdx + COMPONENT_IMAGE_COUNT_LENGTH +
                     COMPONENT_ID_OFFSET) =
        instanceToComponentId.at(instanceNum);
    packageHeader.at(processedDataIdx + COMPONENT_IMAGE_COUNT_LENGTH +
                     COMPONENT_LOCATION_OFFSET) += newDescriptorData.size();

    auto checkSum = crc32(packageHeader.data(),
                          pkgHeaderInfo->package_header_size - 4);
    packageHeader.at(pkgHeaderInfo->package_header_size - 4) = checkSum;
    packageHeader.at(pkgHeaderInfo->package_header_size - 3) = checkSum >> 8;
    packageHeader.at(pkgHeaderInfo->package_header_size - 2) = checkSum >> 16;
    packageHeader.at(pkgHeaderInfo->package_header_size - 1) = checkSum >> 24;
    newPackage.write(reinterpret_cast<const char*>(packageHeader.data()),
                     pkgHeaderInfo->package_header_size);

    package.seekg(originalPkgHeaderSize);
    // Write remaing package data
    auto packageSize = std::filesystem::file_size(inputPackagePath);
    package.seekg(originalPkgHeaderSize);
    std::vector<uint8_t> packageData(packageSize - originalPkgHeaderSize);
    package.read(reinterpret_cast<char*>(packageData.data()),
                 packageData.size());
    newPackage.write(reinterpret_cast<const char*>(packageData.data()),
                     packageData.size());

    package.close();
    newPackage.close();
    return;
}

void tryReWrapCXLUpdatePackage(uint8_t slotNumber, uint8_t instanceNum,
                               const std::string& inputPackagePath)
{
    try
    {
        outputCXLPackageWithSlotNumberInHeader(slotNumber, instanceNum,
                                               inputPackagePath);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to re-wrap the update package, error: " << e.what()
                  << std::endl;
        return;
    }
}

int main(int argc, char** argv)
{
    std::string imagePath{};
    uint8_t slotNumber, instanceNum;

    CLI::App app{
        "Re-wrap PLDM firmware update package, the output file will be ${input_file}_re_wrapped at the same path."};
    app.require_subcommand(1);

    // Init command for BIC
    auto reWrapBIC = app.add_subcommand(
        "bic", "Re-wrap the BIC firmware update package for a specific slot.");
    reWrapBIC->add_option("-f,--file", imagePath, "The path of BIC image file.")
        ->required()
        ->check(CLI::ExistingFile);

    reWrapBIC
        ->add_option("-s,--slot", slotNumber, "The number of slot to update.")
        ->required()
        ->check(CLI::Range(1, 8));

    reWrapBIC->callback(
        [&]() { tryReWrapBICUpdatePackage(slotNumber, imagePath); });

    // Init command for CXL
    auto reWrapCXL = app.add_subcommand(
        "cxl",
        "Re-wrap the CXL firmware update package for a specific instance.");

    reWrapCXL->add_option("-f,--file", imagePath, "The path of CXL image file.")
        ->required()
        ->check(CLI::ExistingFile);

    reWrapCXL
        ->add_option("-s,--slot", slotNumber, "The number of slot to update.")
        ->required()
        ->check(CLI::Range(1, 8));

    reWrapCXL
        ->add_option("-i,--instance", instanceNum,
                     "The number of instance to update.")
        ->required()
        ->check(CLI::Range(1, 2));

    reWrapCXL->callback([&]() {
        tryReWrapCXLUpdatePackage(slotNumber, instanceNum, imagePath);
    });

    CLI11_PARSE(app, argc, argv);

    return 0;
}

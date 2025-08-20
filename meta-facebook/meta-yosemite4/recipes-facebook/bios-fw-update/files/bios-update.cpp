#include "bios-update.hpp"
#include "bios-usb-update.hpp"
#include "config.hpp"
#include "pldmutils/package_parser.hpp"

#include <ctype.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>
#include <set>

#include <variant>
#include <vector>

constexpr auto HOST_STATE_SERVICE = "xyz.openbmc_project.State.Host";
constexpr auto HOST_STATE_OBJECT = "/xyz/openbmc_project/state/host";
constexpr auto HOST_STATE_INTERFACE = "xyz.openbmc_project.State.Host";

using PropertyValue = std::variant<std::string>;

enum class POWER : uint8_t
{
    OFF = 0x00,
    ON = 0x01,
};

uint8_t getSlotAddress(uint8_t slotId)
{
    // base 0
    return ((slotId - 1) << 2);
}

void cmd_begin_with_IANA(std::vector<uint8_t>& cmd)
{
    uint8_t iana[IANA_ID_SIZE];
    memcpy(iana, (uint8_t*)&IANA_ID, IANA_ID_SIZE);
    cmd.insert(cmd.begin(), iana, iana + IANA_ID_SIZE);
}

bool set_host_state(sdbusplus::bus_t& bus, POWER ctrl, uint8_t slotId)
{
    std::string service = std::string{HOST_STATE_SERVICE} +
                          std::to_string(slotId);
    std::string object = std::string{HOST_STATE_OBJECT} +
                         std::to_string(slotId);

    auto hostStateCall = bus.new_method_call(service.c_str(), object.c_str(),
                                             "org.freedesktop.DBus.Properties",
                                             "Set");

    PropertyValue value = (ctrl == POWER::ON)
                              ? "xyz.openbmc_project.State.Host.Transition.On"
                              : "xyz.openbmc_project.State.Host.Transition.Off";

    hostStateCall.append(HOST_STATE_INTERFACE, "RequestedHostTransition",
                         value);
    try
    {
        bus.call_noreply(hostStateCall);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "Failed to set property: " << e.what() << "\n";
        return false;
    }

    (ctrl == POWER::ON) ? sleep(1) : sleep(6); // wait until setting done.
    return true;
}

bool power_ctrl(sdbusplus::bus_t& bus, POWER ctrl, uint8_t slotId)
{
    set_host_state(bus, ctrl, slotId);

    // TODO: Check power status using PLDM tool
    return true;
}

bool BIOSupdater::run()
{
    // erase only
    if (imagePath.empty()) {
        int ret = update_bic_usb_bios(slotId, imagePath, cpuType);
        return (ret < 0) ? false : true;
    }

    // raw image validation
    std::ifstream imageFile(imagePath, std::ios::binary);
    if (!imageFile)
    {
        std::cerr << "Failed to open image file: " << imagePath << std::endl;
        return false;
    }
    imageFile.seekg(0x61000);
    auto pos = imageFile.tellg();
    if (pos != 0x61000)
    {
        std::cerr << "Invalid image file: " << imagePath << std::endl;
        return false;
    }
    uint32_t signature = 0;
    if (imageFile.read(reinterpret_cast<char*>(&signature), sizeof(signature)))
    {
        if (signature != 0x50535024)
        {
            std::cerr << "Invalid image file: " << imagePath << std::endl;
            return false;
        }
    }
    imageFile.close();
    int ret = 0;

    if (!power_ctrl(bus, POWER::OFF, slotId))
    {
        return false;
    }

    // wait usb hub processing the usb disconnection event
    sleep(3);

    ret = update_bic_usb_bios(slotId, imagePath, cpuType);

    if (!power_ctrl(bus, POWER::ON, slotId))
    {
        return false;
    }

    return (ret < 0) ? false : true;
}

pldm::fw_update::Descriptors createDescriptors(const std::string& cpuType) {
    using namespace pldm::fw_update;

    Descriptors descriptors{};
    uint32_t ianaNumber = htobe32(IANA_NUMBER);

    descriptors.emplace(
        PLDM_FWUP_IANA_ENTERPRISE_ID,
        DescriptorData{
            reinterpret_cast<const uint8_t*>(&ianaNumber),
            reinterpret_cast<const uint8_t*>(&ianaNumber) +
                sizeof(ianaNumber)});

    std::string compatibleHardware;

    compatibleHardware.append("com.")
        .append(VENDOR_NAME)
        .append(".Hardware.")
        .append(PLATFORM_NAME)
        .append(".")
        .append(BOARD_NAME)
        .append(".BIOS");
    auto compatibleHardwareBergamo = compatibleHardware + ".AMD_BERGAMO";
    auto compatibleHardwareTurin = compatibleHardware + ".AMD_TURIN";
    auto compatibleHardwareTurines = compatibleHardware + ".AMD_TURINES";

    if (cpuType == "BERGAMO" || cpuType == "ALL")
    {
        descriptors.emplace(
            PLDM_FWUP_VENDOR_DEFINED,
            std::make_tuple(
                "OpenBMC.CompatibleHardware",
                VendorDefinedDescriptorData{
                    reinterpret_cast<const uint8_t*>(compatibleHardwareBergamo.c_str()),
                    reinterpret_cast<const uint8_t*>(compatibleHardwareBergamo.c_str()) +
                    compatibleHardwareBergamo.size()}));
    }
    if (cpuType == "TURIN" || cpuType == "ALL")
    {
        descriptors.emplace(
            PLDM_FWUP_VENDOR_DEFINED,
            std::make_tuple(
                "OpenBMC.CompatibleHardware",
                VendorDefinedDescriptorData{
                    reinterpret_cast<const uint8_t*>(compatibleHardwareTurin.c_str()),
                    reinterpret_cast<const uint8_t*>(compatibleHardwareTurin.c_str()) +
                    compatibleHardwareTurin.size()}));
    }
    if (cpuType == "TURINES" || cpuType == "ALL")
    {
        descriptors.emplace(
            PLDM_FWUP_VENDOR_DEFINED,
            std::make_tuple(
                "OpenBMC.CompatibleHardware",
                VendorDefinedDescriptorData{
                    reinterpret_cast<const uint8_t*>(compatibleHardwareTurines.c_str()),
                    reinterpret_cast<const uint8_t*>(compatibleHardwareTurines.c_str()) +
                    compatibleHardwareTurines.size()}));
    }

    return descriptors;
}

int main(int argc, char** argv)
{
    auto bus = sdbusplus::bus::new_default();
    std::string imagePath{};
    uint8_t slotId;
    std::string cpuType = "ALL";
    bool eraseFlag = false;

    CLI::App app{"Update or erase the firmware BIOS via USB to BIC"};

    app.add_option("-f,--file", imagePath, "The path of bios image file.")
        ->check(CLI::ExistingFile);

    app.add_option("-s,--slot", slotId, "The number of slot to update.")
        ->required();

    app.add_option("-c, --cpu", cpuType,
                   "BERGAMO or TURIN or TURINES. Update both blocks if it is not set.");

    app.add_flag("--erase", eraseFlag, "Erase BIOS flash only (do not update)");

    CLI11_PARSE(app, argc, argv);

    if (cpuType != "BERGAMO" && cpuType != "TURIN" && cpuType != "TURINES" && cpuType != "ALL")
    {
        std::cerr << "Wrong option: only support BERGAMO or TURIN or TURINES cpu\n";
        return 0;
    }

    // Check that update mode requires imagePath
    if (!eraseFlag && imagePath.empty()) {
        std::cerr << "Error: BIOS image file is required unless --erase is specified.\n";
        return -1;
    }
    
    if (eraseFlag) {
        std::cout << "[ERASE] Start erasing BIOS flash ...\n";
        int ret = update_bic_usb_bios(slotId, imagePath, cpuType, /*eraseOnly=*/true);
        if (ret == 0)
            std::cout << "[ERASE] BIOS erase: success\n";
        else
            std::cout << "[ERASE] BIOS erase: fail\n";
        return ret;
    }
    
    std::ifstream package(imagePath, std::ios::binary);
    if (!package) {
        std::cerr << "Failed to open package file: " << imagePath << std::endl;
        return -1;
    }

    constexpr char expected_sign[16] = {
        0xF0,
        0x18,
        0x87,
        0x8C,
        0xCB,
        0x7D,
        0x49,
        0x43,
        0x98,
        0x00,
        0xA0,
        0x2F,
        0x05,
        0x9A,
        0xCA,
        0x02};
    char sign[16] = {0};
    package.read(sign, sizeof(sign));
    if (package.gcount() == sizeof(sign) &&
        memcmp(sign, expected_sign, sizeof(expected_sign)) == 0) {
        pldm::fw_update::Descriptors descriptors = createDescriptors(cpuType);
        package.seekg(0, std::ios::end);
        size_t packageSize = package.tellg();
        package.seekg(0);
        std::vector<uint8_t> packageHeader(
            sizeof(pldm_package_header_information));
        package.read(
            reinterpret_cast<char*>(packageHeader.data()),
            sizeof(pldm_package_header_information));

        auto pkgHeaderInfo =
            reinterpret_cast<const pldm_package_header_information*>(
                packageHeader.data());
        auto pkgHeaderInfoSize = sizeof(pldm_package_header_information) +
            pkgHeaderInfo->package_version_string_length;
        packageHeader.clear();
        packageHeader.resize(pkgHeaderInfoSize);
        package.seekg(0);
        package.read(
            reinterpret_cast<char*>(packageHeader.data()), pkgHeaderInfoSize);

        auto parser = pldm::fw_update::parsePkgHeader(packageHeader);
        if (parser == nullptr) {
          std::cerr << "Failed to parse package header information"
                    << std::endl;
          return -1;
        }

        package.seekg(0);
        packageHeader.resize(parser->pkgHeaderSize);
        package.read(
            reinterpret_cast<char*>(packageHeader.data()),
            parser->pkgHeaderSize);
        try {
          parser->parse(packageHeader, packageSize);
        } catch (const std::exception& e) {
          std::cerr << "Failed to parse package header, error: " << e.what()
                    << std::endl;
          return -1;
        }

        auto packageDescriptors = std::get<pldm::fw_update::Descriptors>(
            parser->getFwDeviceIDRecords()[0]);

        std::vector<pldm::fw_update::Descriptor> descriptorVec(
            descriptors.begin(), descriptors.end());
        std::vector<pldm::fw_update::Descriptor> packageDescriptorVec(
            packageDescriptors.begin(), packageDescriptors.end());
        std::sort(descriptorVec.begin(), descriptorVec.end());
        std::sort(packageDescriptorVec.begin(), packageDescriptorVec.end());

        if (!std::includes(
                descriptorVec.begin(),
                descriptorVec.end(),
                packageDescriptorVec.begin(),
                packageDescriptorVec.end())) {
          std::cerr << "Package descriptors do not match with the "
                       "provided descriptors"
                    << std::endl;
          return -1;
        }
        auto unpackImagePath = imagePath + ".unpacked";
        std::ofstream unpackImage(unpackImagePath, std::ios::binary);
        if (!unpackImage) {
          std::cerr << "Failed to open unpacked image file: " << unpackImagePath
                    << std::endl;
          return -1;
        }
        package.seekg(parser->pkgHeaderSize);
        std::vector<uint8_t> imageData(packageSize - parser->pkgHeaderSize);
        package.read(
            reinterpret_cast<char*>(imageData.data()),
            packageSize - parser->pkgHeaderSize);
        unpackImage.write(
            reinterpret_cast<char*>(imageData.data()), imageData.size());
        unpackImage.close();
        imagePath = unpackImagePath;
        package.close();
        std::cout << "Unpacked image file: " << imagePath << std::endl;
    }

    const auto lockFile = std::string{} + "/tmp/bios-fw-handler_" + std::to_string(slotId) + ".lock";
    std::cout << "Lock file: " << lockFile << std::endl;
    int fd;
    int retryCount = 6; // Number of retries before giving up
    do
    {
        fd = open(lockFile.c_str(), O_RDWR | O_CREAT, 0666);
        if (fd < 0)
        {
            std::cerr << "Unable to open lock file: " << lockFile << std::endl;
            return 1;
        }
        if (flock(fd, LOCK_EX | LOCK_NB) == 0)
        {
            break; // Successfully acquired the lock
        }
        std::cerr << "Another instance is running, waiting for lock..." << std::endl;
        std::cerr << "Retry time remaining: " << retryCount << std::endl;
        sleep(10); // Wait before retrying
    } while (--retryCount > 0);
    if (retryCount <= 0)
    {
        std::cerr << 
            "Failed to acquire lock after multiple attempts, retry time exhausted" << 
            std::endl;
        close(fd);
        return 1;
    }

    auto bios = BIOSupdater(bus, imagePath, slotId, cpuType);
    if (bios.run())
    {
        std::cerr << "BIOS update: success\n";
    }
    else
    {
        std::cerr << "BIOS update: fail\n";
    }

    std::cout << "Release lock file: " << lockFile << std::endl;
    // Release the lock
    flock(fd, LOCK_UN);
    close(fd);

    return 0;
}

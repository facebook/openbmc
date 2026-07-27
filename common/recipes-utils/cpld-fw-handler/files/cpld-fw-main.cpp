#include "cpld-lattice.hpp"

#include <getopt.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <sys/file.h>
#include <fcntl.h>
#include <cstdio>
#include <string>
#ifdef ENABLE_EVENTS
#include <phosphor-logging/lg2.hpp>
#include <phosphor-logging/commit.hpp>
#include <xyz/openbmc_project/Software/Update/event.hpp>
#endif

#include <chrono>

#ifdef ENABLE_EVENTS
using TargetDetermined = sdbusplus::event::xyz::openbmc_project::software::Update::TargetDetermined;
using UpdateSuccessful = sdbusplus::event::xyz::openbmc_project::software::Update::UpdateSuccessful;
using ApplyFailed = sdbusplus::error::xyz::openbmc_project::software::Update::ActivateFailed;
using VerificationFailed = sdbusplus::error::xyz::openbmc_project::software::Update::VerificationFailed;
#endif

enum class SEL_TYPE
{
    TargetDetermined,
    ApplyFailed,
    UpdateSuccessful,
    VerificationFailed,
};

enum class CpldType
{
    SD,
    MGM,
    SPD,
    OTHER,
};

inline std::string to_string(CpldType type)
{
    switch (type) {
        case CpldType::SD:  return "SD CPLD";
        case CpldType::MGM: return "MGM CPLD";
        case CpldType::SPD: return "SPD CPLD";
        default:            return "CPLD";
    }
}

#ifdef ENABLE_EVENTS
static bool isAllowSel(const std::string& imagePath, const std::string& chip)
{
    return !imagePath.empty() && !chip.empty();
}

void addSelBySelType(SEL_TYPE selType,
                     const std::string& imagePath,
                     const std::string& chip,
                     const std::string& deviceType,
                     const std::string& info)
{
    if (!isAllowSel(imagePath, chip))
    {
        return;
    }

    auto targetName = sdbusplus::object_path(deviceType + " " + info);

    switch (selType)
    {
        case SEL_TYPE::TargetDetermined:
        {
            lg2::commit(
                TargetDetermined(
                    "TARGET_NAME", targetName,
                    "IMAGE_IDENTIFIER", imagePath
                )
            );
            break;
        }
        case SEL_TYPE::ApplyFailed:
        {
            lg2::commit(
                ApplyFailed(
                    "IMAGE_IDENTIFIER", imagePath,
                    "TARGET_NAME", targetName
                )
            );
            break;
        }
        case SEL_TYPE::UpdateSuccessful:
        {
            lg2::commit(
                UpdateSuccessful(
                    "TARGET_NAME", targetName,
                    "IMAGE_IDENTIFIER", imagePath
                )
            );
            break;
        }
        case SEL_TYPE::VerificationFailed:
        {
            lg2::commit(
                VerificationFailed(
                    "IMAGE_IDENTIFIER", imagePath,
                    "TARGET_NAME", targetName
                )
            );
            break;
        }
        default:
        {
            break;
        }
    }
}
#else
inline void addSelBySelType(
    SEL_TYPE /*selType*/, const std::string& /*imagePath*/,
    const std::string& /*chip*/, const std::string& /*deviceType*/,
    const std::string& /*info*/)
{}
#endif

static std::string getCpldType(std::string_view cpldName)
{
    if (cpldName == "LCMXO3LF-6900") return to_string(CpldType::SD);
    else if (cpldName == "LCMXO3D-4300") return to_string(CpldType::MGM);
    else if (cpldName == "LCMXO3D-9400") return to_string(CpldType::SPD);

    return to_string(CpldType::OTHER);
}

static std::string getInfo(std::string_view cpldType, uint8_t bus)
{
    if (cpldType == to_string(CpldType::SD))
    {
        return "slot" + std::to_string(bus + 1); // shift one to match the slot number
    }

    return "bus" + std::to_string(bus);
}

int main(int argc, char** argv)
{
    std::string imagePath{};
    uint8_t bus = 0, addr = 0;
    std::string chip, interface, target;
    bool debugMode{false};
    bool legacyMode{false};
    bool verifyOnly{false};

    CLI::App app{"CPLD update tool"};

    auto update = app.add_subcommand("update", "CPLD FW TOOL VER 1.0");

    update->add_option("-p,--path", imagePath, "image file path.")
        ->required()
        ->check(CLI::ExistingFile);
    update->add_option("-b,--bus", bus, "i2c bus")->required();
    update->add_option("-a,--addr", addr, "slave address")->required();
    update->add_option("-i,--interface", interface, "i2c")->required();

    update
        ->add_option("-c,--chip", chip,
                     "LCMXO3LF-4300|LCMXO3LF-6900|LCMXO3D-4300|LCMXO3D-9400|LFMXO5-25|LFMXO5-65T")
        ->required();
    update->add_option("-t,--target", target,
                       "used for LCMXO3D|LFMXO5 series CPLD, CFG0|CFG1, SRAM (LFMXO5 only)");

    update->add_flag("-v,--verbose", debugMode, "debug mode");
    update->add_flag("-l,--legacy", legacyMode,
                     "Use legacy update protocol for LFMXO5-25");
    update->add_flag("--verify-only", verifyOnly, "verify only");

    auto version = app.add_subcommand("version", "Get Frimware Version");

    version->add_option("-i,--interface", interface, "interface")->required();
    version->add_option("-b,--bus", bus, "i2c bus")->required();
    version->add_option("-a,--addr", addr, "slave address")->required();
    version->add_option("-t,--target", target,
                        "used for LCMXO3D|LFMXO5 series CPLD, CFG0|CFG1");
    version->add_option(
        "-c,--chip", chip,
        "LCMXO3LF-4300|LCMXO3LF-6900|LCMXO3D-4300|LCMXO3D-9400|LFMXO5-25|LFMXO5-65T");

    CLI11_PARSE(app, argc, argv);

    auto cpldManager = CpldLatticeManager(bus, addr, imagePath, chip, interface,
                                          target, debugMode);

    if (chip.empty())
    {
        cpldManager.autoDetectChip();
        chip = cpldManager.getChipName();
    }

    auto cpldType = getCpldType(chip);
    auto info = getInfo(cpldType, bus);

    addSelBySelType(SEL_TYPE::TargetDetermined, imagePath, chip, cpldType, info);

    if (version->parsed())
    {
        if (cpldManager.getVersion() < 0)
        {
            std::cerr << "Failed to get CPLD version" << std::endl;
            addSelBySelType(SEL_TYPE::ApplyFailed, imagePath, chip, cpldType, info);
            return -1;
        }
    }
    else if (update->parsed())
    {
        // Prevent multiple instances using flock

        const auto lockFile = std::string{} + "/tmp/cpld-fw-handler_" + std::to_string(bus) + ".lock";
        std::cout << "Lock file: " << lockFile << std::endl;
        int fd;
        int retryCount = 6; // Number of retries before giving up
        do
        {
            fd = open(lockFile.c_str(), O_RDWR | O_CREAT, 0666);
            if (fd < 0)
            {
                std::cerr << "Unable to open lock file: " << lockFile << std::endl;
                addSelBySelType(SEL_TYPE::ApplyFailed, imagePath, chip, cpldType, info);
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
            addSelBySelType(SEL_TYPE::ApplyFailed, imagePath, chip, cpldType, info);
            close(fd);
            return 1;
        }

        using namespace std::chrono;

        auto start = high_resolution_clock::now();
        if (verifyOnly)
        {
            if  (cpldManager.fwVerifyOnly(legacyMode) < 0)
            {
                std::cerr << "CPLD verify failed" << std::endl;
                addSelBySelType(SEL_TYPE::VerificationFailed, imagePath, chip, cpldType, info);
                return -1;
            }
        }
        else
        {
            if (cpldManager.fwUpdate(legacyMode) < 0)
            {
                std::cerr << "CPLD update failed" << std::endl;
                addSelBySelType(SEL_TYPE::ApplyFailed, imagePath, chip, cpldType, info);
                return -1;
            }
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);

        std::cout << "Execution time: " << std::dec << duration.count()
                  << " seconds" << std::endl;
        addSelBySelType(SEL_TYPE::UpdateSuccessful, imagePath, chip, cpldType, info);

        std::cout << "Release lock file: " << lockFile << std::endl;
        // Release the lock
        flock(fd, LOCK_UN);
        close(fd);
    }


    return 0;
}

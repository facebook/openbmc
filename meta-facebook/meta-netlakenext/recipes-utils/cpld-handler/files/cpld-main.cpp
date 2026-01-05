#include "cpld-lattice.hpp"

#include <getopt.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <sys/file.h>
#include <fcntl.h>
#include <cstdio>

#include <chrono>

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
                     "LCMXO3LF-4300|LCMXO3LF-6900|LCMXO3D-4300|LCMXO3D-9400|LFMXO5-25")
        ->required();
    update->add_option("-t,--target", target,
                       "used for LCMXO3D series CPLD, CFG0|CFG1");

    update->add_flag("-v,--verbose", debugMode, "debug mode");
    update->add_flag("-l,--legacy", legacyMode,
                     "Use legacy update protocol for LFMXO5-25");
    update->add_flag("--verify-only", verifyOnly, "verify only");

    auto version = app.add_subcommand("version", "Get Frimware Version");

    version->add_option("-i,--interface", interface, "interface")->required();
    version->add_option("-b,--bus", bus, "i2c bus")->required();
    version->add_option("-a,--addr", addr, "slave address")->required();
    version->add_option("-t,--target", target,
                        "used for LCMXO3D series CPLD, CFG0|CFG1");
    version->add_option(
        "-c,--chip", chip,
        "LCMXO3LF-4300|LCMXO3LF-6900|LCMXO3D-4300|LCMXO3D-9400|LFMXO5-25");

    CLI11_PARSE(app, argc, argv);

    auto cpldManager = CpldLatticeManager(bus, addr, imagePath, chip, interface,
                                          target, debugMode);

    if (version->parsed())
    {
        if (cpldManager.getVersion() < 0)
        {
            std::cerr << "Failed to get CPLD version" << std::endl;
            return -1;
        }
    }
    else if (update->parsed())
    {
        // Prevent multiple instances using flock

        const auto lockFile = std::string{} + "/tmp/cpld-handler_" + std::to_string(bus) + ".lock";
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

        using namespace std::chrono;

        auto start = high_resolution_clock::now();
        if (verifyOnly)
        {
            if  (cpldManager.fwVerifyOnly(legacyMode) < 0)
            {
                std::cerr << "CPLD verify failed" << std::endl;
                return -1;
            }
        }
        else
        {
            if (cpldManager.fwUpdate(legacyMode) < 0)
            {
                std::cerr << "CPLD update failed" << std::endl;
                return -1;
            }
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);

        std::cout << "Execution time: " << duration.count() << " seconds"
                  << std::endl;

        std::cout << "Release lock file: " << lockFile << std::endl;
        // Release the lock
        flock(fd, LOCK_UN);
        close(fd);
    }


    return 0;
}

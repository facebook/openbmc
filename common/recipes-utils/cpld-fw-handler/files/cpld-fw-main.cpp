#include "cpld-lattice.hpp"

#include <getopt.h>
#include <unistd.h>

#include <CLI/CLI.hpp>

#include <chrono>

int main(int argc, char** argv)
{
    std::string imagePath{};
    uint8_t bus = 0, addr = 0;
    std::string chip, interface;
    bool debugMode{false};

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
                     "LCMXO3LF-4300|LCMXO3LF-6900|LCMXO3D-4300|LCMXO3D-9400")
        ->required();

    update->add_flag("-v,--verbose", debugMode, "debug mode");

    auto version = app.add_subcommand("version", "Get Frimware Version");

    version->add_option("-i,--interface", interface, "interface")->required();
    version->add_option("-b,--bus", bus, "i2c bus")->required();
    version->add_option("-a,--addr", addr, "slave address")->required();

    CLI11_PARSE(app, argc, argv);

    auto cpldManager = CpldLatticeManager(bus, addr, imagePath, chip, interface,
                                          debugMode);

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
        using namespace std::chrono;

        auto start = high_resolution_clock::now();
        if (cpldManager.fwUpdate() < 0)
        {
            std::cerr << "CPLD update failed" << std::endl;
            return -1;
        }

        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<seconds>(stop - start);

        std::cout << "Execution time: " << duration.count() << " seconds"
                  << std::endl;
    }

    return 0;
}

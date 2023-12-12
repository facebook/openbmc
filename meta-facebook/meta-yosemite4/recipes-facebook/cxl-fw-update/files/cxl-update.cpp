#include "cxl-update.hpp"

#include "cci-mctp-update.hpp"

#include <ctype.h>
#include <unistd.h>

#include <CLI/CLI.hpp>
#include <sdbusplus/bus.hpp>

#include <variant>
#include <vector>

bool CCIupdater::updateFw()
{
    throw std::runtime_error("This function is not yet implemented.");
    return true;
}

bool CCIupdater::getFwVersion()
{
    if (get_fw_ver_cci_over_mctp(eid) < 0)
    {
        throw std::runtime_error("Get CXL firmware version over MCTP failed.");
    }

    return true;
}

int main(int argc, char** argv)
{
    std::filesystem::path imagePath;
    uint8_t eid;

    CLI::App app{"CCI Firmware Update tool VER 1.0"};

    auto update = app.add_subcommand(
        "update", "Update the CXL device firmware using CCI over MCTP");
    update->add_option("-f,--file", imagePath, "The path of CXL image file.")
        ->required()
        ->check(CLI::ExistingFile);

    update
        ->add_option("-m,--mctp_eid", eid, "The number of MCTP EID to update.")
        ->required();

    auto version = app.add_subcommand("version", "Get Firmware Version");
    version
        ->add_option("-m,--mctp_eid", eid, "The number of MCTP EID to update.")
        ->required();

    CLI11_PARSE(app, argc, argv);

    auto cci = CCIupdater(imagePath, eid);

    if (version->parsed())
    {
        try
        {
            cci.getFwVersion();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to get CXL firmware version of EID: " << +eid
                      << std::endl;
            return -1;
        }
    }
    else if (update->parsed())
    {
        try
        {
            cci.updateFw();
        }
        catch (const std::exception& e)
        {
            std::cerr << "Exception caught: " << e.what() << std::endl;
            return -1;
        }
    }

    return 0;
}

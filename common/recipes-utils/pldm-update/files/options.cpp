#include "pldm-update.hpp"
#include "mmc-recovery.hpp"

#include <CLI/CLI.hpp>

int main(int argc, char** argv)
{
    CLI::App app{"PLDM update tool"};
    app.require_subcommand(1);

    std::string file;
    bool recover_mmc_mode = false;

    auto update_ag = app.add_subcommand("ag", "Update Aegis image.");
    update_ag->add_option("<FILE>", file, "The file to update")->required();
    update_ag->add_flag("--recover-mmc", recover_mmc_mode, "Recover the MMC");
    update_ag->add_option("-n,--number", board_num, "board number")
        ->default_val("none")
        ->capture_default_str();
    update_ag->add_option("-b,--board", board, "Board type")
        ->default_val("none")
        ->capture_default_str();
    update_ag->callback([&]() {
        if (recover_mmc_mode)
        {
            auto ops = create_recovery_ops();
            recover_mmc(ops.get(), file, board_num, board);
        }
        else
        {
            pldm_update(file);
        }
    });

    CLI11_PARSE(app, argc, argv);
    return 0;
}

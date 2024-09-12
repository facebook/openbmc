#include "pldm-update.hpp"

void PldmUpdateApp::add_options()
{
    // init subcommands
    auto file = std::string();
    auto description = std::string("pldm-update ag <image>");
    auto opt_update_ag = app.add_subcommand("ag", "Update AG image.");
    opt_update_ag->add_option("file", file, description)->required();
    opt_update_ag->callback([&]() {pldm_update(file);});
}
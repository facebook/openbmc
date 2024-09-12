#pragma once

#include "CLI/CLI.hpp"
#include <string>



class PldmUpdateApp {
public:
    std::string pldmdBusName = "";
    PldmUpdateApp(const std::string& description) : app(description) {
        app.failure_message(CLI::FailureMessage::help);
        app.fallthrough();
    }

    // Add options to the app in option.cpp
    void add_options();

    void pldm_update(const std::string& file);

    // Run the app
    int run(int argc, char** argv) {
        try {
            app.parse(argc, argv);
        } catch(const CLI::ParseError &e) {
            return app.exit(e);
        }
        return 0;
    }

private:
    CLI::App app;

};
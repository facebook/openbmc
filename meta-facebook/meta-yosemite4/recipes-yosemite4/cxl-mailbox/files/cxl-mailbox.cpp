#include "cxl-mailbox.hpp"
#include "libcxl.hpp"

#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <signal.h>
#include <unistd.h>

bool g_debug_mode = false;

static std::string g_current_lock_file;
static int g_current_lock_fd = -1;

void signal_handler(int sig) {
    DEBUG_PRINT("Signal %d received, cleaning up", sig);
    
    if (g_current_lock_fd >= 0) {
        close(g_current_lock_fd);
        g_current_lock_fd = -1;
    }
    
    if (!g_current_lock_file.empty()) {
        unlink(g_current_lock_file.c_str());
        g_current_lock_file.clear();
    }
    
    exit(sig);
}

void CxlMctpCciTool::initializeCommands()
{
    commands_ = {
        {"get-fw-info", "Get firmware information", get_fw_info_wrapper, false},
        {"get-event-records", "Get event records (usage: get-event-records [--log-type <0-3>])", get_event_records_wrapper, false},
        {"get-supported-logs", "Get supported logs", get_supported_logs_wrapper, false},
        {"get-log", "Get specific log (usage: get-log --log-uuid <uuid> --log-size <size>)", get_log_wrapper, true},
        {"dimm-spd-read", "Read DIMM SPD data (usage: dimm-spd-read --spd-id <id> --offset <offset> --num-bytes <bytes>)", dimm_spd_read_wrapper, true},
        {"dimm-slot-info", "Get DIMM slot information", dimm_slot_info_wrapper, false},
        {"health-counters-get", "Get health counters", get_health_counters_wrapper, false},
        {"get-cxl-membridge-stats", "Get CXL memory bridge statistics", get_cxl_membridge_stats_wrapper, false}
    };
    
    for (size_t i = 0; i < commands_.size(); ++i) {
        command_map_[commands_[i].name] = i;
    }
}

void CxlMctpCciTool::showVersion()
{
    std::cout << "CXL MCTP CCI Tool Version 0.1" << std::endl;
}

void CxlMctpCciTool::showHelp()
{
    std::cout << "\n usage: " << CxlMctpCci::USAGE_STRING << "\n\n";
    
    std::cout << " Options:\n";
    std::cout << "   " << std::left << std::setw(35) << "--version, -v" << "Show version information" << std::endl;
    std::cout << "   " << std::left << std::setw(35) << "--help, -h" << "Show this help message" << std::endl;
    std::cout << "   " << std::left << std::setw(35) << "--list-cmds" << "List all available commands" << std::endl;
    std::cout << "   " << std::left << std::setw(35) << "--debug, -d" << "Enable debug mode" << std::endl;
    std::cout << "   " << std::left << std::setw(35) << "--mctp-eid, -m" << "Specify MCTP EID" << std::endl;
    
    std::cout << "\n CCI commands (require --mctp-eid):\n";
    for (const auto& cmd : commands_) {
        std::cout << "   " << std::left << std::setw(35) << cmd.name 
                  << cmd.description << std::endl;
    }
    
    std::cout << "\n " << CxlMctpCci::MORE_INFO_STRING << "\n\n";
}

void CxlMctpCciTool::listCommands()
{
    std::cout << "Available commands:" << std::endl;
    
    for (const auto& cmd : commands_) {
        std::cout << "  " << cmd.name << std::endl;
    }
}

const CommandInfo* CxlMctpCciTool::getCommandInfo(const std::string& command)
{
    auto it = command_map_.find(command);
    return (it != command_map_.end()) ? &commands_[it->second] : nullptr;
}

bool CxlMctpCciTool::executeCciCommand(const std::string& command, const std::vector<std::string>& params)
{
    const CommandInfo* cmd_info = getCommandInfo(command);
    if (!cmd_info) {
        return false;
    }
    
    if (cmd_info->needs_params && params.empty()) {
        throw std::runtime_error("Command '" + command + "' requires parameters");
    }
    
    int result = cmd_info->function(eid_, params);
    if (result < 0) {
        throw std::runtime_error(cmd_info->description + " failed");
    }
    
    return true;
}

int main(int argc, char** argv)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        uint8_t eid = 0;
        std::string command;
        std::vector<std::string> all_args(argv + 1, argv + argc);
        std::vector<std::string> command_args;
        bool eid_provided = false;
        bool debug_flag = false;
        
        for (size_t i = 0; i < all_args.size(); ++i) {
            if (all_args[i] == "-m" || all_args[i] == "--mctp-eid") {
                if (i + 1 < all_args.size()) {
                    eid = static_cast<uint8_t>(std::stoul(all_args[i + 1], nullptr, 0));
                    eid_provided = true;
                    ++i;
                }
            } else if (all_args[i] == "-d" || all_args[i] == "--debug") {
                debug_flag = true;
            } else if (all_args[i] == "-h" || all_args[i] == "--help") {
                CxlMctpCciTool tool;
                tool.showHelp();
                return 0;
            } else if (all_args[i] == "-v" || all_args[i] == "--version") {
                CxlMctpCciTool tool;
                tool.showVersion();
                return 0;
            } else if (all_args[i] == "--list-cmds") {
                CxlMctpCciTool tool;
                tool.listCommands();
                return 0;
            } else if (command.empty()) {
                command = all_args[i];
            } else {
                command_args.assign(all_args.begin() + i, all_args.end());
                break;
            }
        }
        
        g_debug_mode = debug_flag;
        
        if (command.empty()) {
            CxlMctpCciTool tool;
            tool.showHelp();
            return 0;
        }

        if (!eid_provided) {
            std::cerr << "Error: Command '" << command << "' requires --mctp-eid parameter" << std::endl;
            std::cerr << "Usage: cxl-mailbox --mctp-eid EID " << command << " [args...]" << std::endl;
            return 1;
        }
        
        CxlMctpCciTool tool(eid);
        
        std::cout << "Executing '" << command << "' for EID: " << +eid << std::endl;
        
        if (!tool.executeCciCommand(command, command_args)) {
            std::cerr << "Unknown command: '" << command << "'" << std::endl;
            std::cerr << "Use '--list-cmds' to see available commands" << std::endl;
            return 1;
        }
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}

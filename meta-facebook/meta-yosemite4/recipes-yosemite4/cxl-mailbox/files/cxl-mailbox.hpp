#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdio>

namespace CxlMctpCci {
    constexpr const char* USAGE_STRING = 
        "cxl-mailbox [--mctp-eid EID] [--debug] [--version] [--help] [--list-cmds] COMMAND [OPTIONS]";
    constexpr const char* MORE_INFO_STRING = 
        "Use cxl-mailbox --list-cmds to see all available commands";
}

struct CommandInfo {
    std::string name;
    std::string description;
    std::function<int(uint8_t, const std::vector<std::string>&)> function;
    bool needs_params;
};

class CxlMctpCciTool {
public:
    CxlMctpCciTool(uint8_t eid) : eid_(eid) { initializeCommands(); }
    CxlMctpCciTool() : eid_(0) { initializeCommands(); }
    ~CxlMctpCciTool() = default;
    
    CxlMctpCciTool(const CxlMctpCciTool&) = delete;
    CxlMctpCciTool& operator=(const CxlMctpCciTool&) = delete;
    
    void showVersion();
    void showHelp();
    void listCommands();
    bool executeCciCommand(const std::string& command, const std::vector<std::string>& params = {});
    const CommandInfo* getCommandInfo(const std::string& command);

private:
    uint8_t eid_;
    std::vector<CommandInfo> commands_;
    std::unordered_map<std::string, size_t> command_map_;
    
    void initializeCommands();
};

extern bool g_debug_mode;

#define DEBUG_PRINT(...) \
    do { \
        if (g_debug_mode) { \
            printf("[DEBUG] %s:%d: ", __func__, __LINE__); \
            printf(__VA_ARGS__); \
            printf("\n"); \
            fflush(stdout); \
        } \
    } while(0)

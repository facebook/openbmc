#include "mmc-recovery.hpp"

struct RBInfo {
    int slot;
    const char* uart_rb;
    const char* uart_sit;
};

constexpr std::array<RBInfo, 4> rb_map = {{
    {6,  "/dev/ttyUSBdevice56", "/dev/ttyUSBdevice1"},
    {10,  "/dev/ttyUSBdevice58", "/dev/ttyUSBdevice29"},
    {8, "/dev/ttyUSBdevice57", "/dev/ttyUSBdevice15"},
    {13, "/dev/ttyUSBdevice59", "/dev/ttyUSBdevice43"},
}};

class SanatabarbaraRecoveryOps : public RecoveryOps {
  public:
    bool set_recovery_mode(bool enable, int bus) override { 
        // 0x00 -> RSVD_GPIO_1 pull low
        // 0xff -> RSVD_GPIO_1 pull high
        std::string cmd;

        cmd += "i2cset -y " + std::to_string(bus) + " 0x21 2 " + (enable ? "0x00" : "0xff");
        cmd += " && sleep 1";

        cmd += " && i2cset -y " + std::to_string(bus) + " 0x21 1 0";
        cmd += " && sleep 5";

        cmd += " && i2cset -y " + std::to_string(bus) + " 0x21 1 1";
        cmd += " && sleep 3";


        std::cout << "Attempting to " << (enable ? "enable" : "disable") 
                << " recovery mode..." << std::endl;

        auto rc = std::system(cmd.c_str());
        if (rc)
        {
            std::cerr << "Failed to set recovery mode to " << enable 
                    << ", rc=" << rc << std::endl;
            return false;
        }
        return true;
    }

    SerialSel resolve_serial(const std::string& rb_num, const std::string& board) override {
        int index = std::stoi(rb_num);
        if (index < 0 || index >= static_cast<int>(rb_map.size()))
        {
            std::cerr << "Invalid rb_num: " << rb_num << std::endl;
            return SerialSel{};
        }

        int slot = rb_map[index].slot;
        const char* uartPort = (board == "sit") ? rb_map[index].uart_sit : rb_map[index].uart_rb;

        std::cout << "[INFO] Slot: " << slot << ", UART: " << uartPort << std::endl;
        return SerialSel{uartPort, slot};
    }
};

std::unique_ptr<RecoveryOps> create_recovery_ops() {
    return std::make_unique<SanatabarbaraRecoveryOps>();
}

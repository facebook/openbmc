#include "mmc-recovery.hpp"
#include "pldm-update.hpp"

#include <CLI/CLI.hpp>
#include <fstream>
#include <vector>

RecoveryOps::~RecoveryOps() {}

namespace fs = std::filesystem;

#pragma pack(push, 1)
typedef struct
{
    uint32_t hdr;
    uint16_t pkg_len;
    uint8_t cmd;
    uint32_t addr;
} uart_cmd_t;

typedef struct
{
    uint32_t hdr;
    uint16_t pkg_len;
    uint8_t cmd;
    uint8_t status;
} uart_resp_t;
#pragma pack(pop)

std::string board_num;
std::string board;


static auto encode_uart_cmd(npcm_uart_cmd cmd, unsigned int addr, 
                            uint16_t pkg_len = PACKAGE_CMD_LENGTH)
{
    uart_cmd_t uartCmd = {};
    uartCmd.hdr = MAGIC_HEADER;
    uartCmd.pkg_len = pkg_len;
    uartCmd.cmd = cmd;
    uartCmd.addr = addr;
    return uartCmd;
}

static auto crc8(const uint8_t *src, size_t len, uint8_t polynomial, 
                 uint8_t initialValue, bool reversed)
{
	uint8_t crc = initialValue;
	size_t i, j;

	for (i = 0; i < len; i++)
    {
		crc ^= src[i];

		for (j = 0; j < 8; j++)
        {
            crc = reversed ? (crc & 0x01) ? (crc >> 1) ^ polynomial : crc >> 1
                           : (crc & 0x80) ? (crc << 1) ^ polynomial : crc << 1;
		}
	}

	return crc;
}

static auto crc8_compare(char *buffer, int size, uint8_t expectCRC)
{
	auto crc = crc8((const uint8_t *)buffer, size, 0x31, 0, false);
    return crc == expectCRC;
}

static auto set_console_service_state(std::string console, bool state)
{
    auto pos = console.find_last_of("/");
    auto deviceName = console.substr(pos + 1);
    auto serviceName = std::string("obmc-console@") + deviceName + ".service";
    auto cmd = std::string("/bin/systemctl ") + 
                (state ? "start" : "stop") + " " + serviceName;

    std::cout << "Attempting to " << (state ? "start " : "stop ") 
              << serviceName << "..." << std::endl;

    auto rc = std::system(cmd.c_str());
    if (rc)
    {
        std::cerr << "Failed to " << (state ? "start " : "stop ") 
                  << "console service: " << serviceName 
                  << ", rc="<< rc << std::endl;
        return false;
    }
    sleep(1); // wait for console service to start/stop
    return true;
}

class DefaultRecoveryOps : public RecoveryOps {
public:
    bool set_recovery_mode(bool enable, int bus) override { 
        std::string cmd;
        cmd += "i2cset -y " + std::to_string(bus) + " 0x21 0x32 " + (enable ? "0x00" : "0x40");
        cmd += " && sleep 1s";
        cmd += " && i2cset -y " + std::to_string(bus) + " 0x21 0x00 0xF7";
        cmd += " && sleep 5s";
        cmd += " && i2cset -y " + std::to_string(bus) + " 0x21 0x00 0xFF";
        cmd += " && sleep 3s";
        std::cout << "Attempting to " << (enable ? "enable" : "disable")
                  << " recovery mode on bus " << bus << "...\n";
        auto rc = std::system(cmd.c_str());
        if (rc)
        {
            std::cerr << "Failed to set recovery mode to " << enable 
                    << ", rc=" << rc << std::endl;
            return false;
        }
        return true;
    }

    SerialSel resolve_serial(const std::string&, const std::string&) override {
        std::cout << "[INFO] Using default UART/bus\n";
        return SerialSel{DEFAULT_SERIAL_PORT, 11};
    }
};

std::unique_ptr<RecoveryOps> __attribute__((weak)) create_recovery_ops() {
    return std::make_unique<DefaultRecoveryOps>();
}

class NpcmDevice
{
  public:
    NpcmDevice(RecoveryOps& ops_, const std::string& serialPath, int i2cBus)
    : ops(ops_), serial(serialPath), bus(i2cBus) {
        set_console_service_state(serial.get_path(), false);
        ops.set_recovery_mode(true, bus);
        serial.open_port();
        sleep(1);
      }
    ~NpcmDevice()
    {
        ops.set_recovery_mode(false, bus);
        set_console_service_state(serial.get_path(), true);
    }

    bool program_loader(const std::string& loaderFile)
    {
        std::cout << "Check if loader is alive..." << std::endl;
        std::cout << "Sending UART_CMD_LOADER_ALIVE to address: 0x" 
                << std::hex << UART_LOADER_ADDRESS << std::dec << std::endl;
        auto rc = send_uart_cmd_to_npcm(UART_CMD_LOADER_ALIVE, 
                                        UART_LOADER_ADDRESS, nullptr, 0);
        std::cout << "UART command return code: " << rc << std::endl;
    
        if (rc == UART_RESP_STATUS_NO_ERR)
        {
            std::cout << "Loader is alive." << std::endl;
            return true;
        }
        std::cout << "Loader is not alive." << std::endl;

        std::ifstream file(loaderFile, std::ios::binary | std::ios::ate);
        if (!file)
        {
            std::cerr << "Faied to open loader file." << std::endl;
            return false;
        }

        std::streamsize loaderSize = file.tellg();
        file.seekg(0, std::ios::beg);

        auto loaderBuf = std::make_unique<char[]>(loaderSize);
        if (!file.read(loaderBuf.get(), loaderSize))
        {
            std::cerr << "Failed to read loader file." << std::endl;
            return false;
        }

        // Program loader to RAM
        std::cout << "Program Loader: " << std::endl;
        if (send_data_to_npcm(UART_CMD_WRITE_MEMORY, UART_LOADER_ADDRESS, 
                              loaderBuf.get(), loaderSize))
        {
            std::cerr << "Failed to program loader file." << std::endl;
            return false;
        }
 
        // Verify the written data
        std::cout << "Verify Loader: " << std::endl;
        if (verify_sent_data(UART_CMD_READ_MEMORY, UART_LOADER_ADDRESS, 
                             loaderBuf.get(), loaderSize))
        {
            std::cerr << "Failed to verify loader file." << std::endl;
            return false;
        }

        // Get loader entry pointer
        auto entryPointer = *reinterpret_cast<unsigned int*>(
                                loaderBuf.get() + UART_LOADER_ENTRY_OFFSET);

        // Jump to loader entry point
        if (send_uart_cmd_to_npcm(UART_CMD_EXE_ADR, entryPointer, nullptr, 0))
        {
            std::cerr << "Failed to jump to loader file." << std::endl;
            return false;
        }

        // Wait the loader become ready
        sleep(2);

        std::cout << "Program loader done." << std::endl;
        return true;
    }

    bool program_flash(const std::string& fwFile)
    {
        std::ifstream file(fwFile, std::ios::binary | std::ios::ate);
        if (!file)
        {
            std::cerr << "Faied to open firmware file." << std::endl;
            return false;
        }

        std::streamsize writeSize = file.tellg();
        file.seekg(0, std::ios::beg);

        auto writeBuf = std::make_unique<char[]>(writeSize);
        if (!file.read(writeBuf.get(), writeSize))
        {
            std::cerr << "Failed to read firmware file." << std::endl;
            return false;
        }

        auto rc = select_flash(0 /* mcp */);
        if(rc)
        {
            std::cerr << "Select flash failed, rc=" << rc << std::endl;
            return false;
        }

        int capacity = 0;
        rc = send_uart_cmd_to_npcm(UART_CMD_FLASH_READ_CAPACITY, 0, 
                    reinterpret_cast<char*>(&capacity), sizeof(capacity));
        if (rc)
        {
            std::cerr << "Failed to get flash capacity, rc=" << rc << std::endl;
            return false;
        }

        std::cout << std::hex
                  << "\nFlash selected: mcp\n"
                  << "Flash capacity: 0x" << capacity << "\n"
                  << "Input file: " << fwFile << "\n"
                  << "Write size: 0x" << writeSize << "\n"
                  << "Address: 0x" << 0x00 << "\n"
                  << std::dec << std::endl;  

        // Program firmware to flash
        std::cout << "Program flash: " << std::endl;
        rc = send_data_to_npcm(UART_CMD_FLASH_WRITE, 0, 
                                    writeBuf.get(), writeSize);
        if (rc)
        {
            std::cerr << "Failed to program flash, rc" << rc << std::endl;
            return false;
        }
    
        rc = send_uart_cmd_to_npcm(UART_CMD_FLASH_WRITE_FINISH, 0, nullptr, 0);
        if (rc)
        {
            std::cerr << "Failed to program flash, rc" << rc << std::endl;
            return false;
        }

        // Verify the written data
        std::cout << "Verify flash: " << std::endl;
        if (verify_sent_data(UART_CMD_FLASH_READ, 0, writeBuf.get(), writeSize))
        {
            std::cerr << "Verify flash failed." << std::endl;
            return false;
        }

        std::cout << "Program flash done." << std::endl;
        return true;
    }

    bool run_recovery(const std::string& loaderFile, const std::string& fwFile)
    {
        if (!program_loader(loaderFile))
        {
            std::cerr << "Failed to program loader." << std::endl;
            return false;
        }

        if (!program_flash(fwFile))
        {
            std::cerr << "Failed to program flash." << std::endl;
            return false;
        }

        std::cout << "Recovery success." << std::endl;
        return true;
    }

  private:
    RecoveryOps& ops;
    SerialDevice serial;
    int bus;

    int select_flash(uint8_t flash)
    {
        uint8_t crc = 0;
        uint16_t dataLength = 0x1;
        std::vector<char> rbuf(UART_RECEIVE_BUFFER_SIZE);

        // Make sure serial console rx data clean in non-block mode
        serial.read_data(rbuf.data(), rbuf.size());

        uart_cmd_t uartCmd = encode_uart_cmd(UART_CMD_FLASH_SELECT, 
            0 /*don't care*/, PACKAGE_CMD_LENGTH + DATA_CMD_LENGTH + dataLength);
        
        crc = crc8((const uint8_t *)&uartCmd.cmd, sizeof(uartCmd.cmd),
                0x31, crc, false);
        crc = crc8((const uint8_t *)&uartCmd.addr, sizeof(uartCmd.addr),
                0x31, crc, false);
        crc = crc8((const uint8_t *)&dataLength, sizeof(uint16_t), 0x31,
                crc, false);
        crc = crc8((const uint8_t *)&flash, sizeof(uint8_t), 0x31,
                crc, false);

        serial.write_data(reinterpret_cast<char*>(&uartCmd), sizeof(uart_cmd_t));
        serial.write_data(reinterpret_cast<char*>(&dataLength), sizeof(uint16_t));
        serial.write_data(reinterpret_cast<char*>(&flash), sizeof(uint8_t));
        serial.write_data(reinterpret_cast<char*>(&crc), sizeof(uint8_t));

        return receive_resp_data(rbuf.data(), MIN_RESP_PACKAGE_LENGTH);
    }

    int receive_resp_data(char* rbuf, size_t rlen)
    {
        int readCount = 0;
        size_t received = 0;
        uint16_t dataResLength = 0;
        
        while (received < rlen)
        {
            readCount = serial.read_data(rbuf + received, rlen - received);
            if (readCount <= 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    // No data available, continue to wait
                    continue;
                }
                else
                {
                    std::cerr << "Error in read: " 
                              << strerror(errno) << std::endl;
                    return UART_RESP_STATUS_READ_ERR;
                }
            }
            else
            {
                received += readCount;
            }

            if (received >= sizeof(uart_resp_t) + CRC8_LENGTH)
            {
                auto* res = reinterpret_cast<uart_resp_t*>(rbuf);
                if (res->status != 0)
                {
                    return res->status;
                }

                // Check if package has only the header
                if (res->pkg_len == PACKAGE_RESP_LENGTH)
                {
                    return UART_RESP_STATUS_NO_ERR;
                }

                // Check if have additional data
                if (received >= sizeof(uart_resp_t) + DATA_RESP_LENGTH)
                {
                    dataResLength = *reinterpret_cast<const uint16_t*>(
                                        rbuf + sizeof(uart_resp_t));
                }
            }

            if (received == sizeof(uart_resp_t) + PACKAGE_RESP_LENGTH + 
                            dataResLength + CRC8_LENGTH)
            {
                return UART_RESP_STATUS_NO_ERR;
            }
        }

        return UART_RESP_STATUS_INVALID_LEN;
    }

    int send_uart_cmd_to_npcm(const npcm_uart_cmd& cmd, const uint32_t& addr, 
                              char *rbuf, const size_t& rlen)
    {
        int rc = 0;
        std::vector<char> buf(UART_RECEIVE_BUFFER_SIZE);
        uart_cmd_t uartCmd = encode_uart_cmd(cmd, addr);

        // Make sure serial console rx data clean in non-block mode
        serial.read_data(buf.data(), buf.size());
        
        // Calculate CRC over command and address
        uint8_t crc = 0;
        crc = crc8((const uint8_t *)&uartCmd.cmd, sizeof(uartCmd.cmd),
                0x31, crc, false);
        crc = crc8((const uint8_t *)&uartCmd.addr, sizeof(uartCmd.addr),
                0x31, crc, false);

        serial.write_data(reinterpret_cast<char*>(&uartCmd), sizeof(uart_cmd_t));
        serial.write_data(reinterpret_cast<char*>(&crc), sizeof(uint8_t));

        rc = receive_resp_data(buf.data(), buf.size());
        if (rc == UART_RESP_STATUS_NO_ERR && rbuf)
        {
            memcpy(rbuf, buf.data() + sizeof(uart_resp_t) + DATA_RESP_LENGTH, rlen);
        }

        return rc;
    }

    int send_data_to_npcm(const npcm_uart_cmd& cmd, const uint32_t& addr, 
                          const char *tbuf, const size_t& tlen)
    {
        int rc = 0;
        size_t remaining = tlen;
        size_t offset = 0;
        size_t dataLength = 0;
        uint8_t crc = 0;
        uart_cmd_t uartCmd = {};
        std::vector<char> rbuf(UART_RECEIVE_BUFFER_SIZE);

        while (remaining)
        {
            // Make sure serial console rx data clean in non-block mode
            serial.read_data(rbuf.data(), rbuf.size());

            dataLength = remaining > UART_SEND_DATA_MAX ? 
                        UART_SEND_DATA_MAX : remaining;

            uartCmd = encode_uart_cmd(cmd, addr + offset, 
                            PACKAGE_CMD_LENGTH + DATA_CMD_LENGTH + dataLength);

            crc = 0;
            crc = crc8((const uint8_t *)&uartCmd.cmd, sizeof(uartCmd.cmd),
                    0x31, crc, false);
            crc = crc8((const uint8_t *)&uartCmd.addr, sizeof(uartCmd.addr),
                    0x31, crc, false);
            crc = crc8((const uint8_t *)&dataLength, sizeof(uint16_t),
                    0x31, crc, false);
            crc = crc8((const uint8_t *)(tbuf + offset), dataLength,
                    0x31, crc, false);

            serial.write_data(reinterpret_cast<char*>(&uartCmd), sizeof(uart_cmd_t));
            serial.write_data(reinterpret_cast<char*>(&dataLength), sizeof(uint16_t));
            serial.write_data(tbuf + offset, dataLength);
            serial.write_data(reinterpret_cast<char*>(&crc), sizeof(uint8_t));

            // Wait response
            rc = receive_resp_data(rbuf.data(), MIN_RESP_PACKAGE_LENGTH);
            if (rc)
            {
                std::cerr << "\nProgram loader address = 0x" << std::hex 
                          << static_cast<int>(uartCmd.addr) 
                          << " length = 0x" << std::hex 
                          << static_cast<int>(dataLength) 
                          << " failed, rc=" << rc << std::endl;
                return rc;
            }
            else
            {
                std::cout << "\r[" << (offset + dataLength) << "/" << tlen << "]" 
                          << std::flush;
            }

            offset += dataLength;
            remaining -= dataLength;

            if (remaining == 0) { std::cout << std::endl; }
        }
        return 0;
    }

    int verify_sent_data(const npcm_uart_cmd& cmd, const uint32_t& addr, 
                         const char *verify_buf, const size_t& verify_size)
    {
        int rc = 0;
        size_t offset = 0;
        size_t remaining = verify_size;
        uint8_t crc = 0;
        uint16_t receiveLength = 0;
        uart_cmd_t uartCmd = {};
        std::vector<char> rbuf(UART_RECEIVE_BUFFER_SIZE);

        while (remaining)
        {
            // Make sure serial console rx data clean in non-block mode
            serial.read_data(rbuf.data(), rbuf.size());
            
            receiveLength = remaining > UART_SEND_DATA_MAX ? 
                          UART_SEND_DATA_MAX : remaining;

            uartCmd = encode_uart_cmd(cmd, addr + offset, 
                                       PACKAGE_CMD_LENGTH + DATA_CMD_LENGTH);

            crc = 0;
            crc = crc8((const uint8_t *)&uartCmd.cmd, sizeof(uartCmd.cmd),
                    0x31, crc, false);
            crc = crc8((const uint8_t *)&uartCmd.addr, sizeof(uartCmd.addr),
                    0x31, crc, false);
            crc = crc8((const uint8_t *)&receiveLength, sizeof(uint16_t),
                    0x31, crc, false);

            serial.write_data(reinterpret_cast<char*>(&uartCmd), sizeof(uart_cmd_t));
            serial.write_data(reinterpret_cast<char*>(&receiveLength), sizeof(uint16_t));
            serial.write_data(reinterpret_cast<char*>(&crc), sizeof(uint8_t));

            // Wait response
            rc = receive_resp_data(rbuf.data(), rbuf.size());
            if (rc)
            {
                std::cerr << "Read offset = 0x" << std::hex 
                          << static_cast<int>(uartCmd.addr) 
                          << " length = 0x" << std::hex 
                          << static_cast<int>(receiveLength) 
                          << " failed, rc=" << rc << std::endl;
                return rc;
            }

            auto recv_ptr = rbuf.data() + sizeof(uart_resp_t) + DATA_RESP_LENGTH;
            auto expected_crc = *(recv_ptr + receiveLength);
            auto crc_len = sizeof(uart_resp_t) + DATA_RESP_LENGTH + receiveLength;

            if (!crc8_compare(rbuf.data(), crc_len, expected_crc))
            {
                std::cerr << "CRC verification failed at address: 0x" 
                          << std::hex << uartCmd.addr << "\n"
                          << "\nExpected CRC: 0x" << static_cast<int>(expected_crc)
                          << "\nLength of data checked: 0x" << receiveLength 
                          << std::endl;

                return UART_RESP_STATUS_VIRIFY_ERR;
            }

            if (std::memcmp(verify_buf + offset, recv_ptr, receiveLength) != 0)
            {
                std::cerr << "Compare read offset = 0x" << std::hex 
                          << uartCmd.addr << " length = 0x" << receiveLength 
                          << " failed." << std::endl;
                return UART_RESP_STATUS_VIRIFY_ERR;
            }
            else
            {
                std::cout << "\r[" << (offset + receiveLength) << "/" 
                          << verify_size << "]" << std::flush;
            }

            offset += receiveLength;
            remaining -= receiveLength;

            if (remaining == 0) { std::cout << std::endl; }
        }
        
        return 0;
    }
};

void check_gpio_switch()
{
    FILE* pipe = popen("gpiofind SCM_USB_SEL", "r");
    if (!pipe)
    {
        std::cerr << "Failed to run gpiofind." << std::endl;
        return;
    }
    char buffer[128];
    std::string gpioPath;
    if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
    {
        gpioPath = std::string(buffer);
        gpioPath.erase(gpioPath.find_last_not_of(" \n\r\t") + 1);
    }
    pclose(pipe);
    if (gpioPath.empty())
    {
        std::cerr << "GPIO SCM_USB_SEL not found." << std::endl;
        return;
    }
    std::cout << "Found GPIO path: " << gpioPath << std::endl;
    std::string gpiosetCmd = "gpioset " + gpioPath + "=0";
    int rc = std::system(gpiosetCmd.c_str());
    if (rc != 0)
    {
        std::cerr << "Failed to set GPIO low: " << gpiosetCmd << std::endl;
        return;
    }
    sleep(5);
    rc = std::system("ls /dev/ttyUSB* > /dev/null 2>&1");
    if (rc == 0)
    {
        std::cout << "[OK] UART enumerated, switch is likely working." << std::endl;
    }
    else
    {
        std::cerr << "[ERROR] UART device not found. Switch may be in recovery or failed." << std::endl;
    }    
}

 void recover_mmc(RecoveryOps *ops, const std::string& tarFile,
    const std::string& board_num, std::string board)
{
    std::unique_ptr<PldmUpdateLock> lock;
    try
    {
        lock = std::make_unique<PldmUpdateLock>();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << std::endl;
        return;
    }

    std::error_code ec;
    if (!fs::is_regular_file(tarFile, ec))
    {
        std::cerr << "Tarball " << tarFile << " does not exist: " 
                  << ec.message() << std::endl;
        return;
    }

    auto tarFilePath = fs::path(tarFile);
    if (!tarFilePath.string().ends_with(".tar.xz"))
    {
        std::cout << "The tarball is invalid. It should be a .tar.xz file." 
                  << std::endl;
        return;
    }

    auto filename = tarFilePath.stem().stem(); // remove .tar.xz
    auto tmpDir = fs::path("/tmp") / filename;

    if (!fs::exists(tmpDir) && !fs::create_directory(tmpDir, ec))
    {
        std::cerr << "Failed to create tmp dir: " << tmpDir << ", " 
                  << ec.message() << std::endl;
        return;
    }
    RemovablePath tmpDirToRemove(tmpDir);

    // Extract tarball
    auto cmd = std::string("tar -xf ") + tarFile + " -C " + tmpDir.string();
    int rc = std::system(cmd.c_str());
    if (rc != 0)
    {
        std::cerr << "Failed to extract tarball with exit code: " << rc 
                  << std::endl;
        return;
    }

    auto loaderFile = tmpDir / "loader_signed.bin";
    auto updateFile = tmpDir / (filename.string() + ".bin");



    if (!fs::exists(loaderFile, ec) || !fs::exists(updateFile, ec))
    {
        std::cerr << "Required files " << loaderFile << " and " << updateFile
                  << " missing after extraction in: " 
                  << tmpDir << std::endl;
        return;
    }
    else
    {
        SerialSel sel = ops->resolve_serial(board_num, board);
        NpcmDevice npcmDevice(*ops, sel.uart_path, sel.bus);
        if (!npcmDevice.run_recovery(loaderFile.string(), updateFile.string()))
        {
            std::cerr << "Failed to run recovery." << std::endl;
        }
    }
    
    std::cout << "Restarting pldmd.service..." << std::endl;
    rc = std::system("systemctl restart pldmd.service");
    if (rc)
    {
        std::cerr << "Failed to restart pldmd.service: " << rc << std::endl;
    }
}

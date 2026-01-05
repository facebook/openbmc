#pragma once
#include "cpld-handler.hpp"

#include <chrono>
#include <iomanip>

constexpr uint8_t busyWaitmaxRetry = 200;
constexpr uint8_t busyFlagBit = 0x80;
constexpr uint8_t statusRegBusy = 0x10;
constexpr uint8_t statusRegFail = 0x20;
constexpr std::chrono::milliseconds waitBusyTime(200);

static constexpr const char* TAG_QF = "QF";
static constexpr const char* TAG_UH = "UH";
static constexpr const char* TAG_CF_START = "L000";
static constexpr const char* TAG_TAG_DATA = "NOTE TAG DATA";
static constexpr const char* TAG_UFM = "NOTE USER MEMORY DATA";
static constexpr const char* TAG_CHECKSUM = "C";
static constexpr const char* TAG_USERCODE = "NOTE User Electronic";
static constexpr const char* TAG_EBR_INIT_DATA = "NOTE EBR_INIT DATA";
static constexpr const char* TAG_END_CONFIG = "NOTE END CONFIG DATA";
static constexpr const char* TAG_DEV_NAME = "NOTE DEVICE NAME";

constexpr uint8_t isOK = 0;
constexpr uint8_t isReady = 0;
constexpr uint8_t busyOrReadyBit = 4;
constexpr uint8_t failOrOKBit = 5;

enum cpldI2cCmd
{
    CMD_ERASE_FLASH = 0x0E,
    CMD_DISABLE_CONFIG_INTERFACE = 0x26,
    CMD_READ_STATUS_REG = 0x3C,
    CMD_RESET_CONFIG_FLASH = 0x46,
    CMD_PROGRAM_DONE = 0x5E,
    CMD_PROGRAM_PAGE = 0x70,
    CMD_READ_PAGE = 0x73,
    CMD_ENABLE_CONFIG_MODE = 0x74,
    CMD_SET_PAGE_ADDRESS = 0xB4,
    CMD_READ_FW_VERSION = 0xC0,
    CMD_PROGRAM_USER_CODE = 0xC2,
    CMD_READ_DEVICE_ID = 0xE0,
    CMD_READ_BUSY_FLAG = 0xF0,
};

struct cpldI2cInfo
{
    unsigned long int QF;
    unsigned int* UFM;
    unsigned int Version;
    unsigned int CheckSum;
    std::vector<uint8_t> cfgData;
    std::vector<uint8_t> ufmData;
};

class CpldLatticeManager : public CpldManager
{
  public:
    std::vector<uint8_t> fwData;
    cpldI2cInfo fwInfo{};
    CpldLatticeManager(const uint8_t bus, const uint8_t addr,
                       const std::string& path, const std::string& chip,
                       const std::string& interface, const std::string& target,
                       const bool debugMode) :
        CpldManager(bus, addr, path, chip, interface, target, debugMode)
    {}

    int getVersion() override;
    int fwUpdate(bool legacy) override;
    int fwVerifyOnly(bool legacy) override;
    int jedFileParser();

  private:
    int indexof(const char* str, const char* ptn);
    int readDeviceId();
    int enableProgramMode();
    int eraseFlash();
    int resetConfigFlash();
    int writeProgramPage();
    int programUserCode();
    int programDone();
    int verifyData();
    int verifyUserCode();
    int disableusyAndVerify();
    int disableConfigInterface();
    int readBusyFlag(uint8_t& busyFlag);
    int readStatusReg(uint8_t& statusReg);
    bool waitBusyAndVerify();
    int readUserCode(uint32_t& userCode);
    int XO2XO3Family_update();
    int XO2XO3Family_verifyOnly();
    int XO5Family_update(bool legacy);
    int programSinglePage(uint16_t page_offset, span<const uint8_t> page_data);
    int verifySinglePage(uint16_t page_offset, span<const uint8_t> page_data);
    int setPageAddr(uint16_t page_offset);
    static void updateFailedWarning();
};

class XO5I2CManager : public CpldLatticeManager
{
  public:
    XO5I2CManager(uint8_t bus, uint8_t addr, const std::string& path,
                  const std::string& chip, const std::string& interface,
                  const std::string& target, bool debugMode,
                  bool legacy = false) :
        CpldLatticeManager(bus, addr, path, chip, interface, target, debugMode),
        legacyMode(legacy),
        cfgIndex{static_cast<uint8_t>(
            target == "CFG1" ? (legacyMode ? XO5_PARTITION_CFG1 : 1) : 0)}
    {}

    bool ready()
    {
        return waitUntilReady();
    }
    bool eraseCfg();
    bool programCfg();
    bool verifyCfg();

  private:
    enum class Cmd : uint8_t
    {
        SectorErase = 0xd8,
        PageProgram = 0x02,
        PageRead = 0x0b,
        ReadUsercode = 0xc0
    };

    enum class Status : uint8_t
    {
        Ready = 0x00,
        NotReady = 0xff
    };

    struct Cfg
    {
        static constexpr size_t PageSize = 256;
        static constexpr size_t PagesPerBlock = 256;
        static constexpr size_t BlocksPerCfg = 11;
    };

    static constexpr std::array<uint8_t, 3> CfgStartBlocks = {0x01, 0x10, 0x1F};
    static constexpr auto ReadyPollInterval = std::chrono::milliseconds(10);
    static constexpr auto ReadyTimeout = std::chrono::milliseconds(1000);
    static constexpr auto ErasePageDelay = std::chrono::milliseconds(250);

    constexpr auto getStartBlock(uint8_t cfg) const
    {
        if (cfg >= CfgStartBlocks.size())
        {
            throw std::out_of_range("Invalid cfg number");
        }
        return CfgStartBlocks[cfg];
    }

    void updateProgress(size_t block, size_t page, size_t bytes,
                        size_t totalBytes) const
    {
        std::cout << "Block " << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(block) << " Page " << std::setw(2) << static_cast<int>(page) << " - " << std::fixed << std::setprecision(1) << (100.0f * bytes / totalBytes) << "%  \r"
                  << std::flush;
    }

    bool waitUntilReady(std::chrono::milliseconds timeout = ReadyTimeout);
    bool programPage(uint8_t block, uint8_t page,
                     span<const uint8_t> data);
    bool readPage(uint8_t block, uint8_t page, span<uint8_t> data);

    // ==================================================
    // LEGACY XO5 PROGRAMMING MODE
    // ==================================================
    static constexpr auto XO5_CMD_SET_PAGE = 0x01;
    static constexpr auto XO5_CMD_ERASE_FLASH = 0x02;
    static constexpr auto XO5_CMD_CFG_WRITE_PAGE = 0x11;
    static constexpr auto XO5_CMD_CFG_READ_PAGE = 0x19;

    static constexpr auto XO5_PARTITION_CFG0 = 0x00;
    static constexpr auto XO5_PARTITION_CFG1 = 0x02;

    static constexpr auto XO5_ERASE_BLOCK = 0x0;
    static constexpr auto XO5_ERASE_WHOLE = 0x1;

    bool setPage(uint8_t cfg, uint8_t block, uint8_t page);
    bool legacyProgramPage(span<const uint8_t> data);
    bool legacyReadPage(span<uint8_t> data);

    const bool legacyMode;
    // ==================================================

    uint8_t cfgIndex;
};

#pragma once
#include "cpld-fw-handler.hpp"

#include <chrono>

constexpr uint8_t busyWaitmaxRetry = 30;
constexpr uint8_t busyFlagBit = 0x80;
constexpr uint8_t statusRegBusy = 0x10;
constexpr uint8_t statusRegFail = 0x20;
constexpr std::chrono::milliseconds waitBusyTime(200);

static constexpr auto TAG_QF = "QF";
static constexpr auto TAG_UH = "UH";
static constexpr auto TAG_CF_START = "L000";
static constexpr auto TAG_UFM = "NOTE TAG DATA";
static constexpr auto TAG_ROW = "NOTE FEATURE";
static constexpr auto TAG_CHECKSUM = "C";
static constexpr auto TAG_USERCODE = "NOTE User Electronic";
static constexpr auto TAG_EBR_INIT_DATA = "NOTE EBR_INIT DATA";
static constexpr auto TAG_CF_END = "NOTE END OF CFG";

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
    CMD_ENABLE_CONFIG_MODE = 0x74,
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
    int fwUpdate() override;
    int getVersion() override;
    int XO2XO3Family_update();
    int XO5Family_update();

  private:
    int indexof(const char* str, const char* ptn);
    int jedFileParser();
    int readDeviceId();
    int enableProgramMode();
    int eraseFlash();
    int resetConfigFlash();
    int writeProgramPage();
    int programUserCode();
    int programDone();
    int disableusyAndVerify();
    int disableConfigInterface();
    int readBusyFlag(uint8_t& busyFlag);
    int readStatusReg(uint8_t& statusReg);
    bool waitBusyAndVerify();
    int readUserCode(uint32_t& userCode);
};

class XO5I2CManager : public CpldLatticeManager
{
  friend class CpldLatticeManager;
  public:
    // XO5 commands
    static const uint8_t XO5_CMD_IDLE = 0x00;
    static const uint8_t XO5_CMD_SET_PAGE = 0x01;
    static const uint8_t XO5_CMD_ERASE_FLASH = 0x02;
    static const uint8_t XO5_CMD_CFG_WRITE_PAGE = 0x11;
    static const uint8_t XO5_CMD_CFG_RESET_ADDR = 0x12;
    static const uint8_t XO5_CMD_CFG_READ_PAGE = 0x19;
    static const uint8_t XO5_CMD_UFM_WRITE_PAGE = 0x20;
    static const uint8_t XO5_CMD_UFM_READ_PAGE = 0x21;
    static const uint8_t XO5_CMD_UFM_RESET_ADDR = 0x22;
    static const uint8_t XO5_CMD_USERDATA_WRITE_PAGE = 0x30;
    static const uint8_t XO5_CMD_USERDATA_READ_PAGE = 0x31;
    static const uint8_t XO5_CMD_USERDATA_RESET_ADDR = 0x32;

    // Flash partition (X25)
    static const uint8_t XO5_PARTITION_CFG0 = 0x00;
    static const uint8_t XO5_PARTITION_UFM0 = 0x01;
    static const uint8_t XO5_PARTITION_CFG1 = 0x02;
    static const uint8_t XO5_PARTITION_UFM1 = 0x03;
    static const uint8_t XO5_PARTITION_CFG2 = 0x04;
    static const uint8_t XO5_PARTITION_UFM2 = 0x05;
    static const uint8_t XO5_PARTITION_USERDATA0 = 0x06;
    static const uint8_t XO5_PARTITION_USERDATA1 = 0x07;
    static const uint8_t XO5_PARTITION_USERDATA2 = 0x08;
    static const uint8_t XO5_PARTITION_USERDATA3 = 0x09;
    static const uint8_t XO5_PARTITION_USERDATA4 = 0x0A;
    static const uint8_t XO5_PARTITION_USERDATA5 = 0x0B;
    static const uint8_t XO5_PARTITION_USERDATA6 = 0x0C;
    static const uint8_t XO5_PARTITION_USERDATA7 = 0x0D;
    static const uint8_t XO5_PARTITION_USERDATA8 = 0x0E;

    // Flash erase size
    static const uint8_t XO5_ERASE_BLOCK = 0x0;
    static const uint8_t XO5_ERASE_WHOLE = 0x1;

    static const int XO5_PAGE_NUM = 256;
    static const int XO5_PAGE_SIZE = 256;
    static const int XO5_CFG_BLOCK_NUM = 11;

    XO5I2CManager(const uint8_t bus, const uint8_t addr,
                       const std::string& path, const std::string& chip,
                       const std::string& interface, const std::string& target,
                       const bool debugMode) :
        CpldLatticeManager(bus, addr, path, chip, interface, target, debugMode)
    {}

  private:
    int XO5jedFileParser();
    int set_page(uint8_t partition_num, uint32_t page_num);
    int erase_flash(uint8_t size);
    int cfg_reset_addr();
    std::vector<uint8_t> cfg_read_page();
    int cfg_write_page(const std::vector<uint8_t>& byte_list);
    bool programCfgData(uint8_t cfg);
    bool verifyCfgData(uint8_t cfg);
};

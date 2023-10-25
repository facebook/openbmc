#pragma once
#include <chrono>
#include "cpld-fw-handler.hpp"

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
    CMD_READ_DEVICE_ID = 0xE0,
    CMD_READ_BUSY_FLAG = 0xF0,
};

typedef struct
{
  unsigned long int QF;
  unsigned int *UFM;
  unsigned int Version;
  unsigned int CheckSum;
  std::vector<uint8_t> cfgData;
  std::vector<uint8_t> ufmData;
} cpldI2cInfo;

class CpldLatticeManager : public CpldManager
{
public:
    std::vector<uint8_t> fwData;
    cpldI2cInfo fwInfo{};
    CpldLatticeManager(const uint8_t bus, const uint8_t addr, const std::string &path,
                       const std::string &chip, const std::string &interface, const bool debugMode) :
                       CpldManager(bus, addr, path, chip, interface, debugMode) {}
    int fwUpdate() override;
    int getVersion() override;
    int XO2XO3Family_update();

private:
    int indexof(const char *str, const char *ptn);
    int jedFileParser();
    int readDeviceId();
    int enableProgramMode();
    int eraseFlash();
    int resetConfigFlash();
    int writeProgramPage();
    int programDone();
    int disableusyAndVerify();
    int disableConfigInterface();
    int readBusyFlag(uint8_t &busyFlag);
    int readStatusReg(uint8_t &statusReg);
    bool waitBusyAndVerify();

};

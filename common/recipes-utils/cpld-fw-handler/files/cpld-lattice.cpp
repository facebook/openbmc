#include "cpld-lattice.hpp"
#include "xo5/xo5_sram_recover.hpp"
#include <openssl/sha.h>
#include <unistd.h>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <map>
#include <numeric>
#include <thread>
#include <vector>

static uint8_t reverse_bit(uint8_t b)
{
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

const std::map<std::string, std::vector<uint8_t>> chipToDeviceIdMappingTable = {
    {"LCMXO3LF-4300", {0x61, 0x2b, 0xc0, 0x43}},
    {"LCMXO3LF-6900", {0x61, 0x2b, 0xd0, 0x43}},
    {"LCMXO3D-4300", {0x01, 0x2e, 0x20, 0x43}},
    {"LCMXO3D-9400", {0x21, 0x2e, 0x30, 0x43}},
    {"LFMXO5-25", {0x01, 0x0f, 0x70, 0x43}},
    {"LFMXO5-65T", {0x01, 0x0f, 0xc0, 0x43}},
};

const std::map<std::string, std::vector<uint8_t>> MagicDeviceIdMappingTable = {
    {"LFMXO5-65T", {0x01, 0x0f, 0xc0, 0x44}},
};

int CpldLatticeManager::lockI2c()
{
    if (!isSOFTIP || ((softIpVersion & 0xF0) != 0x20))
    {
        return 0;
    }

    std::vector<uint8_t> cmd = {CMD_I2C_LOCK, 0x1};
    constexpr size_t resSize = 1;
    std::vector<uint8_t> readData(resSize, 0);
    if (i2cWriteReadCmd(cmd, resSize, readData) < 0)
    {
        std::cerr << "Failed to send lock command." << std::endl;
        return -1;
    }

    if (!readData.empty() && (readData[0] & 0xFE) != 0x00)
    {
        return 0;
    }

    std::cerr << "I2C lock not acquired, response: 0x" << std::hex << (int)readData[0] << std::endl;
    return -1;
}

bool CpldLatticeManager::setCrcMode(bool enable)
{
    std::vector<uint8_t> cmd = {
        0xfd, static_cast<uint8_t>(enable ? 0x01 : 0x00), 0x00, 0x00};

    int ret = i2cWriteReadCmd(cmd);
    if (ret < 0) {
        std::cerr << "Fail to " << (enable ? "enable" : "disable")
        << " CRC mode." << std::endl;
        return false;
    }
    return true;
}

bool CpldLatticeManager::CheckSOFTIP()
{
    if (!setCrcMode(false)) {
        return false;
    }
    std::vector<uint8_t> cmd = {CMD_READ_SOFTIP_ID, 0x0, 0x0, 0x0};

    constexpr size_t resSize = 5;
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if (ret < 0)
    {
        std::cout << "Fail to read SOFTIP Id." << std::endl;
        return false;
    }

    setCrcMode(true);

    std::vector<uint8_t> MagicNumber(readData.begin(), readData.begin() + 4);
    bool matchFound = false;
    for (const auto& [name, idBytes] : MagicDeviceIdMappingTable)
    {
        if (idBytes.size() >= 4 && std::equal(MagicNumber.begin(),
        MagicNumber.end(), idBytes.begin()))
        {
            matchFound = true;
            break;
        }
    }
    if (matchFound) {
        isSOFTIP = true;
        softIpVersion = readData[4];
        if (debugMode)
        {
            std::cout << std::format("Detected SOFTIP, Version: v{}.{}\n",
                                     softIpVersion >> 4,
                                     softIpVersion & 0x0f);
        }
    }

    return isSOFTIP;
}

int CpldLatticeManager::jedFileParser()
{
    enum class ParseState
    {
        None,
        Cfg,
        EndCfg,
        Ufm,
        Checksum,
        UserCode
    };
    ParseState state = ParseState::None;
    std::vector<uint8_t> sumOnly;

    std::string line;
    std::ifstream ifs(imagePath);
    if (!ifs)
    {
        std::cerr << "Failed to open JED file\n";
        return -1;
    }

    auto pushPage = [](std::string& line, std::vector<uint8_t>& sector) {
        if (line[0] == '0' || line[0] == '1')
        {
            while (line.size() >= 8)
            {
                try
                {
                    sector.push_back(static_cast<uint8_t>(
                        std::stoi(line.substr(0, 8), 0, 2)));
                    line.erase(0, 8);
                }
                catch (...)
                {
                    break;
                }
            }
        }
    };

    while (getline(ifs, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty())
        {
            continue;
        }

        if (line.starts_with(TAG_QF))
        {
            ssize_t numberSize = line.find("*") - line.find("F") - 1;
            if (numberSize > 0)
            {
                fwInfo.QF =
                    std::stoul(line.substr(TAG_QF.length(), numberSize));
                std::cout << std::format("QF Size = {}\n", fwInfo.QF);
            }
        }
        else if (line.starts_with(TAG_CF_START) ||
                 line.starts_with(TAG_EBR_INIT_DATA))
        {
            state = ParseState::Cfg;
            continue;
        }
        else if (line.starts_with(TAG_END_CONFIG))
        {
            state = ParseState::EndCfg;
            continue;
        }
        else if (line.starts_with(TAG_UFM) || line.starts_with(TAG_TAG_DATA))
        {
            state = ParseState::Ufm;
            continue;
        }
        else if (line.starts_with(TAG_USERCODE))
        {
            state = ParseState::UserCode;
            continue;
        }
        else if (line.starts_with(TAG_CHECKSUM))
        {
            state = ParseState::Checksum;
        }
        else if (line.starts_with(TAG_DEV_NAME))
        {
            std::cout << line << "\n";
            if (line.find(chip) == std::string::npos)
            {
                std::cerr
                    << "STOP UPDATING: The image does not match the chip.\n";
                ifs.close();
                return -1;
            }
        }

        switch (state)
        {
            case ParseState::Cfg:
                pushPage(line, fwInfo.cfgData);
                break;
            case ParseState::EndCfg:
                pushPage(line, sumOnly);
                break;
            case ParseState::Ufm:
                pushPage(line, fwInfo.ufmData);
                break;
            case ParseState::Checksum:
                if (line.size() > 1)
                {
                    state = ParseState::None;
                    ssize_t numberSize = line.find("*") - line.find("C") - 1;
                    if (numberSize <= 0)
                    {
                        std::cerr << "Error in parsing checksum\n";
                        ifs.close();
                        return -1;
                    }
                    static constexpr auto start = TAG_CHECKSUM.length();
                    std::istringstream iss(line.substr(start, numberSize));
                    iss >> std::hex >> fwInfo.CheckSum;
                    std::cout << std::format("Checksum = 0x{:04X}\n",
                                             fwInfo.CheckSum);
                }
                break;
            case ParseState::UserCode:
                if (line.starts_with(TAG_UH))
                {
                    state = ParseState::None;
                    ssize_t numberSize = line.find("*") - line.find("H") - 1;
                    if (numberSize <= 0)
                    {
                        std::cerr << "Error in parsing usercode\n";
                        ifs.close();
                        return -1;
                    }
                    std::istringstream iss(
                        line.substr(TAG_UH.length(), numberSize));
                    iss >> std::hex >> fwInfo.Version;
                    std::cout
                        << std::format("UserCode = 0x{:08X}\n", fwInfo.Version);
                }
                break;
            default:
                break;
        }
    }
    ifs.close();

    std::cout << std::format("CFG Size = {}\n", fwInfo.cfgData.size());
    if (fwInfo.ufmData.size() > 0)
    {
        std::cout << std::format("UFM Size = {}\n", fwInfo.ufmData.size());
    }

    uint32_t calculated = 0u;
    auto addByte = [](uint32_t sum, uint8_t byte) {
        return sum + reverse_bit(byte);
    };
    calculated = std::accumulate(fwInfo.cfgData.begin(), fwInfo.cfgData.end(),
                                 calculated, addByte);
    calculated =
        std::accumulate(sumOnly.begin(), sumOnly.end(), calculated, addByte);
    calculated = std::accumulate(fwInfo.ufmData.begin(), fwInfo.ufmData.end(),
                                 calculated, addByte);
    if (fwInfo.CheckSum != (calculated & 0xFFFF))
    {
        std::cerr << std::format("JED File Checksum Error - {:X}\n",
                                 calculated);
        return -1;
    }
    std::cout << std::format("JED File Checksum = 0x{:X}\n", calculated);

    return 0;
}

int CpldLatticeManager::readDeviceId()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    // 0xE0
    std::vector<uint8_t> cmd = {CMD_READ_DEVICE_ID, 0x0, 0x0, 0x0};
    size_t resSize;
    if (isSOFTIP)
    {
        if ((softIpVersion & 0xF0) == 0x10) {
            appendCrc16(cmd);
            resSize = 6;
        } else {
            resSize = 4;
        }
    }
    else
    {
        resSize = 4;
    }
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if (ret < 0)
    {
        std::cout << "Fail to read device Id." << std::endl;
        return -1;
    }

    if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd, resSize, readData) < 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Read device ID fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }

    readData.resize(4);
    std::cout << "Device ID = ";
    for (auto v : readData)
    {
        std::cout << std::hex << std::setfill('0') << std::setw(2)
                  << unsigned(v) << " ";
    }

    auto chipWantToUpdate = chipToDeviceIdMappingTable.find(chip);

    if (chipWantToUpdate != chipToDeviceIdMappingTable.end() &&
        chipWantToUpdate->second == readData)
    {
        if (chip.rfind("LCMXO3D", 0) == 0)
        {
            isLCMXO3D = true;
            if (!target.empty() && target != "CFG0" && target != "CFG1")
            {
                std::cerr << "Error: unknown target." << std::endl;
                return -1;
            }
        }

        std::cout << "[OK] Device ID match with chip\n";
        return 0;
    }

    std::cerr << "ERROR: The device id not match with chip.\n";
    std::cerr << "Only the following chip names are supported: \n";
    for (const auto& chip : chipToDeviceIdMappingTable)
    {
        std::cerr << chip.first << "\n";
    }

    return -1;
}

int CpldLatticeManager::autoDetectChip()
{
    std::vector<uint8_t> cmd = {CMD_READ_DEVICE_ID, 0x0, 0x0, 0x0};
    std::array<uint8_t, 5> readData = {};

    if (CheckSOFTIP() && ((softIpVersion & 0xF0) == 0x10))
    {
        appendCrc16(cmd);
    }

    if (i2cWriteReadCmd(cmd, readData.size(), readData) != 0)
    {
        std::cerr << "Fail to read device Id for auto-detection.\n";
        return -1;
    }

    for (const auto& [name, idBytes] : chipToDeviceIdMappingTable)
    {
        if (std::equal(idBytes.begin(), idBytes.end(), readData.begin()) ||
            (readData[0] == 0x00 &&
             std::equal(idBytes.begin(), idBytes.end(), readData.begin() + 1)))
        {
            this->chip = name;
            std::cout << std::format("Found CPLD Chip: {}\n", name);
            return 0;
        }
    }
    std::cerr << std::format(
        "Unknown Device ID: {:02X} {:02X} {:02X} {:02X} {:02X}\n", readData[0],
        readData[1], readData[2], readData[3], readData[4]);

    return -1;
}

int CpldLatticeManager::enableProgramMode()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    // 0x74 transparent mode
    std::vector<uint8_t> cmd;
    if (isSOFTIP)
    {
        cmd = {CMD_ENABLE_CONFIG_MODE, 0x02, 0x0, 0x0};
        appendCrc16(cmd);
    }
    else
    {
        cmd = {CMD_ENABLE_CONFIG_MODE, 0x08, 0x0, 0x0};
    }

    if (i2cWriteReadCmd(cmd) < 0)
    {
        return -1;
    }

    if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd) < 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Enable program mode fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }
    else
    {
        if (!waitBusyAndVerify())
        {
            std::cerr << "Wait busy and verify fail" << std::endl;
            return -1;
        }
        std::this_thread::sleep_for(waitBusyTime);
    }
    return 0;
}

int CpldLatticeManager::eraseFlash()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    std::vector<uint8_t> cmd;

    if (isLCMXO3D || isSOFTIP)
    {
        /*
        Erase the different internal
        memories. The bit in YYY defines
        which memory is erased in Flash
        access mode.
        Bit 1=Enable
        8 Erase CFG0
        9 Erase CFG1
        10 Erase UFM0
        11 Erase UFM1
        12 Erase UFM2
        13 Erase UFM3
        14 Erase CSEC
        15 Erase USEC
        16 Erase PUBKEY
        17 Erase AESKEY
        18 Erase FEA
        19 Reserved
        CMD_ERASE_FLASH = 0x0E, 0Y YY 00
        */
        if (target.empty() || target == "CFG0")
        {
            cmd = {CMD_ERASE_FLASH, 0x00, 0x01, 0x00};
        }
        else if (target == "CFG1")
        {
            cmd = {CMD_ERASE_FLASH, 0x00, 0x02, 0x00};
        }
        else
        {
            std::cerr << "Error: unknown target." << std::endl;
            return -1;
        }
        if (isSOFTIP)
        {
            appendCrc16(cmd);
        }
    }
    else
    {
        cmd = {CMD_ERASE_FLASH, 0xC, 0x0, 0x0};
    }

    int ret = i2cWriteReadCmd(cmd);
    if (ret < 0)
    {
        return ret;
    }

    if (isSOFTIP){
        uint8_t retry = 0;
        uint8_t CRCandBusyCheck = waitBusyAndVerifyCRC();
        while ((CRCandBusyCheck & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd) < 0)
            {
                return -1;
            }
            retry++;
        }
        while ((CRCandBusyCheck & BUSY_MASK))
        {
            std::this_thread::sleep_for(1s);
            CRCandBusyCheck = waitBusyAndVerifyCRC();
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Erase flash fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }
    else if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }
    std::this_thread::sleep_for(waitBusyTime);
    return 0;
}

int CpldLatticeManager::resetConfigFlash()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    // CMD_RESET_CONFIG_FLASH = 0x46

    std::vector<uint8_t> cmd;
    if (isLCMXO3D || isSOFTIP)
    {
        /*
        Set Page Address pointer to the
        beginning of the different internal
        Flash sectors. The bit in YYYY
        defines which sector is selected.
        Bit Flash sector selected
        8 CFG0
        9 CFG1
        10 FEA
        11 PUBKEY
        12 AESKEY
        13 CSEC
        14 UFM0
        15 UFM1
        16 UFM2
        17 UFM3
        18 USEC
        19 Reserved
        20 Reserved
        21 Reserved
        22 Reserved
        CMD_RESET_CONFIG_FLASH = 0x46, YY YY 00
        */
        if (target.empty() || target == "CFG0")
        {
            cmd = {CMD_RESET_CONFIG_FLASH, 0x00, 0x01, 0x00};
        }
        else if (target == "CFG1")
        {
            cmd = {CMD_RESET_CONFIG_FLASH, 0x00, 0x02, 0x00};
        }
        else
        {
            std::cerr << "Error: unknown target." << std::endl;
            return -1;
        }
        if (isSOFTIP)
        {
            appendCrc16(cmd);
        }
    }
    else
    {
        cmd = {CMD_RESET_CONFIG_FLASH, 0x0, 0x0, 0x0};
    }

    int ret = i2cWriteReadCmd(cmd);
    if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd) < 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Reset config flash fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }
    return ret;
}

int CpldLatticeManager::setPageAddr(uint16_t page_offset)
{
    std::vector<uint8_t> cmd = {CMD_SET_PAGE_ADDRESS, 0x0, 0x0, 0x0, 0x00, 0x00, 0x00, 0x00};

    // Set Address command Bit[17:14]:
    // 4’h0              CFG0
    // 4’h1              UFM0
    // 4’h2              Reserved
    // 4’h3              FEA
    // 4’h4              CFG1
    // 4’h5              UFM1
    // 4’h6              PUBKEY
    // 4’h7              CSEC
    // 4’h8              UFM2
    // 4’h9              UFM3
    // 4’hA              ASEKEY
    // 4’hB              USEC
    if (target == "CFG1") {
        cmd[5] = 0x01;
    }

    cmd[6] = (page_offset / 256);
    cmd[7] = (page_offset % 256);

    if (i2cWriteReadCmd(cmd) < 0)
    {
        std::cerr << "Set page address fail (write)" << std::endl;
        return -1;
    }

    return 0;
}

int CpldLatticeManager::programSinglePage(uint16_t page_offset, std::span<const uint8_t> page_data)
{
    if (setPageAddr(page_offset) < 0) {
        std::cerr << "Set page command failed" << std::endl;
        return -1;
    }

    // Write Page Data
    std::vector<uint8_t> writeCmd = {CMD_PROGRAM_PAGE, 0x0, 0x0, 0x01};
    writeCmd.insert(writeCmd.end(), page_data.begin(), page_data.end());

    if (i2cWriteReadCmd(writeCmd) < 0)
    {
        std::cerr << "Write page data failed" << std::endl;
        return -1;
    }

    usleep(200);

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }

    return 0;
}

int CpldLatticeManager::verifySinglePage(uint16_t page_offset, std::span<const uint8_t> page_data)
{
    if (setPageAddr(page_offset) < 0) {
        std::cerr << "Set page command failed" << std::endl;
        return -1;
    }

    // Read Page Data
    std::vector<uint8_t> readCmd = {CMD_READ_PAGE, 0x0, 0x0, 0x1};
    std::vector<uint8_t> readData(16);
    if (i2cWriteReadCmd(readCmd, page_data.size(), readData) < 0)
    {
        std::cerr << "Read page data failed" << std::endl;
        return -1;
    }

    auto mismatch_pair = std::mismatch(page_data.begin(), page_data.end(), readData.begin());
    if (mismatch_pair.first != page_data.end())
    {
        size_t idx = std::distance(page_data.begin(), mismatch_pair.first);
        std::cerr << "Verify failed at " << ((page_offset * 16) + idx) << std::endl;
        return -1;
    }

    return 0;
}

int CpldLatticeManager::writeProgramPage()
{
    const size_t iterSize = 16;

    for (size_t i = 0; (i * iterSize) < fwInfo.cfgData.size(); i ++)
    {
        size_t byteOffset = i * iterSize;
        double progressRate =
            ((double(byteOffset) / double(fwInfo.cfgData.size())) * 100);
        std::cout << "Update : " << std::fixed << std::dec
                  << std::setprecision(2) << progressRate << "% \r";

        uint8_t len = ((byteOffset + iterSize) < fwInfo.cfgData.size())
                          ? iterSize
                          : (fwInfo.cfgData.size() - byteOffset);

        auto page_data = std::vector<uint8_t>(fwInfo.cfgData.begin() + byteOffset, fwInfo.cfgData.begin() + byteOffset + len);

        size_t retry = 0;
        const size_t maxWriteRetry = 10;
        while (retry < maxWriteRetry)
        {
            if (programSinglePage(i, page_data) == 0 &&
                verifySinglePage(i, page_data) == 0)
            {
                break; // Success
            }

            ++retry;
        }

        if (retry >= maxWriteRetry)
        {
            std::cerr << "Program and verify page failed" << std::endl;
            return -1;
        }
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }
    return 0;
}

int CpldLatticeManager::programUserCode()
{
    /*
    CMD_PROGRAM_USER_CODE = 0xC2,

    Program user code.
    */
    std::vector<uint8_t> cmd = {CMD_PROGRAM_USER_CODE, 0x0, 0x0, 0x0};
    for (int i = 3; i >= 0; i--)
    {
        cmd.push_back((fwInfo.Version >> (i * 8)) & 0xFF);
    }

    if (i2cWriteReadCmd(cmd) < 0)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }

    return 0;
}

int CpldLatticeManager::programDone()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    // CMD_PROGRAM_DONE = 0x5E
    std::vector<uint8_t> cmd = {CMD_PROGRAM_DONE, 0x0, 0x0, 0x0};

    if (isSOFTIP)
    {
        appendCrc16(cmd);
    }

    if (i2cWriteReadCmd(cmd) < 0)
    {
        return -1;
    }

    if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd) < 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Program done fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }else if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }


    return 0;
}

int CpldLatticeManager::verifyData()
{
    const size_t iterSize = 16;

    for (size_t i = 0; (i * iterSize) < fwInfo.cfgData.size(); i ++)
    {
        size_t byteOffset = i * iterSize;
        double progressRate =
            ((double(byteOffset) / double(fwInfo.cfgData.size())) * 100);
        std::cout << "Verify : " << std::fixed << std::dec
                  << std::setprecision(2) << progressRate << "% \r";

        uint8_t len = ((byteOffset + iterSize) < fwInfo.cfgData.size())
                          ? iterSize
                          : (fwInfo.cfgData.size() - byteOffset);

        auto page_data = std::vector<uint8_t>(fwInfo.cfgData.begin() + byteOffset, fwInfo.cfgData.begin() + byteOffset + len);

        if (verifySinglePage(i, page_data) < 0)
        {
            return -1;
        }
    }
    return 0;
}

int CpldLatticeManager::verifyUserCode()
{
    uint32_t userCode = 0;
    if (readUserCode(userCode) < 0)
    {
        std::cerr << "Fail to read user code." << std::endl;
        return -1;
    }

    if (userCode != fwInfo.Version)
    {
        std::cerr << "UserCode verify failed." << std::endl;
        return -1;
    }

    return 0;
}

int CpldLatticeManager::disableConfigInterface()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    int8_t ret = 0;
    // CMD_DISABLE_CONFIG_INTERFACE = 0x26,
    std::vector<uint8_t> cmd = {CMD_DISABLE_CONFIG_INTERFACE, 0x0, 0x0};

    if (isSOFTIP)
    {
        cmd.push_back(0x0);
        appendCrc16(cmd);
    }

    ret = i2cWriteReadCmd(cmd);

    if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd) < 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Disable config interface fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }
    return ret;
}

bool CpldLatticeManager::waitBusyAndVerify()
{
    int8_t ret = 0;
    uint8_t retry = 0;

    while (retry <= busyWaitmaxRetry)
    {
        uint8_t busyFlag = 0xff;

        ret = readBusyFlag(busyFlag);
        if (ret < 0)
        {
            std::cerr << "Fail to read busy flag. ret = " << unsigned(ret)
                      << std::endl;
            return false;
        }

        if (busyFlag & busyFlagBit)
        {
            std::cout << std::fixed << std::dec
                << "\rBusy Flag is still raised at try count "
                << unsigned(retry) << std::flush;
            std::this_thread::sleep_for(waitBusyTime);
            retry++;
            if (retry > busyWaitmaxRetry)
            {
                std::cout << "\nStatus Reg : Busy!" << std::endl;
                return false;
            }
        }
        else
        {
            if(retry > 0)
            {
                std::cout << std::endl;
            }
            break;
        }
    } // while loop busy check

    // Check out status reg
    uint8_t statusReg = 0xff;

    ret = readStatusReg(statusReg);
    if (ret < 0)
    {
        std::cerr << "Fail to read status register. ret = " << unsigned(ret)
                  << std::endl;
        return false;
    }

    if (((statusReg >> busyOrReadyBit) & 1) == isReady &&
        ((statusReg >> failOrOKBit) & 1) == isOK)
    {
        if (debugMode)
        {
            std::cout << "Status Reg : OK" << std::endl;
        }
    }
    else
    {
        std::cerr << "Status Reg : Fail!" << std::endl;
        return false;
    }

    return true;
}

uint8_t CpldLatticeManager::waitBusyAndVerifyCRC()
{
    uint8_t busyFlag = 0xff;
    int8_t ret = readBusyFlag(busyFlag);
    if (ret < 0)
    {
        std::cerr << "Fail to read busy flag. ret = " << unsigned(ret)
                    << std::endl;
        return -1;
    }
    /*
    if ((busyFlag & CRC_MASK) == CRC_MASK)
    {
        return false;
    }

    if ((busyFlag & BUSY_MASK) == BUSY_MASK)
    {
        return false;
    }
*/
    return (busyFlag & (CRC_MASK | BUSY_MASK));
}

int CpldLatticeManager::readBusyFlag(uint8_t& busyFlag)
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    std::vector<uint8_t> cmd = {CMD_READ_BUSY_FLAG, 0x0, 0x0, 0x0};
    constexpr size_t resSize = 1;
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if ((ret < 0) || (readData.size() != resSize))
    {
        return -1;
    }
    else
    {
        busyFlag = readData.at(0);
    }
    return 0;
}

int CpldLatticeManager::readStatusReg(uint8_t& statusReg)
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    std::vector<uint8_t> cmd = {CMD_READ_STATUS_REG, 0x0, 0x0, 0x0};
    constexpr size_t resSize = 4;
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if ((ret < 0) || (readData.size() != resSize))
    {
        return -1;
    }
    else
    {
        /*
        Read Status Register
        [LSC_READ_STATUS]
        0x3C 00 00 00 N/A YY YY YY YY Bit 1 0
        12 Busy Ready
        13 Fail OK
         */
        statusReg = readData.at(2);
    }
    return 0;
}

int CpldLatticeManager::readUserCode(uint32_t& userCode)
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    std::vector<uint8_t> cmd;
    if ((softIpVersion & 0xF0) == 0x20 || chip == "LFMXO5-25")
    {
        uint8_t targetIdx = 0x00;
        if (target == "CFG0")
        {
            targetIdx = 0x01;
        }
        else if (target == "CFG1")
        {
            targetIdx = 0x02;
        }
        cmd = {CMD_READ_FW_VERSION, 0x00, targetIdx, 0x00};
    }
    else
    {
        cmd = {CMD_READ_FW_VERSION, 0x00, 0x00, 0x00};
    }
    const auto isXO5 = chip == "LFMXO5-25";
    size_t resSize;
    std::array<uint8_t, 6> data{};
    if (isSOFTIP)
    {
        appendCrc16(cmd);
        resSize = 6;
    }
    else
    {
        resSize = isXO5 ? 5 : 4;
    }

    if (i2cWriteReadCmd(cmd, resSize, std::span{data.data(), resSize}) != 0)
    {
        return -1;
    }

    if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd, resSize, std::span{data.data(), resSize}) != 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Read CPLD FW version fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }

    userCode = isXO5 ? (uint32_t{data[4]} << 24) | (data[3] << 16) |
                           (data[2] << 8) | data[1]
                     : (isSOFTIP && (((softIpVersion & 0xF0) == 0x10))) ?
                           (uint32_t{data[3]} << 24) | (data[2] << 16) |
                           (data[1] << 8) | data[0]
                     : (uint32_t{data[0]} << 24) | (data[1] << 16) |
                           (data[2] << 8) | data[3];

    return 0;
}

int CpldLatticeManager::XO2XO3Family_update()
{
    if (readDeviceId() < 0)
    {
        return -1;
    }

    if (jedFileParser() < 0)
    {
        std::cerr << "JED file parsing failed" << std::endl;
        return -1;
    }

    if (debugMode)
    {
        if (isLCMXO3D)
        {
            std::cerr << "isLCMXO3D\n";
        }
        else
        {
            std::cerr << "is not LCMXO3D\n";
        }
    }

    std::cout << "Start to update ..." << std::endl;
    std::cout << "Enable program mode." << std::endl;

    waitBusyAndVerify();

    if (enableProgramMode() < 0)
    {
        std::cout << "Enable program mode failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Erase flash." << std::endl;
    if (eraseFlash() < 0)
    {
        std::cerr << "Erase flash failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Reset config flash for write." << std::endl;
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash for write failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Write Program Page ..." << std::endl;
    if (writeProgramPage() < 0)
    {
        std::cerr << "Write program page failed." << std::endl;
        updateFailedWarning();
        return -1;
    }
    std::cout << "Write Program Page Done." << std::endl;

    std::cout << "Program user code." << std::endl;
    if (programUserCode() < 0)
    {
        std::cerr << "Program user code failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    if (programDone() < 0)
    {
        std::cerr << "Program not done." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Reset config flash for verification." << std::endl;
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash for verification failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Verify data." << std::endl;
    if (verifyData() < 0)
    {
        std::cerr << "Verify data failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Verify user code." << std::endl;
    if (verifyUserCode() < 0)
    {
        std::cerr << "Verify user code failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "Disable config interface." << std::endl;
    if (disableConfigInterface() < 0)
    {
        std::cerr << "Disable Config Interface failed." << std::endl;
        updateFailedWarning();
        return -1;
    }

    std::cout << "\nUpdate completed! Please AC." << std::endl;

    return 0;
}

int CpldLatticeManager::XO2XO3Family_verifyOnly()
{
    if (readDeviceId() < 0)
    {
        return -1;
    }

    if (jedFileParser() < 0)
    {
        std::cerr << "JED file parsing failed" << std::endl;
        return -1;
    }

    if (debugMode)
    {
        if (isLCMXO3D)
        {
            std::cerr << "isLCMXO3D\n";
        }
        else
        {
            std::cerr << "is not LCMXO3D\n";
        }
    }

    std::cout << "Start to verify ..." << std::endl;
    std::cout << "Enable program mode." << std::endl;

    waitBusyAndVerify();

    if (enableProgramMode() < 0)
    {
        std::cout << "Enable program mode failed." << std::endl;
        return -1;
    }

    bool verifyFail = false;

    std::cout << "Reset config flash for verification." << std::endl;
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash for verification failed." << std::endl;
        verifyFail = true;
        goto cleanup;
    }

    std::cout << "Verify data." << std::endl;
    if (verifyData() < 0)
    {
        std::cerr << "Verify data failed." << std::endl;
        verifyFail = true;
        goto cleanup;
    }

    std::cout << "Verify user code." << std::endl;
    if (verifyUserCode() < 0)
    {
        std::cerr << "Verify user code failed." << std::endl;
        verifyFail = true;
        goto cleanup;
    }

cleanup:
    std::cout << "Disable config interface." << std::endl;
    if (disableConfigInterface() < 0)
    {
        std::cerr << "Disable Config Interface failed." << std::endl;
        return -1;
    }

    std::cout << "\nVerify " << (verifyFail ? "failed" : "completed") << "!."
              << std::endl;
    return (verifyFail ? -1 : 0);
}

bool XO5I2CManager::setPage(uint8_t cfg, uint8_t block, uint8_t page)
{
    std::array<uint8_t, 5> cmd{XO5_CMD_SET_PAGE, cfg, 0x0, block, page};
    return i2cWriteReadCmd(cmd) == 0;
}

bool XO5I2CManager::legacyProgramPage(std::span<const uint8_t> data)
{
    std::array<uint8_t, 1 + Cfg::PageSize> cmdBuffer{
        XO5_CMD_CFG_WRITE_PAGE,
    };

    std::span<uint8_t> cmd{cmdBuffer.data(), 1 + data.size()};
    std::copy(data.begin(), data.end(), cmd.begin() + 1);
    if (i2cWriteReadCmd(cmd) != 0)
    {
        return false;
    }
    std::this_thread::sleep_for(1ms);
    return true;
}

bool XO5I2CManager::legacyReadPage(std::span<uint8_t> data)
{
    constexpr std::array<uint8_t, 1> cmd{XO5_CMD_CFG_READ_PAGE};
    return i2cWriteReadCmd(cmd, data.size(), data) == 0;
}

bool XO5I2CManager::waitUntilReady(std::chrono::milliseconds timeout)
{
    const auto endTime = std::chrono::steady_clock::now() + timeout;
    std::array<uint8_t, 1> status{static_cast<uint8_t>(Status::NotReady)};

    while (std::chrono::steady_clock::now() < endTime)
    {
        if (i2cWriteReadCmd({}, 1, status) != 0)
        {
            std::cerr << "Status read failed\n";
            return false;
        }

        if (status[0] == static_cast<uint8_t>(Status::Ready))
        {
            return true;
        }
        std::this_thread::sleep_for(ReadyPollInterval);
    }

    std::cerr << "Timeout waiting for device ready\n";
    return false;
}

bool XO5I2CManager::programPage(uint8_t block, uint8_t page,
                                std::span<const uint8_t> data)
{
    std::array<uint8_t, 4 + Cfg::PageSize> cmdBuffer{
        static_cast<uint8_t>(Cmd::PageProgram),
        block,
        page,
        0x0,
    };

    std::span<uint8_t> cmd{cmdBuffer.data(), 4 + data.size()};
    std::copy(data.begin(), data.end(), cmd.begin() + 4);
    if (i2cWriteReadCmd(cmd) != 0)
    {
        return false;
    }

    std::this_thread::sleep_for(1ms);
    return waitUntilReady();
}

bool XO5I2CManager::readPage(uint8_t block, uint8_t page,
                             std::span<uint8_t> data)
{
    std::array<uint8_t, 4> cmd{static_cast<uint8_t>(Cmd::PageRead), block, page,
                               0x0};
    if (i2cWriteReadCmd(cmd) != 0)
    {
        return false;
    }

    std::this_thread::sleep_for(1ms);
    if (!waitUntilReady())
    {
        return false;
    }
    if (i2cWriteReadCmd({}, data.size(), data) != 0)
    {
        return false;
    }

    return data[0] == static_cast<uint8_t>(Status::Ready);
}

bool XO5I2CManager::eraseCfg()
{
    const auto startBlock = (legacyMode) ? 0 : getStartBlock(cfgIndex);
    const auto endBlock = startBlock + Cfg::BlocksPerCfg;

    auto eraseBlock = [this](uint8_t block) -> bool {
        if (legacyMode)
        {
            setPage(cfgIndex, block, 0);
            std::array<uint8_t, 2> cmd{XO5_CMD_ERASE_FLASH, XO5_ERASE_BLOCK};
            if (i2cWriteReadCmd(cmd) != 0)
            {
                return false;
            }
            std::this_thread::sleep_for(ErasePageDelay);
        }
        else
        {
            std::array<uint8_t, 4> cmd{static_cast<uint8_t>(Cmd::SectorErase),
                                       block, 0x0, 0x0};
            if (i2cWriteReadCmd(cmd) != 0)
            {
                return false;
            }
            if (!waitUntilReady())
                return false;
        }
        return true;
    };

    for (size_t block = startBlock; block < endBlock; ++block)
    {
        if (!eraseBlock(block))
        {
            std::cerr << std::format("ERASE FAILED: Block {:02X}\n", block);
            return false;
        }
    }
    return true;
}

bool XO5I2CManager::programCfg()
{
    const auto startBlock = (legacyMode) ? 0 : getStartBlock(cfgIndex);
    const auto endBlock = startBlock + Cfg::BlocksPerCfg;
    const auto& cfgData = fwInfo.cfgData;
    const auto totalBytes = cfgData.size();
    size_t bytesWritten = 0;

    for (size_t block = startBlock; block < endBlock; ++block)
    {
        if (legacyMode)
        {
            setPage(cfgIndex, block, 0);
        }

        for (size_t page = 0; page < Cfg::PagesPerBlock; ++page)
        {
            if (bytesWritten >= totalBytes)
            {
                return true;
            }

            const auto chunkSize =
                std::min(Cfg::PageSize, totalBytes - bytesWritten);
            auto chunk = std::span(cfgData).subspan(bytesWritten, chunkSize);
            const auto success = (legacyMode) ? legacyProgramPage(chunk)
                                              : programPage(block, page, chunk);
            if (!success)
            {
                std::cerr << std::format(
                    "\nPROGRAM FAILED: Block {:02X} Page {:02X}\n", block,
                    page);
                return false;
            }

            bytesWritten += chunkSize;
            updateProgress(block, page, bytesWritten, totalBytes);
        }
    }
    std::cout << std::endl;
    return true;
}

bool XO5I2CManager::verifyCfg()
{
    const auto startBlock = (legacyMode) ? 0 : getStartBlock(cfgIndex);
    const auto endBlock = startBlock + Cfg::BlocksPerCfg;
    const auto& cfgData = fwInfo.cfgData;
    const auto totalBytes = cfgData.size();
    uint8_t readBuffer[1 + Cfg::PageSize];
    size_t bytesVerified = 0;

    for (size_t block = startBlock; block < endBlock; ++block)
    {
        if (legacyMode)
        {
            setPage(cfgIndex, block, 0);
        }

        for (size_t page = 0; page < Cfg::PagesPerBlock; ++page)
        {
            if (bytesVerified >= totalBytes)
            {
                return true;
            }

            const auto chunkSize =
                std::min(Cfg::PageSize, totalBytes - bytesVerified);
            auto expected =
                std::span(cfgData).subspan(bytesVerified, chunkSize);
            auto chunk = [=, this, &readBuffer]() -> std::span<uint8_t> {
                if (legacyMode)
                {
                    auto readSpan = std::span(readBuffer).first(chunkSize);
                    return legacyReadPage(readSpan) ? readSpan
                                                    : std::span<uint8_t>{};
                }
                auto readSpan = std::span(readBuffer).first(1 + chunkSize);
                return readPage(block, page, readSpan) ? readSpan.subspan(1)
                                                       : std::span<uint8_t>{};
            }();
            if (chunk.empty())
            {
                std::cerr << std::format(
                    "\nCould not read Block {:02X} Page {:02X}\n", block, page);
                return false;
            }
            if (!std::equal(chunk.begin(), chunk.end(), expected.begin()))
            {
                std::cerr << std::format(
                    "\nVERIFY FAILED: Block {:02X} Page {:02X}\n", block, page);
                return false;
            }

            bytesVerified += chunkSize;
            updateProgress(block, page, bytesVerified, totalBytes);
        }
    }
    std::cout << std::endl;
    return true;
}

bool XO5I2CManager::programDone()
{
    std::array<uint8_t, 4> cmd{static_cast<uint8_t>(Cmd::ProgramDone), 0x0,
                               0x0, 0x0};
    if (i2cWriteReadCmd(cmd) != 0)
    {
        return false;
    }
    return true;
}

int CpldLatticeManager::XO5Family_update(bool legacy)
{
    std::cout << std::format("Starting to update {}\n", chip);

    if (target.empty())
    {
        target = "CFG0";
    }

    if (target == "SRAM")
    {
        XO5SRAMRecover recover(bus, addr, imagePath, chip, interface, target,
                               debugMode);

        return recover.fwUpdate(legacy);
    }

    XO5I2CManager i2cManager(bus, addr, imagePath, chip, interface, target,
                             debugMode, legacy);

    if (target != "CFG0" && target != "CFG1")
    {
        std::cerr << "Error: unknown target.\n";
        return -1;
    }

    if (i2cManager.jedFileParser() < 0)
    {
        std::cerr << "JED file parsing failed.\n";
        return -1;
    }

    if (!legacy && !i2cManager.ready())
    {
        std::cerr << "Error: Device not ready.\n";
        return -1;
    }

    std::cout << std::format("Erasing {} ...\n", target);
    if (!i2cManager.eraseCfg())
    {
        std::cerr << "Erase cfg data failed.\n";
        return -1;
    }

    std::cout << std::format("Programming {} ...\n", target);
    if (!i2cManager.programCfg())
    {
        std::cerr << "Program cfg data failed.\n";
        return -1;
    }

    std::cout << std::format("Verifying {} ...\n", target);
    if (!i2cManager.verifyCfg())
    {
        std::cerr << "Verify cfg data failed.\n";
        return -1;
    }

    std::cout << std::format("ProgramDone sending {}...\n", target);
    if (!i2cManager.programDone())
    {
        std::cerr << "ProgramDone failed.\n";
        return -1;
    }

    std::cout << "\nUpdate completed! Please AC.\n";

    return 0;
}

int CpldLatticeManager::XO5Familyv2_update()
{
    XO5I2CManagerv2 i2cManager(bus, addr, imagePath, chip, interface, target,
                               debugMode);

    i2cManager.isSOFTIP = CheckSOFTIP();
    i2cManager.softIpVersion = this->softIpVersion;
    std::cout << std::format("Starting to update {} with SOFTIP {}.{}\n",
                             chip,
                             i2cManager.softIpVersion >> 4,
                             i2cManager.softIpVersion & 0x0f);
    if (target.empty())
    {
        target = "CFG0";
    }

    if (target == "SRAM")
    {
        XO5SRAMRecover recover(bus, addr, imagePath, chip, interface, target,
                               debugMode);

        return recover.fwUpdate(false);
    }

    if (target != "CFG0" && target != "CFG1")
    {
        std::cerr << "Error: unknown target.\n";
        return -1;
    }

    if (i2cManager.jedFileParser() < 0)
    {
        std::cerr << "JED file parsing failed.\n";
        return -1;
    }

    std::cout << std::format("Erasing {} ...\n", target);
    if (!i2cManager.eraseCfg())
    {
        updateFailedWarning();
        std::cerr << "Erase cfg data failed.\n";
        return -1;
    }else{
        std::cout << "Erase cfg data done.\n";
    }

    std::cout << std::format("Pre program {} ...\n", target);
    if (!i2cManager.pre_program())
    {
        updateFailedWarning();
        std::cerr << "Pre program failed.\n";
        return -1;
    }else{
        std::cout << "Pre program done.\n";
    }

    std::cout << std::format("Programming {} ...\n", target);
    if (!i2cManager.programCfg())
    {
        updateFailedWarning();
        std::cerr << "Program cfg data failed.\n";
        return -1;
    }

    std::cout << std::format("Post program {} ...\n", target);
    if (!i2cManager.post_program())
    {
        updateFailedWarning();
        std::cerr << "Post program failed.\n";
        return -1;
    }

    std::cout << std::format("Verifying {} ...\n", target);
    if (!i2cManager.verifyCfg())
    {
        updateFailedWarning();
        std::cerr << "Verify cfg data failed.\n";
        return -1;
    }
    std::cout << "\nUpdate completed! Please AC.\n";

    return 0;
}

int CpldLatticeManager::XO5Familyv2_version()
{
    uint32_t userCode = 0;
    if ((softIpVersion & 0xF0) == 0x10) {
        if (target == "CFG0" || target == "CFG1")
        {
            if (resetConfigFlash() < 0)
            {
                std::cerr << "Reset config flash failed." << std::endl;
                return -1;
            }
        }

        // read user code first time
        if (readUserCode(userCode) < 0)
        {
            std::cerr << "Read usercode failed." << std::endl;
            return -1;
        }
        while (waitBusyAndVerifyCRC() & BUSY_MASK)
        {
            std::this_thread::sleep_for(5ms);
        }
        if (readUserCode(userCode) < 0)
        {
            std::cerr << "Read usercode failed." << std::endl;
            return -1;
        }
    } else {
        if (readUserCode(userCode) < 0)
        {
            std::cerr << "Read usercode failed." << std::endl;
            return -1;
        }
    }
    std::cout << "CPLD " << target << " version: 0x" << std::hex
            << std::setfill('0') << std::setw(8) << userCode << std::endl;
    return 0;
}

int CpldLatticeManager::fwUpdate(bool legacy)
{
    if (chip == "LCMXO3LF-4300" || chip == "LCMXO3LF-6900" ||
        chip == "LCMXO3D-4300" || chip == "LCMXO3D-9400") {
        return XO2XO3Family_update();
    } else if (chip == "LFMXO5-25") {
        return XO5Family_update(legacy);
    } else if (chip == "LFMXO5-65T") {
        return XO5Familyv2_update();
    } else {
        std::cerr << "Unsupported chip type: " << chip << std::endl;
        return -1;
    }
}

int CpldLatticeManager::fwVerifyOnly(bool legacy [[maybe_unused]])
{
    if (chip == "LCMXO3LF-4300" || chip == "LCMXO3LF-6900" ||
        chip == "LCMXO3D-4300" || chip == "LCMXO3D-9400") {
        return XO2XO3Family_verifyOnly();
    } else {
        std::cerr << "Unsupported chip type: " << chip << std::endl;
        return -1;
    }
}

int CpldLatticeManager::getVersion()
{
    if (CheckSOFTIP()){
        return XO5Familyv2_version();
    }

    uint32_t userCode = 0;

    if (target.empty() || chip == "LFMXO5-25")
    {
        if (readUserCode(userCode) < 0)
        {
            std::cerr << "Read usercode failed." << std::endl;
            return -1;
        }

        if (target.empty())
        {
            std::cout << std::format("CPLD version: 0x{:08x}\n", userCode);
        }
        else
        {
            std::cout << std::format("CPLD {} version: 0x{:08x}\n", target,
                                     userCode);
        }
    }
    else if (target == "CFG0" || target == "CFG1")
    {
        isLCMXO3D = true;
        waitBusyAndVerify();

        if (enableProgramMode() < 0)
        {
            std::cerr << "Enable program mode failed." << std::endl;
            return -1;
        }

        if (resetConfigFlash() < 0)
        {
            std::cerr << "Reset config flash failed." << std::endl;
            return -1;
        }

        if (readUserCode(userCode) < 0)
        {
            std::cerr << "Read usercode failed." << std::endl;
            return -1;
        }

        if (programDone() < 0)
        {
            std::cerr << "Program not done." << std::endl;
            return -1;
        }

        if (disableConfigInterface() < 0)
        {
            std::cerr << "Disable Config Interface failed." << std::endl;
            return -1;
        }

        std::cout << "CPLD " << target << " version: 0x" << std::hex
                << std::setfill('0') << std::setw(8) << userCode << std::endl;
    }
    else
    {
        std::cerr << "Error: unknown target." << std::endl;
        return -1;
    }
    return 0;
}

void CpldLatticeManager::updateFailedWarning()
{
    std::cerr << "CPLD ROM is now corrupted, do not perform power cycle before it's recovered, otherwise the slot will bricked." << std::endl;
    std::cerr << "Strong recommend to perform the update again right away." << std::endl;
}

uint16_t CpldLatticeManager::crc16_ccitt(std::span<const uint8_t> cmd)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t byte : cmd)
    {
        crc ^= static_cast<uint16_t>(byte) << 8;

        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void CpldLatticeManager::appendCrc16(std::vector<uint8_t>& cmd)
{
    uint16_t crc = crc16_ccitt(std::span<const uint8_t>(cmd.data(), cmd.size()));
    cmd.push_back(static_cast<uint8_t>(crc & 0xFF));
    cmd.push_back(static_cast<uint8_t>((crc >> 8) & 0xFF));
}

bool XO5I2CManagerv2::eraseCfg()
{
    std::cout << "Erase: Read Device ID.\n";
    if (readDeviceId() < 0)
    {
        std::cerr << "Read Device ID failed.\n";
        return false;
    }

    std::cout << "Erase: Enable Program Mode.\n";
    if (enableProgramMode() < 0)
    {
        std::cerr << "Enable program mode failed.\n";
        return false;
    }

    std::cout << "Erase: Reset config flash for write.\n";
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash failed.\n";
        return false;
    }

    std::cout << "Erase: Erase flash.\n";
    if (eraseFlash() < 0)
    {
        std::cerr << "Erase flash failed.\n";
        return false;
    }

    std::cout << "Erase: Noop.\n";
    std::vector<uint8_t> cmd{static_cast<uint8_t>(Cmd::CMD_NOOP_REG)};
    if (isSOFTIP)
    {
        appendCrc16(cmd);
    }

    if (lockI2c() < 0)
    {
        return -1;
    }

    if (i2cWriteReadCmd(cmd) != 0)
    {
        std::cerr << "Noop failed.\n";
        return false;
    }
    else if (isSOFTIP){
        uint8_t retry = 0;
        while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
        {
            std::this_thread::sleep_for(5ms);
            if (i2cWriteReadCmd(cmd) < 0)
            {
                return -1;
            }
            retry++;
        }
        if (retry >= BusyAndCRCmaxRetry)
        {
            std::cerr << "Command fail due to CRC or Busy timeout." << std::endl;
            return -1;
        }
    }

    std::cout << "Erase: Disable config interface.\n";
    if (disableConfigInterface() < 0)
    {
        std::cerr << "Disable Config Interface failed.\n";
        return false;
    }

    return true;
}

bool XO5I2CManagerv2::programCfg()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    uint8_t CRCandBusyCheck = 0;
    const auto& cfgData = fwInfo.cfgData;
    const auto totalBytes = cfgData.size();
    size_t totalPages = (totalBytes + Cfg::PageSize - 1) / Cfg::PageSize;

    size_t bytesWritten = 0;
    std::array<uint8_t, 4 + Cfg::PageSize + Cfg::CRCSize> cmdBuffer{
    static_cast<uint8_t>(Cmd::CMD_PROGRAM), 0x00, 0x00, 0x00
    };

    for (size_t page = 0; page < totalPages; ++page)
    {
        if (bytesWritten >= totalBytes)
        {
            return true;
        }

        const size_t maxWriteRetry = 10;
        size_t retry = 0;
        const auto chunkSize = std::min(Cfg::PageSize, totalBytes - bytesWritten);
        auto chunk = std::span(cfgData).subspan(bytesWritten, chunkSize);
        do{
            std::span<uint8_t> cmd{cmdBuffer.data(), 4 + chunkSize + Cfg::CRCSize};
            std::copy(chunk.begin(), chunk.end(), cmd.begin() + 4);
            uint16_t crc = crc16_ccitt(std::span<const uint8_t>(cmd.data(), 4 + chunk.size()));
            std::array<uint8_t, 2> crcBytes{
                static_cast<uint8_t>(crc & 0xFF),        // low byte
                static_cast<uint8_t>((crc >> 8) & 0xFF)  // high byte
            };
            std::copy(crcBytes.begin(), crcBytes.end(), cmd.begin() + 4 + chunk.size());

            constexpr size_t resSize = 1;
            std::vector<uint8_t> readData(resSize, 0);
            int ret = i2cWriteReadCmd(cmd, resSize, readData);
            if (page % 2 == 0)
            {
                break;
            }
            CRCandBusyCheck = waitBusyAndVerifyCRC();
            if ((ret != 0) || (readData.size() != resSize) || (readData[0] & 0x01) == 0 || (CRCandBusyCheck & CRC_MASK))
            {
                ++retry;
                if (debugMode)
                {
                    std::cerr << std::format(
                        "Retry PROGRAM: page={}, retry={}, ret={}, status={}\n",
                        page, retry, ret,
                        (readData.size() ? int(readData[0]) : -1));
                }
            }
            else
            {
                while ((CRCandBusyCheck & BUSY_MASK))
                {
                    CRCandBusyCheck = waitBusyAndVerifyCRC();
                }
                if (debugMode)
                {
                    std::cout << std::format(
                        "PROGRAM: page={},ret={}, status={}\n",
                        page, ret,
                        (readData.size() ? int(readData[0]) : -1));
                }
                break;  // Success
            }
        }while(retry < maxWriteRetry);

        if (retry >= maxWriteRetry)
        {
                std::cerr << std::format(
                "\nPROGRAM FAILED: Page {:02X}\n", page);
                return false;
        }

        bytesWritten += chunk.size();
        updateProgress(page, bytesWritten, totalBytes);
    }
    return true;
}

bool XO5I2CManagerv2::verifyCfg()
{
    std::cout << "Verify: Reset calculate hash.\n";
    if (reset_calculate_hash() < 0)
    {
        std::cerr << "Reset calculate hash failed.\n";
        return false;
    }

    std::cout << "Verify: Read fw hash.\n";
    if (read_fw_hash() < 0)
    {
        std::cerr << "Verify read fw hash failed." << std::endl;
        return false;
    }

    return true;
}

bool XO5I2CManagerv2::pre_program()
{
    std::cout << "pre_program: Enable Program Mode.\n";
    if (enableProgramMode() < 0)
    {
        std::cerr << "Enable program mode failed.\n";
        return false;
    }

    std::cout << "pre_program: Reset config flash for write.\n";
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash failed.\n";
        return false;
    }

    std::cout << "pre_program: Reset calculate hash.\n";
    if (reset_calculate_hash() < 0)
    {
        std::cerr << "Reset calulate hash failed.\n";
        return false;
    }

    return true;
}

bool XO5I2CManagerv2::post_program()
{
    std::cout << "post_program: Enable Program Mode.\n";
    if (enableProgramMode() < 0)
    {
        std::cerr << "Enable program mode failed.\n";
        return false;
    }

    std::cout << "post_program: Reset config flash for write.\n";
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash failed.\n";
        return false;
    }

    std::cout << "post_program: Chcek program done.\n";
    if (programDone() < 0)
    {
        std::cerr << "Program not done." << std::endl;
        return false;
    }

    std::cout << "post_program: Disable config interface.\n";
    if (disableConfigInterface() < 0)
    {
        std::cerr << "Disable Config Interface failed.\n";
        return false;
    }

    return true;
}

int XO5I2CManagerv2::reset_calculate_hash()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    std::vector<uint8_t> cmd{static_cast<uint8_t>(Cmd::CMD_RST_CAL_HASH), 0x0, 0x0, 0x0};

    appendCrc16(cmd);

    int ret = i2cWriteReadCmd(cmd);
    uint8_t retry = 0;
    while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
    {
        std::this_thread::sleep_for(5ms);
        if (i2cWriteReadCmd(cmd) < 0)
        {
            return -1;
        }
        retry++;
    }
    if (retry >= BusyAndCRCmaxRetry)
    {
        std::cerr << "Reset calculate hash due to CRC or Busy timeout." << std::endl;
        return -1;
    }
    return ret;
}

int XO5I2CManagerv2::read_fw_hash()
{
    if (lockI2c() < 0)
    {
        return -1;
    }
    std::vector<uint8_t> cmd{static_cast<uint8_t>(Cmd::CMD_READ_FW_HASH), 0x0, 0x0, 0x0};

    appendCrc16(cmd);
    size_t resSize = 50;
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if (ret < 0)
    {
        std::cout << "Fail to read device Id." << std::endl;
        return -1;
    }

    uint8_t retry = 0;
    while ((waitBusyAndVerifyCRC() & CRC_MASK) && retry < BusyAndCRCmaxRetry)
    {
        std::this_thread::sleep_for(5ms);
        if (i2cWriteReadCmd(cmd, resSize, readData) < 0)
        {
            return -1;
        }
        retry++;
    }
    if (retry >= BusyAndCRCmaxRetry)
    {
        std::cerr << "Read CPLD image hash due to CRC or Busy timeout." << std::endl;
        return -1;
    }

    std::span<const uint8_t> readHash(readData.data(), SHA384_DIGEST_LENGTH);
    std::cout << "Read FW hash: ";
    for (uint8_t b : readHash)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(b);
    }

    std::vector<uint8_t> imagehash(SHA384_DIGEST_LENGTH, 0);
    SHA384(fwInfo.cfgData.data(), fwInfo.cfgData.size(), imagehash.data());
    std::cout << "\nFW hash: ";
    for (uint8_t b : imagehash)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(b);
    }
    std::cout << std::endl;
    if (!std::equal(readHash.begin(), readHash.end(), imagehash.begin()))
    {
        std::cerr << "FW hash verify failed." << std::endl;
        return -1;
    }
    else
    {
        std::cout << "FW hash verify success." << std::endl;
    }
    return 0;
}

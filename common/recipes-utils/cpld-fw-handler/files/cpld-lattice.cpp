#include "cpld-lattice.hpp"

#include <fstream>
#include <map>
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
};

int CpldLatticeManager::jedFileParser()
{
    bool cfStart = false;
    bool ufmStart = false; // for isLCMXO3D
    bool ufmPrepare = false;
    bool versionStart = false;
    bool checksumStart = false;
    bool ebrInitDataStart = false;
    int numberSize = 0;

    std::string line;
    std::ifstream ifs(imagePath, std::ifstream::in);
    if (!ifs.good())
    {
        std::cerr << "Failed to open JED file" << std::endl;
        return -1;
    }

    // Parsing JED file
    while (getline(ifs, line))
    {
        if (line.rfind(TAG_QF, 0) == 0)
        {
            numberSize = line.find("*") - line.find("F") - 1;
            if (numberSize <= 0)
            {
                std::cerr << "Error in parsing QF tag" << std::endl;
                ifs.close();
                return -1;
            }
            static constexpr auto start = std::strlen(TAG_QF);
            fwInfo.QF = std::stoul(line.substr(start, numberSize));

            std::cout << "QF Size = " << fwInfo.QF << std::endl;
        }
        else if (line.rfind(TAG_CF_START, 0) == 0)
        {
            cfStart = true;
        }
#ifdef ENABLE_UPDATE_EBR_INIT
        else if (line.rfind(TAG_EBR_INIT_DATA, 0) == 0)
        {
            ebrInitDataStart = true;
        }
#endif
        else if (ufmPrepare == true)
        {
            ufmPrepare = false;
            ufmStart = true;
            continue;
        }
        else if (line.rfind(TAG_USERCODE, 0) == 0)
        {
            versionStart = true;
        }
        else if (line.rfind(TAG_CHECKSUM, 0) == 0)
        {
            checksumStart = true;
        }

        if (line.rfind("NOTE DEVICE NAME:", 0) == 0)
        {
            std::cerr << line << "\n";
            if (line.find(chip) != std::string::npos)
            {
                std::cout
                    << "[OK] The image device name match with chip name\n";
            }
            else
            {
                std::cerr << "STOP UPDATEING: The image not match with chip.\n";
                return -1;
            }
        }

        if (cfStart == true)
        {
            // L000
            if ((line.rfind(TAG_CF_START, 0)) && (line.size() != 1))
            {
                if ((line.rfind("0", 0) == 0) || (line.rfind("1", 0) == 0))
                {
                    while (line.size())
                    {
                        auto binary_str = line.substr(0, 8);
                        try
                        {
                            fwInfo.cfgData.push_back(
                                std::stoi(binary_str, 0, 2));
                            line.erase(0, 8);
                        }
                        catch (const std::invalid_argument& error)
                        {
                            break;
                        }
                        catch (...)
                        {
                            std::cerr << "Error while parsing CF section"
                                      << std::endl;
                            return -1;
                        }
                    }
                }
                else
                {
                    std::cerr << "CF Size = " << fwInfo.cfgData.size()
                              << std::endl;
                    cfStart = false;
                    if (ebrInitDataStart == false)
                    {
                        ufmPrepare = true;
                    }
                }
            }
        }
#ifdef ENABLE_UPDATE_EBR_INIT
        else if (ebrInitDataStart == true)
        {
            // NOTE EBR_INIT DATA
            if ((line.rfind(TAG_EBR_INIT_DATA, 0)) && (line.size() != 1))
            {
                if ((line.rfind("L", 0)) && (line.size() != 1))
                {
                    if ((line.rfind("0", 0) == 0) || (line.rfind("1", 0) == 0))
                    {
                        while (line.size())
                        {
                            auto binary_str = line.substr(0, 8);
                            try
                            {
                                fwInfo.cfgData.push_back(
                                    std::stoi(binary_str, 0, 2));
                                line.erase(0, 8);
                            }
                            catch (const std::invalid_argument& error)
                            {
                                break;
                            }
                            catch (...)
                            {
                                std::cerr << "Error while parsing CF section"
                                        << std::endl;
                                return -1;
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "CF Size with ERB_INIT Data = " << fwInfo.cfgData.size()
                                << std::endl;
                        ebrInitDataStart = false;
                        ufmPrepare = true;
                    }
                }
            }
        }
#endif
        else if ((checksumStart == true) && (line.size() != 1))
        {
            checksumStart = false;
            numberSize = line.find("*") - line.find("C") - 1;
            if (numberSize <= 0)
            {
                std::cerr << "Error in parsing checksum" << std::endl;
                ifs.close();
                return -1;
            }
            static constexpr auto start = std::strlen(TAG_CHECKSUM);
            std::istringstream iss(line.substr(start, numberSize));
            iss >> std::hex >> fwInfo.CheckSum;

            std::cout << "Checksum = 0x" << std::hex << fwInfo.CheckSum
                      << std::endl;
        }
        else if (versionStart == true)
        {
            if ((line.rfind(TAG_USERCODE, 0)) && (line.size() != 1))
            {
                versionStart = false;

                if (line.rfind(TAG_UH, 0) == 0)
                {
                    numberSize = line.find("*") - line.find("H") - 1;
                    if (numberSize <= 0)
                    {
                        std::cerr << "Error in parsing version" << std::endl;
                        ifs.close();
                        return -1;
                    }
                    static constexpr auto start = std::strlen(TAG_UH);
                    std::istringstream iss(line.substr(start, numberSize));
                    iss >> std::hex >> fwInfo.Version;

                    std::cout << "UserCode = 0x" << std::hex << fwInfo.Version
                              << std::endl;
                }
            }
        }
        else if (ufmStart == true)
        {
            if ((line.rfind("L", 0)) && (line.size() != 1))
            {
                if ((line.rfind("0", 0) == 0) || (line.rfind("1", 0) == 0))
                {
                    while (line.size())
                    {
                        auto binary_str = line.substr(0, 8);
                        try
                        {
                            fwInfo.ufmData.push_back(
                                std::stoi(binary_str, 0, 2));
                            line.erase(0, 8);
                        }
                        catch (const std::invalid_argument& error)
                        {
                            break;
                        }
                        catch (...)
                        {
                            std::cerr << "Error while parsing UFM section"
                                      << std::endl;
                            return -1;
                        }
                    }
                }
                else
                {
                    std::cout << "UFM size = " << fwInfo.ufmData.size()
                              << std::endl;
                    ufmStart = false;
                }
            }
        }
    }

    // Compute check sum
    unsigned int jedFileCheckSum = 0;
    for (unsigned i = 0; i < fwInfo.cfgData.size(); i++)
    {
        jedFileCheckSum += reverse_bit(fwInfo.cfgData.at(i));
    }
    for (unsigned i = 0; i < fwInfo.ufmData.size(); i++)
    {
        jedFileCheckSum += reverse_bit(fwInfo.ufmData.at(i));
    }
    std::cout << "jedFileCheckSum = " << jedFileCheckSum << "\n";
    jedFileCheckSum = jedFileCheckSum & 0xffff;

    if ((fwInfo.CheckSum != jedFileCheckSum) || (fwInfo.CheckSum == 0))
    {
        std::cerr << "CPLD JED File CheckSum Error - " << std::hex
                  << jedFileCheckSum << std::endl;
        ifs.close();
        return -1;
    }
    std::cout << "JED File CheckSum = 0x" << std::hex << jedFileCheckSum
              << std::endl;

    ifs.close();
    return 0;
}

int CpldLatticeManager::readDeviceId()
{
    // 0xE0
    std::vector<uint8_t> cmd = {CMD_READ_DEVICE_ID, 0x0, 0x0, 0x0};
    constexpr size_t resSize = 4;
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if (ret < 0)
    {
        std::cout << "Fail to read device Id." << std::endl;
        return -1;
    }

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

int CpldLatticeManager::enableProgramMode()
{
    // 0x74 transparent mode
    std::vector<uint8_t> cmd = {CMD_ENABLE_CONFIG_MODE, 0x08, 0x0, 0x0};
    std::vector<uint8_t> read;

    if (i2cWriteReadCmd(cmd, 0, read) < 0)
    {
        return -1;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }
    std::this_thread::sleep_for(waitBusyTime);
    return 0;
}

int CpldLatticeManager::eraseFlash()
{
    std::vector<uint8_t> cmd;
    std::vector<uint8_t> read;

    if (isLCMXO3D)
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
    }
    else
    {
        cmd = {CMD_ERASE_FLASH, 0xC, 0x0, 0x0};
    }

    int ret = i2cWriteReadCmd(cmd, 0, read);
    if (ret < 0)
    {
        return ret;
    }

    if (!waitBusyAndVerify())
    {
        std::cerr << "Wait busy and verify fail" << std::endl;
        return -1;
    }
    std::this_thread::sleep_for(waitBusyTime);
    return 0;
}

int CpldLatticeManager::resetConfigFlash()
{
    // CMD_RESET_CONFIG_FLASH = 0x46

    std::vector<uint8_t> cmd;
    if (isLCMXO3D)
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
    }
    else
    {
        cmd = {CMD_RESET_CONFIG_FLASH, 0x0, 0x0, 0x0};
    }
    std::vector<uint8_t> read;
    return i2cWriteReadCmd(cmd, 0, read);
}

int CpldLatticeManager::writeProgramPage()
{
    /*
    CMD_PROGRAM_PAGE = 0x70,

    Program one NVCM/Flash page. Can be
    used to program the NVCM0/CFG or
    NVCM1/UFM.

    */
    std::vector<uint8_t> cmd = {CMD_PROGRAM_PAGE, 0x0, 0x0, 0x01};
    std::vector<uint8_t> read;
    size_t iterSize = 16;

    for (size_t i = 0; i < fwInfo.cfgData.size(); i += iterSize)
    {
        double progressRate =
            ((double(i) / double(fwInfo.cfgData.size())) * 100);
        std::cout << "Update :" << std::fixed << std::dec
                  << std::setprecision(2) << progressRate << "% \r";

        uint8_t len = ((i + iterSize) < fwInfo.cfgData.size())
                          ? iterSize
                          : (fwInfo.cfgData.size() - i);
        std::vector<uint8_t> data = cmd;

        data.insert(data.end(), fwInfo.cfgData.begin() + i,
                    fwInfo.cfgData.begin() + i + len);

        if (i2cWriteReadCmd(data, 0, read) < 0)
        {
            return -1;
        }

        /*
         Reference spec
         Important! If don't sleep, it will take a long time to update.
        */
        usleep(200);

        if (!waitBusyAndVerify())
        {
            std::cerr << "Wait busy and verify fail" << std::endl;
            return -1;
        }

        data.clear();
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
    std::vector<uint8_t> read;
    for (int i = 3; i >= 0; i--)
    {
        cmd.push_back((fwInfo.Version >> (i * 8)) & 0xFF);
    }

    if (i2cWriteReadCmd(cmd, 0, read) < 0)
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
    // CMD_PROGRAM_DONE = 0x5E
    std::vector<uint8_t> cmd = {CMD_PROGRAM_DONE, 0x0, 0x0, 0x0};
    std::vector<uint8_t> read;

    if (i2cWriteReadCmd(cmd, 0, read) < 0)
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

int CpldLatticeManager::disableConfigInterface()
{
    // CMD_DISABLE_CONFIG_INTERFACE = 0x26,
    std::vector<uint8_t> cmd = {CMD_DISABLE_CONFIG_INTERFACE, 0x0, 0x0};
    std::vector<uint8_t> read;
    return i2cWriteReadCmd(cmd, 0, read);
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
            std::this_thread::sleep_for(waitBusyTime);
            retry++;
            if (retry > busyWaitmaxRetry)
            {
                std::cout << "Status Reg : Busy!" << std::endl;
                return false;
            }
        }
        else
        {
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

int CpldLatticeManager::readBusyFlag(uint8_t& busyFlag)
{
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
    std::vector<uint8_t> cmd = {CMD_READ_FW_VERSION, 0x0, 0x0, 0x0};
    constexpr size_t resSize = 4;
    std::vector<uint8_t> readData(resSize, 0);

    int ret = i2cWriteReadCmd(cmd, resSize, readData);
    if (ret < 0)
    {
        return -1;
    }

    for (size_t i = 0; i < resSize; i++)    
    {
        userCode |= readData.at(i) << ((3 - i) * 8);
    }
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

    std::cout << "Starts to update ..." << std::endl;
    std::cout << "Enable program mode." << std::endl;

    waitBusyAndVerify();

    if (enableProgramMode() < 0)
    {
        std::cout << "Enable program mode failed." << std::endl;
        return -1;
    }

    std::cout << "Erase flash." << std::endl;
    if (eraseFlash() < 0)
    {
        std::cerr << "Erase flash failed." << std::endl;
        return -1;
    }

    std::cout << "Reset config flash." << std::endl;
    if (resetConfigFlash() < 0)
    {
        std::cerr << "Reset config flash failed." << std::endl;
        return -1;
    }

    std::cout << "Write Program Page ..." << std::endl;
    if (writeProgramPage() < 0)
    {
        std::cerr << "Write program page failed." << std::endl;
        return -1;
    }
    std::cout << "Write Program Page Done." << std::endl;

    std::cout << "Program user code." << std::endl;
    if (programUserCode() < 0)
    {
        std::cerr << "Program user code failed." << std::endl;
        return -1;
    }

    if (programDone() < 0)
    {
        std::cerr << "Program not done." << std::endl;
        return -1;
    }

    std::cout << "Disable config interface." << std::endl;
    if (disableConfigInterface() < 0)
    {
        std::cerr << "Disable Config Interface failed." << std::endl;
        return -1;
    }

    std::cout << "\nUpdate completed! Please AC." << std::endl;

    return 0;
}

int XO5I2CManager::set_page(uint8_t partition_num, uint32_t page_num)
{
      std::vector<uint8_t> cmd = {XO5_CMD_SET_PAGE, partition_num,
                                  static_cast<uint8_t>((page_num >> 16) & 0xFF),
                                  static_cast<uint8_t>((page_num >> 8) & 0xFF),
                                  static_cast<uint8_t>(page_num & 0xFF)};
      std::vector<uint8_t> read;

      return i2cWriteReadCmd(cmd, 0, read);
}

int XO5I2CManager::erase_flash(uint8_t size)
{
    std::vector<uint8_t> cmd = {XO5_CMD_ERASE_FLASH, size};
    std::vector<uint8_t> read;
    return i2cWriteReadCmd(cmd, 0, read);
}

int XO5I2CManager::cfg_reset_addr()
{
    std::vector<uint8_t> cmd = {XO5_CMD_CFG_RESET_ADDR};
    std::vector<uint8_t> read;
    return i2cWriteReadCmd(cmd, 0, read);
}

std::vector<uint8_t> XO5I2CManager::cfg_read_page()
{
    std::vector<uint8_t> cmd = {XO5_CMD_CFG_READ_PAGE};
    std::vector<uint8_t> read(256);

    if (i2cWriteReadCmd(cmd, 256, read) < 0) {
        return {};
    } else {
        return read;
    }
}

int XO5I2CManager::cfg_write_page(const std::vector<uint8_t>& byte_list)
{
    std::vector<uint8_t> cmd = {XO5_CMD_CFG_WRITE_PAGE};
    cmd.insert(cmd.end(), byte_list.begin(), byte_list.end());
    std::vector<uint8_t> read;
    return i2cWriteReadCmd(cmd, 0, read);
}

bool XO5I2CManager::programCfgData(uint8_t cfg)
{
	unsigned int data_offset = 0;

	for (int b_idx = 0; b_idx < XO5_CFG_BLOCK_NUM; ++b_idx) {
		set_page(cfg, b_idx * XO5_PAGE_NUM);
		erase_flash(XO5_ERASE_BLOCK);

		for (int p_idx = 0; p_idx < XO5_PAGE_NUM; ++p_idx) {
			if (data_offset < fwInfo.cfgData.size()) {
				int remaining_bytes = fwInfo.cfgData.size() - data_offset;
				int bytes_to_write = std::min(static_cast<int>(XO5_PAGE_SIZE), remaining_bytes);
				std::vector<uint8_t> wr_buffer(fwInfo.cfgData.begin() + data_offset,
								fwInfo.cfgData.begin() + data_offset + bytes_to_write);

				if (cfg_write_page(wr_buffer)) {
						std::cerr << "Failed to write page " << p_idx <<
						" of block " << b_idx << std::endl;
						return false;
				}

				data_offset += bytes_to_write;

				float progressRate = (static_cast<float>(data_offset) / fwInfo.cfgData.size()) * 100;
				std::cout << "Update :" << std::fixed << std::dec <<
						std::setprecision(2) << progressRate << "% \r";

				usleep(200);
			} else {
				return true;
			}
		}
	}
	return true;
}

bool XO5I2CManager::verifyCfgData(uint8_t cfg)
{
	unsigned int data_offset = 0;

	for (int b_idx = 0; b_idx < XO5_CFG_BLOCK_NUM; ++b_idx) {
		set_page(cfg, b_idx * XO5_PAGE_NUM);

		for (int p_idx = 0; p_idx < XO5_PAGE_NUM; ++p_idx) {
			if (data_offset < fwInfo.cfgData.size()) {
				std::vector<uint8_t> read_buffer = cfg_read_page();

				if (read_buffer.empty()) {
					std::cerr << "Failed to read page " << p_idx <<
										" of block " << b_idx << std::endl;
					return false;
				}

				int remaining_bytes = fwInfo.cfgData.size() - data_offset;
				int bytes_to_compare = std::min(static_cast<int>(XO5_PAGE_SIZE), remaining_bytes);

				if (!std::equal(read_buffer.begin(), read_buffer.begin() + bytes_to_compare,
												fwInfo.cfgData.begin() + data_offset)) {
					std::cerr << "Verification failed at page " << p_idx <<
										" of block " << b_idx << std::endl;
					return false;
				}
				data_offset += bytes_to_compare;
				} else {
					return true;
				}
			float progressRate = (static_cast<float>(data_offset) / fwInfo.cfgData.size()) * 100;
					std::cout << "Verify :" << std::fixed << std::dec <<
								std::setprecision(2) << progressRate << "% \r";
			usleep(200);
		}
	}

	std::cout << "Verification successful." << std::endl;
	return true;
}


int XO5I2CManager::XO5jedFileParser()
{
    bool cfStart = false;
    bool checksumStart = false;
    bool endofCF = false;
    int numberSize = 0;

    std::string line;
    std::ifstream ifs(imagePath, std::ifstream::in);
    if (!ifs.good())
    {
        std::cerr << "Failed to open JED file" << std::endl;
        return -1;
    }

    // Parsing JED file
    while (getline(ifs, line))
    {
        if (line.rfind(TAG_QF, 0) == 0)
        {
            numberSize = line.find("*") - line.find("F") - 1;
            if (numberSize <= 0)
            {
                std::cerr << "Error in parsing QF tag" << std::endl;
                ifs.close();
                return -1;
            }
            static constexpr auto start = std::strlen(TAG_QF);
            fwInfo.QF = std::stoul(line.substr(start, numberSize));

            std::cout << "QF Size = " << fwInfo.QF << std::endl;
        }
        else if (line.rfind(TAG_CF_START, 0) == 0)
        {
            cfStart = true;
        }
        else if (line.rfind(TAG_CF_END, 0) == 0)
        {
            endofCF = true;
            cfStart = false;
        }
        else if (line.rfind(TAG_CHECKSUM, 0) == 0)
        {
            checksumStart = true;
        }

        if (line.rfind("NOTE DEVICE NAME:", 0) == 0)
        {
            std::cerr << line << "\n";
            if (line.find(chip) != std::string::npos)
            {
                std::cout
                    << "[OK] The image device name match with chip name\n";
            }
            else
            {
                std::cerr << "STOP UPDATEING: The image not match with chip.\n";
                return -1;
            }
        }

        if (cfStart == true)
        {
            if ((line.rfind(TAG_CF_START, 0)) && (line.size() != 1))
            {
                if ((line.rfind("0", 0) == 0) || (line.rfind("1", 0) == 0))
                {
                    while (line.size())
                    {
                        auto binary_str = line.substr(0, 8);
                        try
                        {
                            fwInfo.cfgData.push_back(
                                std::stoi(binary_str, 0, 2));
                            line.erase(0, 8);
                        }
                        catch (const std::invalid_argument& error)
                        {
                            break;
                        }
                        catch (...)
                        {
                            std::cerr << "Error while parsing CF section"
                                      << std::endl;
                            return -1;
                        }
                    }
                }
                else {
                    continue;
                }
            }
        }
        else if (endofCF == true) {
            std::cerr << "CF Size = " << fwInfo.cfgData.size()
                              << std::endl;
            endofCF = false;
        }
        else if ((checksumStart == true) && (line.size() != 1))
        {
            checksumStart = false;
            numberSize = line.find("*") - line.find("C") - 1;
            if (numberSize <= 0)
            {
                std::cerr << "Error in parsing checksum" << std::endl;
                ifs.close();
                return -1;
            }
            static constexpr auto start = std::strlen(TAG_CHECKSUM);
            std::istringstream iss(line.substr(start, numberSize));
            iss >> std::hex >> fwInfo.CheckSum;

            std::cout << "Checksum = 0x" << std::hex << fwInfo.CheckSum
                      << std::endl;
        }
    }
    // Compute check sum
    unsigned int jedFileCheckSum = 0;
    for (unsigned i = 0; i < fwInfo.cfgData.size(); i++)
    {
        jedFileCheckSum += reverse_bit(fwInfo.cfgData.at(i));
    }
    for (unsigned i = 0; i < fwInfo.ufmData.size(); i++)
    {
        jedFileCheckSum += reverse_bit(fwInfo.ufmData.at(i));
    }
    std::cout << "jedFileCheckSum = " << jedFileCheckSum << "\n";
    jedFileCheckSum = jedFileCheckSum & 0xffff;

    if ((fwInfo.CheckSum != jedFileCheckSum) || (fwInfo.CheckSum == 0))
    {
        std::cerr << "CPLD JED File CheckSum Error - " << std::hex
                  << jedFileCheckSum << std::endl;
        ifs.close();
        return -1;
    } else {
        std::cout << "[OK] JED File Checksum compare success" << std::endl;
    }

    ifs.close();
    return 0;
}

int CpldLatticeManager::XO5Family_update()
{
    uint8_t operate_taget;
    XO5I2CManager i2cManager(bus, addr, imagePath, chip, interface, target, debugMode);

    std::cout << "Starting to update " << chip << std::endl;

    if (target.empty() || target == "CFG0")
    {
        operate_taget = i2cManager.XO5_PARTITION_CFG0;

    } else if (target == "CFG1")
    {
        operate_taget = i2cManager.XO5_PARTITION_CFG1;
    } else
    {
        std::cerr << "Error: unknown target." << std::endl;
        return -1;
    }

    if (i2cManager.XO5jedFileParser() < 0)
    {
        std::cerr << "JED file parsing failed" << std::endl;
        return -1;
    }

    std::cout << "Program CFG" << (operate_taget == i2cManager.XO5_PARTITION_CFG0 ? "0" : "1")
        << " data ..." << std::endl;
    if (i2cManager.programCfgData(operate_taget) == false)
    {
        std::cerr << "Program cfg data failed." << std::endl;
        return -1;
    }

    std::cout << "Verify CFG data ..." << std::endl;
    if (i2cManager.verifyCfgData(operate_taget) == false)
    {
        std::cerr << "Verify cfg data failed." << std::endl;
        return -1;
    }

    std::cout << "\nUpdate completed! Please AC." << std::endl;

    return 0;
}

int CpldLatticeManager::fwUpdate()
{
    if (chip == "LCMXO3LF-4300" || chip == "LCMXO3LF-6900" ||
        chip == "LCMXO3D-4300" || chip == "LCMXO3D-9400") {
        return XO2XO3Family_update();
    } else if (chip == "LFMXO5-25") {
        return XO5Family_update();
    } else {
        std::cerr << "Unsupported chip type: " << chip << std::endl;
        return -1;
    }
}

int CpldLatticeManager::getVersion()
{
    uint32_t userCode = 0;

    if (target.empty())
    {
        if (readUserCode(userCode) < 0)
        {
            std::cerr << "Read usercode failed." << std::endl;
            return -1;
        }

        std::cout << "CPLD version: 0x" << std::hex << std::setfill('0')
                    << std::setw(8) << userCode << std::endl;
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

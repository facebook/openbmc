#include "xo5_sram_recover.hpp"
#include "xo5/xo5_bit_file_frame_parser.hpp"

#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <span>
#include <thread>
#include <chrono>
#include <sstream>

namespace
{

std::string vecToHexStr(const std::vector<uint8_t>& v)
{
    std::ostringstream oss;
    oss << std::hex << std::uppercase << std::setfill('0');

    for (size_t i = 0; i < v.size(); ++i)
    {
        oss << std::setw(2)
            << static_cast<unsigned int>(v[i]);

        if (i + 1 < v.size())
        {
            oss << ' ';
        }
    }

    oss << std::dec;

    return oss.str();
}

void printProgress(size_t current, size_t total)
{
    if (total == 0)
    {
        return;
    }

    double progressRate =
        static_cast<double>(current) * 100.0 / static_cast<double>(total);

    std::cout << std::fixed << std::dec
              << std::setprecision(2)
              << "Progress: " << progressRate << "%\r"
              << std::flush;
}

} // namespace

bool XO5SRAMRecover::parseBitFile()
{
    XO5BitFileFrameParser parser;
    frameData = parser.parseFile(imagePath);

    if (frameData.controlReg0Cmd.empty() ||
        frameData.usercodeCmd.empty() ||
        frameData.mibFrames.empty() ||
        frameData.dataFrames.empty())
    {
        std::cerr << "ERROR: invalid bitstream content\n";
        return false;
    }

    std::cout << "Parsed Control Register Cmd: "
                << vecToHexStr(frameData.controlReg0Cmd) << "\n";
    
    std::cout << "Parsed User Code Cmd: "
                << vecToHexStr(frameData.usercodeCmd) << "\n";

    std::cout << "Parsed " << frameData.mibFrames.size()
              << " MIB frames\n";
    
    std::cout << "Parsed " << frameData.dataFrames.size()
              << " Data frames\n";

    std::cout << "Parsed " << frameData.initBusFrames.size()
              << " Init BUS frames\n";

    return true;
}

bool XO5SRAMRecover::enterProgrammingMode()
{
    std::vector<uint8_t> activationKeyCmd = {0xFF, 0xA4, 0xC6, 0xF4, 0x8A};
    std::vector<uint8_t> deviceIdCmd = {0xE0, 0x00, 0x00, 0x00};
    std::vector<uint8_t> iscEnableCmd = {0xC6, 0x00, 0x00, 0x00};

    std::cout << "Sending activation key: "
              << vecToHexStr(activationKeyCmd) << "\n";

    if (i2cWriteReadCmd(activationKeyCmd) != 0)
    {
        std::cerr << "Failed to send key activation request\n";
        return false;
    }

    std::vector<uint8_t> deviceId(4);
    if (i2cWriteReadCmd(deviceIdCmd, deviceId.size(), deviceId) != 0)
    {
        std::cerr << "Failed to read device ID\n";
        return false;
    }
    std::cout << "Device ID Read: " << vecToHexStr(deviceId) << "\n";
    
    std::cout << "Entering Programming Mode\n";

    if (i2cWriteReadCmd(iscEnableCmd) != 0)
    {
        std::cerr << "Failed to send ISC enable request\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    readStatusRegister();

    return true;
}

std::vector<uint8_t> XO5SRAMRecover::readStatusRegister()
{
    std::vector<uint8_t> cmd = {0x3C, 0x00, 0x00, 0x00};
    std::vector<uint8_t> status(8);

    if (i2cWriteReadCmd(cmd, status.size(), status) != 0)
    {
        std::cerr << "Failed to read status register\n";
        return {};
    }

    std::cout << "Status Register Read: " << vecToHexStr(status) << "\n";

    return status;
}

bool XO5SRAMRecover::eraseSRAM()
{
    std::cout << "Erasing SRAM....\n";

    std::vector<uint8_t> eraseCmd = {0x0E, 0x00, 0x00, 0x00};

    if (i2cWriteReadCmd(eraseCmd) != 0)
    {
        std::cerr << "Failed to send erase request\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "Erased SRAM\n";

    readStatusRegister();

    return true;
}

bool XO5SRAMRecover::programCtrlRegister()
{
    if (i2cWriteReadCmd(frameData.controlReg0Cmd) != 0)
    {
        std::cerr << "Failed to program control register 0 by command: "
                  << vecToHexStr(frameData.controlReg0Cmd);
        return false;
    }

    std::this_thread::sleep_for(std::chrono::microseconds(100));;

    std::cout << "Ctrl register programmed\n";

    return true;
}

bool XO5SRAMRecover::programMIBFrames()
{
    std::cout << "Programming MIB Frames...\n";

    if (frameData.mibFrames.empty())
    {
        std::cerr << "No MIB frames found to program\n";
        return false;
    }

    std::vector<uint8_t> setAddrCmd = {0xB4, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x80, 0x00};
    std::vector<uint8_t> progCmd = {0x82, 0x21, 0x00, 0x00};

    if (i2cWriteReadCmd(setAddrCmd) != 0)
    {
        std::cerr << "Faild to set address, cmd = " << vecToHexStr(setAddrCmd)
                  << "\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::microseconds(1000));

    for (size_t i = 0; i < frameData.mibFrames.size(); ++i)
    {
        const auto& frame = frameData.mibFrames[i];
        std::vector<uint8_t> cmd(progCmd);
        cmd.insert(cmd.end(), frame.data.begin(), frame.data.end());

        if (i2cWriteReadCmd(cmd) != 0)
        {
            std::cerr << "Failed to program MIB frame " << i << "\n";
            return false;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        printProgress(i + 1, frameData.mibFrames.size());
    }

    std::cout << "Programmed " << frameData.mibFrames.size() << " MIB frames\n";
    return true;
}

bool XO5SRAMRecover::programDataFrames()
{
    std::cout << "Programming Data Frames...\n";

    if (frameData.dataFrames.empty())
    {
        std::cerr << "No Data frames found to program\n";
        return false;
    }

    std::vector<uint8_t> initAddrCmd = {0x46, 0x00, 0x00, 0x00};
    std::vector<uint8_t> progCmd = {0x82, 0x21, 0x00, 0x00};

    if (i2cWriteReadCmd(initAddrCmd) != 0)
    {
        std::cerr << "Failed to send init address command: "
                  << vecToHexStr(initAddrCmd) << "\n";
        return false;
    }

    for (size_t i = 0; i < frameData.dataFrames.size(); ++i)
    {
        const auto& frame = frameData.dataFrames[i];
        std::vector<uint8_t> cmd(progCmd);
        cmd.insert(cmd.end(), frame.data.begin(), frame.data.end());
        if (i2cWriteReadCmd(cmd) != 0)
        {
            std::cerr << "Failed to program Data frame " << i << "\n";
            return false;
        }

        std::this_thread::sleep_for(std::chrono::microseconds(1000));
        printProgress(i + 1, frameData.dataFrames.size());
    }

    std::cout << "Programmed " << frameData.dataFrames.size()
              << " Data frames\n";
    return true;
}

bool XO5SRAMRecover::programInitBusFrames()
{
    const auto& frames = frameData.initBusFrames;
    if (frames.empty())
    {
        std::cout << "No InitBus frames to program.\n";

        // Init bus programming is not required
        return true;
    }

    std::cout << "Programming Init Bus Frames...\n";

    std::vector<uint8_t> buf;

    for (size_t i = 0; i < frames.size(); ++i)
    {
        const auto& addr = frames[i].addr;
        const auto& data = frames[i].data;


        if (i2cWriteReadCmd(addr) != 0)
        {
            std::cerr << "Failed to write InitBus address for frame " << i
                        << ": " << vecToHexStr(addr) << "\n";
            return false;
        }
        
        std::this_thread::sleep_for(std::chrono::microseconds(10000));

        buf = data;
        if (i2cWriteReadCmd(buf) != 0)
        {
            std::cerr << "Failed to write InitBus data for frame " << i
                        << ": " << vecToHexStr(buf) << "\n";
            return false;
        }
    
        std::this_thread::sleep_for(std::chrono::microseconds(10000));
        printProgress(i + 1, frames.size());
    }

    std::cout << "Programmed " << frameData.initBusFrames.size()
              << " Init Bus frames\n";
    return true;
}

bool XO5SRAMRecover::programPowerCtrlRegister()
{
    std::vector<uint8_t> powerCtrlCmd = {0x56, 0x80, 0x00, 0x00};

    std::this_thread::sleep_for(std::chrono::microseconds(1000));

    if (i2cWriteReadCmd(powerCtrlCmd) != 0)
    {
        std::cerr << "Failed to program power control register\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Programmed Power Ctrl Register\n";

    return true;
}

bool XO5SRAMRecover::programUserCode()
{
    if (frameData.usercodeCmd.size() < 8)
    {
        std::cerr << "Usercode command length is less than 8 bytes\n";
        return false;
    }

    auto userCode = std::vector<uint8_t>(frameData.usercodeCmd.begin() + 4,
                                         frameData.usercodeCmd.begin() + 8);

    std::vector<uint8_t> userCodeCmd = {0xC2, 0x00, 0x00, 0x00};
    userCodeCmd.insert(userCodeCmd.end(), userCode.begin(), userCode.end());

    if (i2cWriteReadCmd(userCodeCmd) != 0)
    {
        std::cerr << "Failed to program Usercode: "
                  << vecToHexStr(userCode) << "\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "Programmed Usercode: " << vecToHexStr(userCode) << "\n";

    return true;
}

bool XO5SRAMRecover::verifyMIBFrames()
{
    std::cout << "Verifying MIB frames...\n";

    std::vector<uint8_t> setAddrCmd = {0xB4, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x80, 0x00};
    std::vector<uint8_t> readBufCmd = {0x6A, 0x21, 0x00, 0x00};

    if (i2cWriteReadCmd(setAddrCmd) != 0)
    {
        std::cerr << "Faild to set address, cmd = " << vecToHexStr(setAddrCmd)
                  << "\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (size_t i = 0; i < frameData.mibFrames.size(); ++i)
    {
        const auto& frame = frameData.mibFrames[i];

        std::vector<uint8_t> buf(frame.data.size() + 2);

        if (i2cWriteReadCmd(readBufCmd, buf.size(), buf) != 0)
        {
            std::cerr << "Failed to read MIB frame " << i << " data\n";
            return false;
        }

        if (!std::equal(buf.begin() + 2, buf.begin() + 2 + frame.data.size(),
                        frame.data.begin()))
        {
            std::cerr << "Verify MIB frame " << i << " failed\n";
            return false;
        }

        printProgress(i + 1, frameData.mibFrames.size());
    }

    std::cout << "Verify MIB frames success\n";
    return true;
}

bool XO5SRAMRecover::verifyDataFrames()
{
    std::cout << "Verifying DATA frames...\n";

    std::vector<uint8_t> initAddrCmd = {0x46, 0x00, 0x00, 0x00};
    std::vector<uint8_t> readBufCmd = {0x6A, 0x21, 0x00, 0x00};

    if (i2cWriteReadCmd(initAddrCmd) != 0)
    {
        std::cerr << "Failed to send init address command: "
                  << vecToHexStr(initAddrCmd) << "\n";
        return false;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    for (size_t i = 0; i < frameData.dataFrames.size(); ++i)
    {
        const auto& frame = frameData.dataFrames[i];

        std::vector<uint8_t> buf(frame.data.size() + 2);

        if (i2cWriteReadCmd(readBufCmd, buf.size(), buf) != 0)
        {
            std::cerr << "Failed to read Data frame " << i << " data\n";
            return false;
        }

        if (!std::equal(buf.begin() + 2, buf.begin() + 2 + frame.data.size(),
                        frame.data.begin()))
        {
            std::cerr << "Verify Data frame " << i << " failed\n";
            return false;
        }

        printProgress(i + 1, frameData.dataFrames.size());
    }

    std::cout << "Verify Data frames success\n";
    return true;
}

bool XO5SRAMRecover::exitProgrammingMode()
{
    std::vector<uint8_t> programDoneBitCmd = {0x5E, 0x00, 0x00, 0x00};
    std::vector<uint8_t> disableISCCmd = {0x26, 0x00, 0x00, 0x00};

    if (i2cWriteReadCmd(programDoneBitCmd) != 0)
    {
        std::cerr << "Failed to program done bit\n";
    }

    std::cout << "Programmed DONE bit\n";

    readStatusRegister();

    if (i2cWriteReadCmd(disableISCCmd) != 0)
    {
        std::cerr << "Failed to disable ISC\n";
    }

    std::cout << "Device Enters Usermode\n";

    return true;
}

int XO5SRAMRecover::fwUpdate(bool)
{
    std::cout << "Start SRAM recover\n";

    if (!parseBitFile()) return -1;
    if (!enterProgrammingMode()) return -1;
    if (!eraseSRAM()) return -1;
    if (!programCtrlRegister()) return -1;
    if (!programMIBFrames()) return -1;
    if (!programDataFrames()) return -1;
    if (!programPowerCtrlRegister()) return -1;
    if (!programUserCode()) return -1;
    if (!verifyMIBFrames()) return -1;
    if (!verifyDataFrames()) return -1;
    if (!programInitBusFrames()) return -1;
    if (!exitProgrammingMode()) return -1;

    std::cout << "SRAM recover finished\n";

    return 0;
}

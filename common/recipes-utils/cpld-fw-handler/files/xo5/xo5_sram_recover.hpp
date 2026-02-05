#pragma once

#include "cpld-fw-handler.hpp"
#include "xo5/xo5_bit_file_frame_parser.hpp"

#include <cstdint>
#include <string>

class XO5SRAMRecover : public CpldManager
{
  public:
    XO5SRAMRecover(uint8_t bus, uint8_t addr, const std::string& path,
                   const std::string& chip, const std::string& interface,
                   const std::string& target, bool debugMode) :
    CpldManager(bus, addr, path, chip, interface, target, debugMode) {}

    int fwUpdate(bool legacy = false) override;

  private:
    bool parseBitFile();
    bool enterProgrammingMode();
    bool eraseSRAM();
    bool programCtrlRegister();
    bool programMIBFrames();
    bool programDataFrames();
    bool programInitBusFrames();
    bool programPowerCtrlRegister();
    bool programUserCode();
    bool verifyMIBFrames();
    bool verifyDataFrames();
    bool exitProgrammingMode();

    std::vector<uint8_t> readStatusRegister();

  private:
    XO5BitFileFrameParser::FrameData frameData;
};

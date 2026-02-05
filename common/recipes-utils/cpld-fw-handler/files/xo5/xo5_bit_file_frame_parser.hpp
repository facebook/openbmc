#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class XO5BitFileFrameParser
{
public:
    struct Frame
    {
        std::vector<uint8_t> addr;
        std::vector<uint8_t> data;
    };

    struct FrameData
    {
        std::vector<uint8_t> controlReg0Cmd;
        std::vector<uint8_t> usercodeCmd;

        std::vector<Frame> mibFrames;
        std::vector<Frame> dataFrames;
        std::vector<Frame> initBusFrames;
    };

    FrameData parseFile(const std::string& filePath);

private:
    bool parseHexFrame(std::string_view frame,
                       std::vector<uint8_t>& out);

    void safeTrimLast3(std::vector<uint8_t>& bytes);
    bool isAllowedLabel(std::string_view label);
};

#include "xo5_bit_file_frame_parser.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip> 

namespace
{
constexpr bool Debug = false;

#define DBG(x) \
    do { if (Debug) std::cerr << "[Parser] " << x << "\n"; } while (0)

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

bool isHex(char c)
{
    return std::isxdigit(static_cast<unsigned char>(c));
}

// Normalize spaces: multiple spaces -> single space, trim ends
std::string normalizeSpaces(std::string_view s)
{
    std::string out;
    bool inSpace = false;

    for (char c : s)
    {
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            if (!inSpace)
            {
                out.push_back(' ');
                inSpace = true;
            }
        }
        else
        {
            out.push_back(c);
            inSpace = false;
        }
    }

    if (!out.empty() && out.front() == ' ')
        out.erase(out.begin());
    if (!out.empty() && out.back() == ' ')
        out.pop_back();

    return out;
}

std::string toLower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

enum class LineType
{
    Comment,
    Blank,
    Data
};

LineType classifyLine(std::string_view line)
{
    if (line.rfind("//", 0) == 0)
        return LineType::Comment;

    for (char c : line)
        if (!std::isspace(static_cast<unsigned char>(c)))
            return LineType::Data;

    return LineType::Blank;
}

enum class LabelType
{
    ControlReg0,
    UserCode,
    MIB,
    Data,
    WriteInitBusAddr,
    InitBusWrite,
    Noop,
    Unknown
};

LabelType classifyLabel(std::string_view label)
{
    auto l = toLower(normalizeSpaces(label));

    if (l.find("control register 0 frame") != std::string::npos)
        return LabelType::ControlReg0;
    if (l.find("usercode frame") != std::string::npos)
        return LabelType::UserCode;
    if (l.find("write mib frame") != std::string::npos)
        return LabelType::MIB;
    if (l.find("data frame") != std::string::npos)
        return LabelType::Data;
    if (l.find("write init bus address frame") != std::string::npos)
        return LabelType::WriteInitBusAddr;
    if (l.find("init bus write frame") != std::string::npos)
        return LabelType::InitBusWrite;
    if (l.find("noop frame") != std::string::npos)
        return LabelType::Noop;

    return LabelType::Unknown;
}

} // namespace

bool XO5BitFileFrameParser::isAllowedLabel(std::string_view label)
{
    return classifyLabel(label) != LabelType::Unknown;
}

bool XO5BitFileFrameParser::parseHexFrame(
    std::string_view frame,
    std::vector<uint8_t> &out)
{
    out.clear();

    std::istringstream iss{std::string(frame)};
    std::string token;
    bool hasData = false;

    while (iss >> token)
    {
        hasData = true;

        if (token.size() % 2 != 0)
            return false;

        for (char c : token)
            if (!isHex(c))
                return false;

        for (size_t i = 0; i < token.size(); i += 2)
        {
            out.push_back(static_cast<uint8_t>(
                std::stoul(token.substr(i, 2), nullptr, 16)));
        }
    }

    return hasData;
}

void XO5BitFileFrameParser::safeTrimLast3(std::vector<uint8_t>& bytes)
{
    if (bytes.size() >= 3 && bytes.back() == 0xFF)
    {
        DBG("Trimming last 3 bytes of frame");
        bytes.resize(bytes.size() - 3);
    }
}

XO5BitFileFrameParser::FrameData
XO5BitFileFrameParser::parseFile(const std::string& filePath)
{
    std::ifstream infile(filePath);
    if (!infile)
    {
        std::cerr << "Failed to open file: " << filePath << "\n";
        return {};
    }

    FrameData frameData;
    std::string currentLabel;
    std::string pendingFrame;
    std::vector<uint8_t> currentBytes;
    std::vector<uint8_t> pendingInitBusAddr;

    auto flushFrameData = [&]() {
        if (pendingFrame.empty())
            return;

        std::vector<uint8_t> tmp;
        if (parseHexFrame(pendingFrame, tmp))
        {
            safeTrimLast3(tmp);
            currentBytes.insert(currentBytes.end(),
                                tmp.begin(), tmp.end());
        }
        pendingFrame.clear();
    };

    auto flushFrameBlock = [&]() {
        flushFrameData();
        if (currentLabel.empty())
            return;

        switch (classifyLabel(currentLabel))
        {
        case LabelType::ControlReg0:
            frameData.controlReg0Cmd = currentBytes;
            DBG("Flushed Control Register, bytes: " 
                << vecToHexStr(currentBytes));
            break;

        case LabelType::UserCode:
            frameData.usercodeCmd = currentBytes;
            DBG("Flushed User Code, bytes: " << vecToHexStr(currentBytes));
            break;

        case LabelType::MIB:
            frameData.mibFrames.push_back(Frame{{}, currentBytes});
            DBG("Flushed MIB Frame, bytes: "  << vecToHexStr(currentBytes));
            break;

        case LabelType::Data:
            frameData.dataFrames.push_back(Frame{{}, currentBytes});
            DBG("Flushed Data Frame, bytes: "  << vecToHexStr(currentBytes));
            break;

        case LabelType::WriteInitBusAddr:
            pendingInitBusAddr = currentBytes;
            break;

        case LabelType::InitBusWrite:
            if (!pendingInitBusAddr.empty())
            {
                frameData.initBusFrames.push_back(
                    Frame{pendingInitBusAddr, currentBytes});

                DBG("Flushed Init Bus Frame, address: "
                    << vecToHexStr(pendingInitBusAddr) << ", bytes: "
                    << vecToHexStr(currentBytes));

                pendingInitBusAddr.clear();
            }
            break;

        case LabelType::Noop:
            // intentionally ignored
            break;

        case LabelType::Unknown:
            DBG("Unknown label encountered: " << currentLabel);
            break;
        }

        currentLabel.clear();
        currentBytes.clear();
    };

    std::string line;
    while (std::getline(infile, line))
    {
        switch (classifyLine(line))
        {
        case LineType::Comment:
        {
            flushFrameBlock();
            std::string label = normalizeSpaces(line.substr(2));
            if (isAllowedLabel(label))
            {
                DBG("Found allowed label: " << label);
                currentLabel = label;
            }
            break;
        }

        case LineType::Data:
            if (!currentLabel.empty())
            {
                if (!pendingFrame.empty())
                    pendingFrame += ' ';
                pendingFrame += normalizeSpaces(line);
            }
            break;

        case LineType::Blank:
            break;
        }
    }

    flushFrameBlock();
    return frameData;
}

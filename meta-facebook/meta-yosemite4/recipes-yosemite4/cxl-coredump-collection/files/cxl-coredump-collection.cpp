#include "cxl-coredump-collection.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <initializer_list>

bool debugFlag = false;
CxlCoredumpCollector* g_collector = nullptr;

void signalHandler(int sig)
{
    if (g_collector != nullptr)
    {
        g_collector->switchMuxToCxl();
        g_collector = nullptr;
    }
    std::_Exit(sig);
}

namespace
{

constexpr uint32_t coredumpHeaderAddr  = 0x01F80000;
constexpr uint32_t coredumpPayloadAddr = 0x01F80010;
constexpr uint32_t expectedSig         = 0xCDCD0100;
constexpr uint16_t chunkSize           = 0x01f4;
constexpr uint16_t headerSize          = 16;

//Convert pldmtool RX data to hex bytes
std::vector<uint8_t> hexToBytes(std::string_view str)
{
    auto hex_val = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        throw std::runtime_error("invalid hex");
    };

    std::vector<uint8_t> result;
    int high = -1;

    for (char c : str) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            continue;
        }

        uint8_t val = hex_val(c);

        if (high == -1) {
            high = val;
        } else {
            uint8_t byte = (high << 4) | val;
            result.push_back(byte);
            high = -1;
        }
    }

    if (high != -1) {
        throw std::runtime_error("odd number of hex digits");
    }

    return result;
}

template <typename T>
std::array<uint8_t, sizeof(T)> toLEBytes(T v)
{
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    std::array<uint8_t, sizeof(T)> out{};
    for (size_t i = 0; i < sizeof(T); ++i)
        out[i] = static_cast<uint8_t>(v >> (i * 8));
    return out;
}

template <typename T>
T readLE(const uint8_t* buf, size_t offset)
{
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i)
        result |= static_cast<T>(buf[offset + i]) << (8 * i);
    return result;
}

uint32_t calChecksum(const std::vector<uint8_t>& payload)
{
    uint32_t sum = 0;
    for (auto b : payload)
        sum += b;
    return sum & 0x0000FFFF;
}

std::string formatHexBytes(std::initializer_list<uint8_t> bytes)
{
    std::string result;
    result.reserve(bytes.size() * 5);
    char buf[6];
    for (auto b : bytes)
    {
        std::snprintf(buf, sizeof(buf), " 0x%02X", b);
        result += buf;
    }
    return result;
}

uint8_t calculateProgressPercent(size_t completed, size_t total)
{
    if (total == 0)
        return 100;

    size_t percent = (completed * 100) / total;
    return static_cast<uint8_t>(std::min<size_t>(100, percent));
}

void writeBinFile(const std::string& path, const std::vector<uint8_t>& data)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs)
        throw std::runtime_error("Failed to open file: " + path);
    ofs.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!ofs)
        throw std::runtime_error("Failed to write data to file: " + path);
}

void printHeaderInfo(const CoredumpHeader& header, bool fullInfo)
{
    std::cout << "=== Coredump Header Info ===" << "\n"
              << std::hex << std::uppercase
              << "Signature:     0x" << header.signature << "\n";
    if (fullInfo)
    {
        std::cout << "Checksum:      0x" << header.checksum << "\n"
                  << std::dec
                  << "Length:        " << header.length << " bytes\n"
                  << "Padded Length: " << header.paddedLen << " bytes";
    }
    std::cout << std::dec << std::endl;
}

}// anonymous namespace 

CxlCoredumpCollector::CxlCoredumpCollector(uint8_t eid, uint8_t cxlCompId)
    : eid_(eid)
{
    req_.cxlCompId = cxlCompId;
}

bool CxlCoredumpCollector::switchMuxToWf()
{
    setReq(req_.cxlCompId, 0x00000000, 0x0000, TransferFlag::start);
    std::cout << "Switch QSPI MUX to WF BIC" << std::endl;
    if (!sendReq())
    {
        std::cerr << "Failed to switch QSPI MUX." << std::endl;
        return false;
    }
    return true;
}

void CxlCoredumpCollector::switchMuxToCxl()
{
    setReq(req_.cxlCompId, 0x00000000, 0x0000, TransferFlag::end);
    std::cout << "Switch QSPI MUX back to CXL" << std::endl;
    if (!sendReq())
        std::cerr << "Failed to switch QSPI MUX back to CXL." << std::endl;
}

bool CxlCoredumpCollector::readHeader(CoredumpHeader& header)
{
    setReq(req_.cxlCompId, coredumpHeaderAddr, 0x0010,
           TransferFlag::readFlash);
    if (!sendReq())
    {
        std::cerr << "Failed to get coredump header" << std::endl;
        return false;
    }
    return parseHeader(header);
}

bool CxlCoredumpCollector::readPayload(std::vector<uint8_t>& payload,
                                       uint64_t payloadLength)
{
    constexpr int MAX_RETRIES = 3;

    uint32_t offset    = coredumpPayloadAddr;
    uint64_t remaining = payloadLength;
    uint8_t lastProgress = 0;

    std::cout << "collecting coredump data: 0%" << std::flush;

    while (remaining > 0)
    {
        uint16_t chunkLen = static_cast<uint16_t>(
            std::min(static_cast<uint64_t>(chunkSize), remaining));

        setReq(req_.cxlCompId, offset, chunkLen, TransferFlag::readFlash);

        bool hasError = true;
        for (int retry = 0; retry < MAX_RETRIES; ++retry)
        {
            if (retry > 0)
                debugPrint("Retrying (", retry, ") times");

            if (!sendReq())
            {
                debugPrint("Chunk read failed at offset 0x",
                            static_cast<unsigned>(offset));
                continue;
            }
            if (resp_.size() <= CC_OFFSET || resp_[CC_OFFSET] != 0x00)
            {
                debugPrint("Bad completion_code at offset 0x",
                            static_cast<unsigned>(offset));
                continue;
            }
            if (resp_.size() < DATA_OFFSET + chunkLen)
            {
                debugPrint("Response too short at offset 0x",
                            static_cast<unsigned>(offset));
                continue;
            }
            if (!checkIana(IANA_OFFSET))
            {
                debugPrint("IANA mismatch at offset 0x",
                            static_cast<unsigned>(offset));
                continue;
            }
            payload.insert(payload.end(),
                           resp_.begin() + DATA_OFFSET,
                           resp_.begin() + DATA_OFFSET + chunkLen);
            hasError = false;
            break;
        }

        if (hasError)
        {
            std::cerr << "All retries failed at offset 0x"
                      << std::hex << offset << std::dec << std::endl;
            return false;
        }

        offset    += chunkLen;
        remaining -= chunkLen;

        uint64_t completed = payloadLength - remaining;
        uint8_t progress = calculateProgressPercent(completed, payloadLength);
        if (progress != lastProgress)
        {
            std::cout << "\rcollecting coredump data: "
                      << static_cast<unsigned>(progress)
                      << "%" << std::flush;
            lastProgress = progress;
        }
    }

    std::cout << std::endl;
    return true;
}

void CxlCoredumpCollector::setReq(uint8_t cxlId, uint32_t dataOffset,
                                  uint16_t dataLen, TransferFlag flag)
{
    req_.cxlCompId  = cxlId;
    req_.dataOffset = dataOffset;
    req_.dataLen    = dataLen;
    req_.flag       = static_cast<uint8_t>(flag);
}

std::string CxlCoredumpCollector::buildPldmCmd() const
{
    auto off = toLEBytes<uint32_t>(req_.dataOffset);
    auto len = toLEBytes<uint16_t>(req_.dataLen);
    return "pldmtool raw -m " + std::to_string(eid_) + " -d" +
           formatHexBytes({
               0x80, 0x3F, req_.readFlashCmd,
               req_.iana[0], req_.iana[1], req_.iana[2],
               req_.flag, req_.cxlCompId,
               off[0], off[1], off[2], off[3],
               len[0], len[1]
           }) + " 2>/dev/null";
}

bool CxlCoredumpCollector::sendReq()
{
    resp_.clear();
    std::string cmd = buildPldmCmd();

    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp)
    {
        debugPrint("popen failed: ", cmd);
        return false;
    }

    char line[512]   = {};
    std::string rxLine;
    std::string currentRx;
    bool collectingRx = false;

    while (fgets(line, sizeof(line), fp))
    {
        std::string s(line);
        size_t rxPos = s.find("Rx:");
        if (rxPos != std::string::npos)
        {
            currentRx    = s.substr(rxPos + 3);
            collectingRx = (s.find('\n') == std::string::npos);
            if (!collectingRx)
                rxLine = currentRx;
            continue;
        }
        if (collectingRx)
        {
            currentRx += s;
            if (s.find('\n') != std::string::npos)
            {
                collectingRx = false;
                rxLine       = currentRx;
            }
        }
    }
    if (collectingRx && !currentRx.empty())
        rxLine = currentRx;

    pclose(fp);

    if (rxLine.empty())
    {
        debugPrint("No 'Rx:' found");
        return false;
    }
    resp_ = hexToBytes(rxLine);
    if (resp_.empty())
    {
        debugPrint("Parsed Rx is empty.");
        return false;
    }
    return true;
}

bool CxlCoredumpCollector::checkIana(size_t offset) const
{
    constexpr uint8_t pat[] = {0x15, 0xA0, 0x00};
    return resp_.size() >= offset + sizeof(pat) &&
           std::memcmp(resp_.data() + offset, pat, sizeof(pat)) == 0;
}

bool CxlCoredumpCollector::parseHeader(CoredumpHeader& header) const
{
    if (resp_.size() <= CC_OFFSET)
    {
        debugPrint("Rx too short to contain completion_code");
        return false;
    }
    if (resp_[CC_OFFSET] != 0x00)
    {
        debugPrint("PLDM completion_code error: 0x",
                    static_cast<unsigned>(resp_[CC_OFFSET]));
        return false;
    }
    if (resp_.size() < DATA_OFFSET + headerSize)
    {
        debugPrint("Response too short to contain coredump header");
        return false;
    }
    if (!checkIana(IANA_OFFSET))
    {
        debugPrint("IANA mismatch in response");
        return false;
    }

    const uint8_t* d = resp_.data() + DATA_OFFSET;
    header.signature = readLE<uint32_t>(d, 0);
    header.checksum  = readLE<uint32_t>(d, 4);
    header.length    = readLE<uint64_t>(d, 8);

    if (header.length % 4 != 0)
    {
        header.padding   = 1;
        header.paddedLen = ((header.length + 3) / 4) * 4;
    }
    else
    {
        header.padding   = 0;
        header.paddedLen = header.length;
    }
    return true;
}

int main(int argc, char** argv)
{
    int slot = 0;
    std::string cxlDevice;
    std::string coredumpFilename;

    CLI::App app{"OpenBMC CXL coredump collection tool."};
    app.footer("Example: cxl-coredump-collection 1 cxl_1 coredump");

    app.add_option("slot", slot, "Slot number")
        ->required()
        ->check(CLI::Range(1, 8));
    app.add_option("cxl_device", cxlDevice, "CXL device (cxl_1 or cxl_2)")
        ->required()
        ->check(CLI::IsMember({"cxl_1", "cxl_2"}));
    app.add_option("coredump_filename", coredumpFilename,
                   "Output coredump filename")
        ->required();
    app.add_flag("--debug", debugFlag, "Enable debug logs");

    try
    {
        app.parse(argc, argv);
    }
    catch (const CLI::ParseError& e)
    {
        return app.exit(e);
    }

    uint8_t wfEid     = static_cast<uint8_t>(slot * 10 + 2);
    uint8_t cxlCompId = (cxlDevice == "cxl_1")
                            ? static_cast<uint8_t>(CompId::cxl1)
                            : static_cast<uint8_t>(CompId::cxl2);

    CxlCoredumpCollector collector{wfEid, cxlCompId};

    if (!collector.switchMuxToWf())
    {
        std::cerr << "Failed to switch slot " << slot
                  << " CXL MUX " << cxlDevice << std::endl;
        return 1;
    }

    // Register signal handlers to restore MUX on interruption
    g_collector = &collector;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // MUX switches back to CXL automatically on every exit path
    MuxGuard guard{collector};
    // Read Header and check signature
    CoredumpHeader header{};
    if (!collector.readHeader(header))
    {
        std::cerr << "NO coredump header found for slot " << slot
                  << " from CXL flash" << std::endl;
        return 1;
    }
   
    if (header.signature == expectedSig)
    {
        printHeaderInfo(header, true);
    }
    else
    {
        printHeaderInfo(header, false);
        std::cerr << "Invalid coredump header from slot " << slot
                  << " CXL flash" << std::endl;
        return 1;
    }
    // Read Payload and verify checksum
    uint64_t payloadSize = header.padding ? header.paddedLen : header.length;
    std::vector<uint8_t> payload;
    payload.reserve(static_cast<size_t>(payloadSize));

    if (!collector.readPayload(payload, payloadSize))
    {
        std::cerr << "Failed to get coredump payload from slot " << slot
                  << " CXL flash" << std::endl;
        return 1;
    }

    if (calChecksum(payload) != header.checksum)
    {
        std::cerr << "payload checksum = " << calChecksum(payload)
                  << " expected = "        << header.checksum << std::endl;
        std::cerr << "Checksum mismatch please check PLDM status and retry" << std::endl;
        return 1;
    }
    else
    {
        std::cout << "Checksum verification passed" << std::endl;
    }

    // If payload has padding, need to remove the padding part for gdb useage
    try
    {
        if (header.padding)
        {
            writeBinFile(coredumpFilename + "_verify.bin", payload);
            payload.resize(static_cast<size_t>(header.length));
            writeBinFile(coredumpFilename + "_gdb.bin", payload);
        }
        else
        {
            writeBinFile(coredumpFilename + "_verify.bin", payload);
            writeBinFile(coredumpFilename + "_gdb.bin",    payload);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    std::cout << "save " << coredumpFilename + "_verify.bin"
              << " for checksum verification" << std::endl;
    std::cout << "save " << coredumpFilename + "_gdb.bin"
              << " for gdb analysis" << std::endl;

    return 0;
}


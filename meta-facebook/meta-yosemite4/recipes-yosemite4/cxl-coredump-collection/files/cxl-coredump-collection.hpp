#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

extern bool debugFlag;

template <typename... Args>
void debugPrint(Args&&... args )
{
    if (debugFlag)
    {
        ((std::cerr << std::forward<Args>(args)), ...) << '\n';
    }
}
enum class TransferFlag : uint8_t {
    start     = 0x00,
    readFlash = 0x01,
    end       = 0x02
};

enum class CompId : uint8_t {
    cxl1 = 0x05,
    cxl2 = 0x06,
};
struct CoredumpHeader
{
    uint32_t signature;
    uint32_t checksum;
    uint64_t length;
    uint8_t  padding   = 0;
    uint64_t paddedLen = 0;
};

struct PldmToolReq
{
    uint8_t  readFlashCmd = 0x08;
    uint8_t  iana[3]      = {0x15, 0xA0, 0x00};
    uint8_t  flag         = 0;
    uint8_t  cxlCompId    = 0;
    uint32_t dataOffset   = 0;
    uint16_t dataLen      = 0;
};


class CxlCoredumpCollector
{
public:
    CxlCoredumpCollector(uint8_t eid, uint8_t cxlCompId);

    bool switchMuxToWf();
    void switchMuxToCxl();
    bool readHeader(CoredumpHeader& header);
    bool readPayload(std::vector<uint8_t>& payload, uint64_t payloadLength);

private:
    static constexpr size_t CC_OFFSET   = 3;
    static constexpr size_t IANA_OFFSET = 4;
    static constexpr size_t DATA_OFFSET = 9;

    uint8_t              eid_;
    PldmToolReq          req_{};
    std::vector<uint8_t> resp_;

    void setReq(uint8_t cxlId, uint32_t dataOffset,
                uint16_t dataLen, TransferFlag flag);
    std::string buildPldmCmd() const;
    bool sendReq();
    bool checkIana(size_t offset) const;
    bool parseHeader(CoredumpHeader& header) const;
};

class MuxGuard
{
public:
    explicit MuxGuard(CxlCoredumpCollector& c) : collector_(c) {}
    ~MuxGuard() noexcept { collector_.switchMuxToCxl(); }
    MuxGuard(const MuxGuard&)            = delete;
    MuxGuard& operator=(const MuxGuard&) = delete;

private:
    CxlCoredumpCollector& collector_;
};

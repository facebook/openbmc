#pragma once

#include <sdbusplus/bus.hpp>

constexpr size_t IANA_ID_SIZE = 3;
constexpr uint32_t IANA_ID = 0x00A015;
constexpr uint8_t ERASE_FLASH_IANA_ID[IANA_ID_SIZE] = {0x15, 0xA0, 0x00};
constexpr size_t SUCCESS = 0;
constexpr int MAX_RETRY_TIME = 3;

constexpr size_t NETFN_OEM_1S_REQ = 0x38;
constexpr size_t CMD_OEM_1S_UPDATE_FW = 0x9;
constexpr size_t CMD_OEM_1S_MSG_OUT = 0x02;
constexpr size_t CMD_OEM_1S_ERASE_BIOS_FLASH = 0xB4;
constexpr size_t CMD_OEM_1S_GET_BIOS_ERASE_PROGRESS = 0xB5;

class BIOSupdater
{
  public:
    explicit BIOSupdater(sdbusplus::bus_t& bus, const std::string& imagePath,
                         const uint8_t slotId, const std::string& cpuType) :
        bus(bus),
        imagePath(imagePath), slotId(slotId), cpuType(cpuType)
    {}

    /** @brief Update bios according to the USB file path.
     *
     *  @return Success or Fail
     */
    bool run();

  private:
    /** @brief Persistent sdbusplus DBus bus connection. */
    sdbusplus::bus_t& bus;

    /** The image path. */
    const std::string& imagePath;

    /** The slot Id for update*/
    const uint8_t slotId;

    /** BERGAMO or TURIN or TURINES cpu will use different offset to update */
    /** Will update both offset if it is not set */
    const std::string& cpuType;
};

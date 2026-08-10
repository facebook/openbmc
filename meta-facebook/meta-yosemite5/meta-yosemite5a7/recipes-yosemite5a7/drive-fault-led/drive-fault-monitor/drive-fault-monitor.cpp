#include "drive-fault-monitor.hpp"

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

#include <charconv>
#include <cstdint>
#include <format>
#include <optional>
#include <string_view>
#include <system_error>
#include <unordered_map>

namespace drive::fault
{

constexpr size_t recordTypeIdx = 2;
constexpr size_t errorTypeIdx  = 3;
constexpr size_t driveSlotIdx  = 4;
constexpr size_t driveEventIdx = 8;

constexpr uint8_t fbOemRecordType = 0xFB;
constexpr uint8_t unifiedDriveEvt = 5;

enum class DriveEvent
{
    Deassert = 0,
    Assert   = 1
};

/**
 * @brief Extract a single byte from a hex-encoded RAW_EVENT string.
 *
 * @param[in] rawEvent  The hex string read from the AdditionalData
 *                      "RAW_EVENT" field of a logging entry.
 * @param[in] offset    Byte offset within the SEL record (0-based).
 * @param[in] len       Number of hex characters to consume. 
 *
 * @return The parsed value on success, or std::nullopt if @p offset / @p len
 *         falls outside the string or the substring is not valid hex.
 */
std::optional<uint8_t> parseRawEvent(std::string_view rawEvent, size_t offset, size_t len = 2)
{
    const size_t idx = offset * 2;
    if (idx + len > rawEvent.size())
    {
        return std::nullopt;
    }

    uint8_t ret{};
    const char* begin = rawEvent.data() + idx;
    const char* end = begin + len;
    auto [ptr, ec] = std::from_chars(begin, end, ret, 16);
    if (ec != std::errc{} || ptr != end)
    {
        return std::nullopt;
    }
    return ret;
}

void DriveSelMonitor::driveLed(int slot, bool assert)
{
    std::string ledPath = 
        std::format("{}/drive{}_fault", LED_GROUP_ROOT, slot);

    auto method = bus.new_method_call(LED_SERVICE, ledPath.c_str(),
        "org.freedesktop.DBus.Properties", "Set");
    method.append(LED_IFACE, "Asserted", std::variant<bool>(assert));
    try 
    {
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to {ASSERT} drive fault LED group, path: {PATH}, error: {ERROR}",
            "ASSERT", (assert) ? "assert" : "deassert", "PATH", ledPath, "ERROR", e);
        return;
    }
}

void DriveSelMonitor::onEntryCreated(sdbusplus::message_t& msg)
{
    sdbusplus::object_path objectPath;
    InterfaceMap interfaces;
    try
    {
        msg.read(objectPath, interfaces);
    }
    catch (const sdbusplus::exception_t& e)
    {
        lg2::error("Failed to read InterfacesAdded entry, error: {ERROR}", "ERROR", e);
        return;
    }

    auto it = interfaces.find(LOGGING_ENTRY_IFACE);
    if (it == interfaces.end())
    {
        return;
    }

    processEntry(it->second);
}

void DriveSelMonitor::processEntry(const PropertyMap& prop)
{
    auto dataIt = prop.find("AdditionalData");
    if (dataIt == prop.end())
    {
        lg2::debug("Logging entry has no AdditionalData property");
        return;
    }

    const auto* data = std::get_if<AdditionalData>(&dataIt->second);
    if (!data)
    {
        lg2::error("AdditionalData is not the expected map type");
        return;
    }

    auto rawEvtIt = data->find("RAW_EVENT");
    if (rawEvtIt == data->end())
    {
        // RAW_EVENT not found
        return;
    }

    std::string_view raw = rawEvtIt->second;

    auto recType = parseRawEvent(raw, recordTypeIdx);
    auto errType = parseRawEvent(raw, errorTypeIdx);
    auto slotNum = parseRawEvent(raw, driveSlotIdx);
    auto evtType = parseRawEvent(raw, driveEventIdx);

    if (!recType || !errType || !slotNum || !evtType)
    {
        lg2::error("Failed to parse RAW_EVENT: {RAW_EVENT}", "RAW_EVENT", raw);
        return;
    }

    if (*recType != fbOemRecordType || (*errType & 0x0F) != unifiedDriveEvt)
    {
        // not a drive-fault unified SEL
        return;
    }

    switch (static_cast<DriveEvent>(*evtType))
    {
        case DriveEvent::Deassert:
            lg2::info("Deasserting drive fault LED for slot {SLOT}",
                      "SLOT", *slotNum);
            driveLed(*slotNum, false);
            break;
        case DriveEvent::Assert:
            lg2::info("Asserting drive fault LED for slot {SLOT}",
                      "SLOT", *slotNum);
            driveLed(*slotNum, true);
            break;
        default:
            lg2::warning(
                "Unknown drive event type {TYPE} for slot {SLOT}",
                "TYPE", *evtType, "SLOT", *slotNum);
            break;
    }
}

} // namespace drive::fault

int main()
{
    sdbusplus::bus_t bus = sdbusplus::bus::new_default();
    drive::fault::DriveSelMonitor monitor(bus);
    bus.process_loop();
    return 0;
}
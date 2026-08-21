#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <string>
#include <variant>
#include <unordered_map>

namespace drive::fault
{
using AdditionalData = std::unordered_map<std::string, std::string>;
using Attributes = std::variant<bool, uint32_t, uint64_t, std::string, AdditionalData>;
using PropertyName = std::string;
using PropertyMap = std::unordered_map<PropertyName, Attributes>;
using InterfaceName = std::string;
using InterfaceMap = std::unordered_map<InterfaceName, PropertyMap>;

static constexpr auto LED_SERVICE = "xyz.openbmc_project.LED.GroupManager";
static constexpr auto LED_GROUP_ROOT = "/xyz/openbmc_project/led/groups";
static constexpr auto LED_IFACE = "xyz.openbmc_project.Led.Group";

static constexpr auto LOGGING_SERVICE = "xyz.openbmc_project.Logging";
static constexpr auto LOGGING_PATH    = "/xyz/openbmc_project/logging";
static constexpr auto LOGGING_ENTRY_IFACE = "xyz.openbmc_project.Logging.Entry";
    
class DriveSelMonitor
{
  public:
    DriveSelMonitor() = delete;
    // disable copy, move
    DriveSelMonitor(const DriveSelMonitor&) = delete;
    DriveSelMonitor& operator=(const DriveSelMonitor&) = delete;
    DriveSelMonitor(DriveSelMonitor&&) = delete;
    DriveSelMonitor& operator=(DriveSelMonitor&&) = delete;
    
    ~DriveSelMonitor() = default;
    
    /**
     * @brief Constructs a monitor to watch for drive SEL events.
     *
     * The monitor listens for new logging entries for unified SEL drive fault events.
     *
     * @param[in] bus  The D-Bus bus object used to interact with the system bus.
     */
    explicit DriveSelMonitor(sdbusplus::bus_t& bus) :
        bus(bus), 
        matchCreated(
            bus,
            sdbusplus::bus::match::rules::interfacesAdded() +
            sdbusplus::bus::match::rules::path_namespace(LOGGING_PATH),
            [this](sdbusplus::message_t& msg) { onEntryCreated(msg); })
    {
        lg2::info("Drive fault monitor initiated");
    }

  private:
    sdbusplus::bus_t& bus;
    sdbusplus::bus::match_t matchCreated;

    /**
     * @brief Update the drive fault LED group state.
     *
     * @param[in] slot     The slot number of target drive (0-based).
     * @param[in] assert   True to assert the LED, false to deassert.
     */
    void driveLed(int slot, bool assert);

    /**
     * @brief Handle newly created logging entries.
     *
     * @param[in] msg  The D-Bus message carrying the InterfacesAdded event.
     */
    void onEntryCreated(sdbusplus::message_t& msg);

    /**
     * @brief Process a logging entry for drive SEL data.
     *
     * @param[in] props  The property map for the logging entry.
     */
    void processEntry(const PropertyMap& props);
};

} // namespace drive::fault
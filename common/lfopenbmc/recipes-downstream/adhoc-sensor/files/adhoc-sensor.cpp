#include "config.h"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/server/manager.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>
#include <xyz/openbmc_project/Metric/Value/aserver.hpp>
#include <xyz/openbmc_project/Sensor/Value/aserver.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <systemd/sd-bus.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <limits>
#include <cmath>

PHOSPHOR_LOG2_USING;

constexpr const char* SENSOR_BUSNAME = "xyz.openbmc_project.AdhocSensor";
constexpr const char* SENSOR_NAMESPACE = "/xyz/openbmc_project/sensors";
// Kept for existing writers, though files may now be any sensor type.
constexpr const char* ADHOC_DIR = "/run/openbmc/sensors/utilization";

// Plain numeric BMC metrics, not sensors: no unit suffix, no clamping, no
// chassis association. bmcweb reports every Metric.Value under this namespace
// as ManagerDiagnosticData Oem/Meta/Metrics/<name>.
constexpr const char* METRIC_NAMESPACE = "/xyz/openbmc_project/metric/bmc/oem";
constexpr const char* METRICS_DIR = "/run/openbmc/metrics";

// Sensors are associated with one inventory chassis so bmcweb lists them under
// /redfish/v1/Chassis/<id>/Sensors. entity-manager publishes inventory some
// time after boot, so the chassis is discovered at runtime and re-evaluated as
// inventory changes. bmcweb's Chassis collection includes boards, and most
// platforms have no Item.Chassis at all, so boards are the fallback.
constexpr const char* INVENTORY_PATH = "/xyz/openbmc_project/inventory";
constexpr const char* CHASSIS_INTERFACE =
    "xyz.openbmc_project.Inventory.Item.Chassis";
constexpr const char* BOARD_INTERFACE =
    "xyz.openbmc_project.Inventory.Item.Board";
constexpr const char* ASSOCIATION_INTERFACE = "xyz.openbmc_project.Association";

// entity-manager adds inventory in bursts; wait for it to settle.
constexpr auto CHASSIS_SETTLE_TIME = std::chrono::seconds(2);

// D-Bus server object using sdbusplus async server_t (CRTP pattern)
class AdhocSensorObject;
using AdhocSensorInterfaces = sdbusplus::async::server_t<
    AdhocSensorObject,
    sdbusplus::aserver::xyz::openbmc_project::association::Definitions,
    sdbusplus::aserver::xyz::openbmc_project::sensor::Value>;

class AdhocSensorObject : public AdhocSensorInterfaces
{
  public:
    AdhocSensorObject(sdbusplus::async::context& ctx, const char* path) :
        AdhocSensorInterfaces(ctx, path)
    {}

    void emit_added()
    {
        Definitions::emit_added();
        Value::emit_added();
    }
};

class AdhocMetricObject;
using AdhocMetricInterfaces = sdbusplus::async::server_t<
    AdhocMetricObject, sdbusplus::aserver::xyz::openbmc_project::metric::Value>;

class AdhocMetricObject : public AdhocMetricInterfaces
{
  public:
    AdhocMetricObject(sdbusplus::async::context& ctx, const char* path) :
        AdhocMetricInterfaces(ctx, path)
    {}

    void emit_added()
    {
        Value::emit_added();
    }
};

using MetricUnit =
    sdbusplus::common::xyz::openbmc_project::metric::Value::Unit;

// Optional second line of a metric file. Empty means Count, the right unit for
// flags and counters, which is what these metrics mostly are.
MetricUnit parseMetricUnit(std::string text, const std::string& name)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (text.empty() || text == "count")
    {
        return MetricUnit::Count;
    }
    if (text == "bytes")
    {
        return MetricUnit::Bytes;
    }
    if (text == "frequency")
    {
        return MetricUnit::Frequency;
    }
    if (text == "percent")
    {
        return MetricUnit::Percent;
    }
    if (text == "seconds")
    {
        return MetricUnit::Seconds;
    }
    error("Unknown unit '{UNIT}' for metric {METRIC}, using count", "UNIT",
          text, "METRIC", name);
    return MetricUnit::Count;
}

// Sensor and metric file names become D-Bus object path elements.
bool isValidName(const std::string& name)
{
    return !name.empty() && std::ranges::all_of(name, [](unsigned char c) {
        return std::isalnum(c) || c == '_';
    });
}

using SensorUnit =
    sdbusplus::common::xyz::openbmc_project::sensor::Value::Unit;

// What an adhoc sensor file can be, selected by its optional second line.
// The suffix is the unit token Meta's sensor naming convention requires
// (central_proxy sensor_schema.py); names without one never reach ODS.
struct SensorType
{
    std::string_view unitName;
    std::string_view nameSpace;
    SensorUnit unit;
    std::string_view suffix;
};

constexpr std::array sensorTypes{
    SensorType{"percent", "utilization", SensorUnit::Percent, "_UTIL_PCT"},
    SensorType{"celsius", "temperature", SensorUnit::DegreesC, "_TEMP_C"},
    SensorType{"watts", "power", SensorUnit::Watts, "_PWR_W"},
    SensorType{"amperes", "current", SensorUnit::Amperes, "_CURR_A"},
    SensorType{"volts", "voltage", SensorUnit::Volts, "_VOLT_V"},
    SensorType{"rpm", "fan_tach", SensorUnit::RPMS, "_SPEED_RPM"},
    SensorType{"cfm", "airflow", SensorUnit::CFM, "_AIRFLOW_CFM"},
    SensorType{"joules", "energy", SensorUnit::Joules, "_ENERGY_J"},
    SensorType{"pascals", "pressure", SensorUnit::Pascals, "_PRESSURE_PA"},
};

// Empty means percent, the only type before units were added. Unknown units
// return nullptr: publishing a value under the wrong unit is worse than not
// publishing it.
const SensorType* parseSensorType(std::string text, const std::string& file)
{
    std::ranges::transform(text, text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (text.empty())
    {
        return sensorTypes.data();
    }
    auto it = std::ranges::find(sensorTypes, text, &SensorType::unitName);
    if (it == sensorTypes.end())
    {
        error("Ignoring adhoc file {FILE}: unknown unit '{UNIT}'", "FILE", file,
              "UNIT", text);
        return nullptr;
    }
    return &*it;
}

class SensorManager
{
  public:
    // Runs on the caller's thread before ctx.run(). Everything after that runs
    // on the context's worker thread and must not make synchronous bus calls:
    // the caller thread is in sd_bus_wait on the same bus, and sd-bus is not
    // thread safe.
    explicit SensorManager(sdbusplus::async::context& ctx) :
        ctx(ctx), selfName(ctx.get_bus().get_unique_name()),
        inotifyFd(-1)
    {
        // Create watch directories if they don't exist
        std::filesystem::create_directories(ADHOC_DIR);
        std::filesystem::create_directories(METRICS_DIR);

        // Initialize inotify
        setupInotify();
    }

    ~SensorManager()
    {
        if (inotifyFd >= 0)
        {
            close(inotifyFd);
        }
    }

    sdbusplus::async::task<> addSensor(std::string file, const SensorType& type,
                                       double value)
    {
        std::string name = file;
        if (!name.ends_with(type.suffix))
        {
            name += type.suffix;
        }
        std::string path = std::string(SENSOR_NAMESPACE) + "/" +
                           std::string(type.nameSpace) + "/" + name;

        // Two owners of one sensor path make bmcweb fail requests for both,
        // so a real sensor always wins. See also monitorSensorCollisions().
        if (auto owner = co_await otherPathOwner(path); !owner.empty())
        {
            error("Not publishing {PATH} for adhoc file {FILE}: {SERVICE} "
                  "already owns it. Rename the file.",
                  "PATH", path, "FILE", file, "SERVICE", owner);
            blockedFiles.insert(file);
            co_return;
        }

        auto object = std::make_unique<AdhocSensorObject>(ctx, path.c_str());

        // Set initial properties without signaling
        constexpr bool emitSignal = false;
        object->unit<emitSignal>(type.unit);
        if (type.unit == SensorUnit::Percent)
        {
            object->max_value<emitSignal>(100.0);
            object->min_value<emitSignal>(0.0);
        }
        object->value<emitSignal>(value);
        object->associations<emitSignal>(chassisAssociations());

        // Announce the object on D-Bus
        object->emit_added();

        sensors[file] = {std::move(object), &type, path};

        info("Created adhoc sensor: {PATH} associated with chassis: {CHASSIS}",
             "PATH", path, "CHASSIS", chassis);
    }

    // A service other than this one with a Sensor.Value at path, or empty.
    // Unfiltered, the mapper lists itself for paths it hangs associations off.
    sdbusplus::async::task<std::string> otherPathOwner(std::string path)
    {
        constexpr auto mapper = sdbusplus::async::proxy()
                                    .service("xyz.openbmc_project.ObjectMapper")
                                    .path("/xyz/openbmc_project/object_mapper")
                                    .interface("xyz.openbmc_project.ObjectMapper");
        std::map<std::string, std::vector<std::string>> owners;
        try
        {
            owners = co_await mapper.call<decltype(owners)>(
                ctx, "GetObject", path,
                std::vector<std::string>{"xyz.openbmc_project.Sensor.Value"});
        }
        catch (const std::exception&)
        {
            // ResourceNotFound: nobody has it.
        }
        for (const auto& [service, interfaces] : owners)
        {
            if (service != SENSOR_BUSNAME && service != selfName)
            {
                co_return service;
            }
        }
        co_return std::string{};
    }

    double readNumberFromFile(const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            error("Failed to open file: {FILE}", "FILE", filePath.string());
            return std::numeric_limits<double>::quiet_NaN();
        }

        std::string line;
        if (!std::getline(file, line))
        {
            error("Failed to read from file: {FILE}", "FILE",
                  filePath.string());
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty())
        {
            error("Empty file content: {FILE}", "FILE", filePath.string());
            return std::numeric_limits<double>::quiet_NaN();
        }

        // Try to parse as double
        double value = 0.0;
        try
        {
            size_t pos = 0;
            value = std::stod(line, &pos);

            // Check if entire string was consumed (no trailing garbage)
            if (pos != line.length())
            {
                error("Invalid numeric value in file {FILE}: '{VALUE}' "
                      "(trailing characters)",
                      "FILE", filePath.string(), "VALUE", line);
                return std::numeric_limits<double>::quiet_NaN();
            }
        }
        catch (const std::invalid_argument& e)
        {
            error("Invalid numeric value in file {FILE}: '{VALUE}' "
                  "(not a number)",
                  "FILE", filePath.string(), "VALUE", line);
            return std::numeric_limits<double>::quiet_NaN();
        }
        catch (const std::out_of_range& e)
        {
            error("Numeric value out of range in file {FILE}: '{VALUE}'",
                  "FILE", filePath.string(), "VALUE", line);
            return std::numeric_limits<double>::quiet_NaN();
        }

        return value;
    }

    // Second line of the file, trimmed; empty if there is none.
    std::string readUnitFromFile(const std::filesystem::path& filePath)
    {
        std::ifstream file(filePath);
        std::string line;
        if (!std::getline(file, line) || !std::getline(file, line))
        {
            return {};
        }
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);
        return line;
    }

    // Percent is the only bounded unit.
    double clampPercent(double value, const std::filesystem::path& filePath)
    {
        if (value < 0.0)
        {
            warning("Negative value {VALUE} in file {FILE}, clamping to 0.0",
                    "VALUE", value, "FILE", filePath.string());
            return 0.0;
        }
        if (value > 100.0)
        {
            warning(
                "Value {VALUE} exceeds 100 in file {FILE}, clamping to 100.0",
                "VALUE", value, "FILE", filePath.string());
            return 100.0;
        }
        return value; // includes NaN
    }

    sdbusplus::async::task<> scanDirectory()
    {
        std::unordered_set<std::string> currentFiles;
        std::unordered_set<std::string> presentFiles;

        try
        {
            if (std::filesystem::exists(ADHOC_DIR))
            {
                for (const auto& entry :
                     std::filesystem::directory_iterator(ADHOC_DIR))
                {
                    if (!entry.is_regular_file())
                    {
                        continue;
                    }
                    std::string file = entry.path().filename().string();
                    if (!isValidName(file))
                    {
                        error("Ignoring adhoc file {FILE}: names must be "
                              "[A-Za-z0-9_]",
                              "FILE", file);
                        continue;
                    }
                    presentFiles.insert(file);
                    if (blockedFiles.contains(file))
                    {
                        continue;
                    }
                    const SensorType* type =
                        parseSensorType(readUnitFromFile(entry.path()), file);
                    if (type == nullptr)
                    {
                        continue;
                    }
                    currentFiles.insert(file);

                    double value = readNumberFromFile(entry.path());
                    if (type->unit == SensorUnit::Percent)
                    {
                        value = clampPercent(value, entry.path());
                    }

                    auto it = sensors.find(file);
                    if (it != sensors.end() && it->second.type != type)
                    {
                        // The unit picks the object path: republish.
                        info("Unit changed for adhoc file {FILE}, recreating",
                             "FILE", file);
                        sensors.erase(it);
                        it = sensors.end();
                    }
                    if (it == sensors.end())
                    {
                        co_await addSensor(file, *type, value);
                    }
                    else
                    {
                        it->second.object->value<true>(value);
                    }
                }
            }

            // Deleting a blocked file unblocks its name.
            std::erase_if(blockedFiles, [&](const std::string& file) {
                return !presentFiles.contains(file);
            });

            std::erase_if(sensors, [&](const auto& kv) {
                if (currentFiles.contains(kv.first))
                {
                    return false;
                }
                info("Adhoc file removed: {FILE}, removing sensor", "FILE",
                     kv.first);
                return true;
            });
        }
        catch (const std::exception& e)
        {
            error("Error scanning adhoc directory: {ERROR}", "ERROR",
                  e.what());
        }
    }

    void scanMetricsDirectory()
    {
        std::unordered_set<std::string> currentFiles;

        try
        {
            if (std::filesystem::exists(METRICS_DIR))
            {
                for (const auto& entry :
                     std::filesystem::directory_iterator(METRICS_DIR))
                {
                    if (!entry.is_regular_file())
                    {
                        continue;
                    }
                    std::string name = entry.path().filename().string();
                    if (!isValidName(name))
                    {
                        error("Ignoring metric file {FILE}: names must be "
                              "[A-Za-z0-9_]",
                              "FILE", name);
                        continue;
                    }
                    currentFiles.insert(name);

                    double value = readNumberFromFile(entry.path());
                    MetricUnit unit =
                        parseMetricUnit(readUnitFromFile(entry.path()), name);

                    auto it = metrics.find(name);
                    if (it != metrics.end() && it->second.unit != unit)
                    {
                        // Unit is a const property: republish the object.
                        info("Unit changed for metric {METRIC}, recreating",
                             "METRIC", name);
                        metrics.erase(it);
                        it = metrics.end();
                    }
                    if (it == metrics.end())
                    {
                        std::string path =
                            std::string(METRIC_NAMESPACE) + "/" + name;
                        auto object = std::make_unique<AdhocMetricObject>(
                            ctx, path.c_str());
                        constexpr bool emitSignal = false;
                        object->unit<emitSignal>(unit);
                        object->value<emitSignal>(value);
                        object->emit_added();
                        info("Created metric: {METRIC} at {PATH}", "METRIC",
                             name, "PATH", path);
                        metrics[name] = {std::move(object), unit};
                    }
                    else
                    {
                        it->second.object->value<true>(value);
                    }
                }
            }

            std::erase_if(metrics, [&](const auto& kv) {
                if (currentFiles.contains(kv.first))
                {
                    return false;
                }
                info("Metric file removed: {METRIC}, removing metric",
                     "METRIC", kv.first);
                return true;
            });
        }
        catch (const std::exception& e)
        {
            error("Error scanning metrics directory: {ERROR}", "ERROR",
                  e.what());
        }
    }

    void setupInotify()
    {
        inotifyFd = inotify_init1(IN_NONBLOCK);
        if (inotifyFd < 0)
        {
            error("Failed to initialize inotify: {ERROR}", "ERROR",
                  strerror(errno));
            ctx.spawn(monitorInotify()); // still does the initial scan
            return;
        }

        // Watch directory for file creation, deletion, and modification
        uint32_t mask =
            IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_TO | IN_MOVED_FROM;

        adhocWd = inotify_add_watch(inotifyFd, ADHOC_DIR, mask);
        if (adhocWd < 0)
        {
            error("Failed to add inotify watch for {DIR}: {ERROR}", "DIR",
                  ADHOC_DIR, "ERROR", strerror(errno));
        }
        else
        {
            info("Added inotify watch for adhoc directory: {DIR}", "DIR",
                 ADHOC_DIR);
        }

        metricsWd = inotify_add_watch(inotifyFd, METRICS_DIR, mask);
        if (metricsWd < 0)
        {
            error("Failed to add inotify watch for {DIR}: {ERROR}", "DIR",
                  METRICS_DIR, "ERROR", strerror(errno));
        }
        else
        {
            info("Added inotify watch for metrics directory: {DIR}", "DIR",
                 METRICS_DIR);
        }

        ctx.spawn(monitorInotify());
    }

    sdbusplus::async::task<> monitorInotify()
    {
        // Initial scan
        co_await scanDirectory();
        scanMetricsDirectory();
        if (inotifyFd < 0)
        {
            co_return;
        }

        sdbusplus::async::fdio fdio(ctx, inotifyFd);

        while (!ctx.stop_requested())
        {
            co_await fdio.next();

            // Read and process events. One scan per directory per batch is
            // enough: a scan reads the whole directory.
            bool adhocChanged = false;
            bool metricsChanged = false;
            alignas(inotify_event) char buffer[4096];
            ssize_t bytesRead = 0;
            while ((bytesRead = read(inotifyFd, buffer, sizeof(buffer))) > 0)
            {
                size_t offset = 0;
                while (offset < static_cast<size_t>(bytesRead))
                {
                    const auto* event =
                        reinterpret_cast<const inotify_event*>(buffer + offset);

                    // Only process regular file events (ignore directories)
                    if (!(event->mask & IN_ISDIR) && event->len > 0)
                    {
                        if (event->wd == metricsWd)
                        {
                            metricsChanged = true;
                        }
                        else
                        {
                            adhocChanged = true;
                        }
                    }

                    offset += sizeof(inotify_event) + event->len;
                }
            }

            if (adhocChanged)
            {
                co_await scanDirectory();
            }
            if (metricsChanged)
            {
                scanMetricsDirectory();
            }
        }
    }

    // Watch inventory and keep the sensor chassis association current.
    // Metrics do not depend on this.
    sdbusplus::async::task<> monitorChassis()
    {
        namespace rules = sdbusplus::bus::match::rules;
        // InterfacesAdded and InterfacesRemoved for any object under
        // inventory. The signal path is the sender's ObjectManager, which may
        // be "/", so match on the object path argument instead.
        sdbusplus::async::match match(
            ctx, rules::type::signal() +
                     rules::interface("org.freedesktop.DBus.ObjectManager") +
                     rules::argNpath(0, std::string(INVENTORY_PATH) + "/"));

        requestChassisRefresh();
        while (!ctx.stop_requested())
        {
            co_await match.next();
            requestChassisRefresh();
        }
    }

    // A real sensor that appears after ours, e.g. dbus-sensors starting
    // later, takes the path: withdraw ours.
    sdbusplus::async::task<> monitorSensorCollisions()
    {
        namespace rules = sdbusplus::bus::match::rules;
        sdbusplus::async::match match(
            ctx, rules::interfacesAdded() +
                     rules::argNpath(0, std::string(SENSOR_NAMESPACE) + "/"));
        while (!ctx.stop_requested())
        {
            auto msg = co_await match.next();
            try
            {
                const char* sender = msg.get_sender();
                if (sender == nullptr || selfName == sender)
                {
                    continue;
                }
                // Evaluating arg0path match rules reads the message after it
                // was queued for us, so start from the beginning.
                sd_bus_message_rewind(msg.get(), true);
                sdbusplus::object_path path;
                msg.read(path);
                auto it = std::ranges::find_if(sensors, [&](const auto& kv) {
                    return kv.second.path == path.str;
                });
                if (it == sensors.end())
                {
                    continue;
                }
                error("Withdrawing {PATH} for adhoc file {FILE}: {SERVICE} "
                      "published it. Rename the file.",
                      "PATH", path.str, "FILE", it->first, "SERVICE",
                      std::string(sender));
                blockedFiles.insert(it->first);
                sensors.erase(it);
            }
            catch (const std::exception& e)
            {
                error("Error handling InterfacesAdded: {ERROR}", "ERROR",
                      e.what());
            }
        }
    }

  private:
    sdbusplus::async::task<std::vector<std::string>>
        inventoryPaths(const char* interface)
    {
        constexpr auto mapper = sdbusplus::async::proxy()
                                    .service("xyz.openbmc_project.ObjectMapper")
                                    .path("/xyz/openbmc_project/object_mapper")
                                    .interface("xyz.openbmc_project.ObjectMapper");
        try
        {
            co_return co_await mapper.call<std::vector<std::string>>(
                ctx, "GetSubTreePaths", INVENTORY_PATH, 0,
                std::vector<std::string>{interface});
        }
        catch (const std::exception& e)
        {
            // The mapper reports ResourceNotFound when nothing matches.
            debug("No inventory objects with {INTF}: {ERROR}", "INTF",
                  interface, "ERROR", e.what());
            co_return std::vector<std::string>{};
        }
    }

    sdbusplus::async::task<std::string> discoverChassis()
    {
        auto chassisPaths = co_await inventoryPaths(CHASSIS_INTERFACE);
        auto boardPaths = co_await inventoryPaths(BOARD_INTERFACE);

        const std::string configured = DEFAULT_CHASSIS;
        if (!configured.empty() &&
            (std::ranges::contains(chassisPaths, configured) ||
             std::ranges::contains(boardPaths, configured)))
        {
            co_return configured;
        }

        auto candidates = chassisPaths.empty() ? boardPaths : chassisPaths;
        if (candidates.empty())
        {
            co_return std::string{};
        }

        // Prefer top-level objects: entity-manager topology gives a contained
        // chassis a "<path>/contained_by" association (e.g. YV4's slots).
        auto associations = co_await inventoryPaths(ASSOCIATION_INTERFACE);
        std::vector<std::string> topLevel;
        std::ranges::copy_if(candidates, std::back_inserter(topLevel),
                             [&](const std::string& path) {
            return !std::ranges::contains(associations,
                                          path + "/contained_by");
        });
        if (!topLevel.empty())
        {
            candidates = std::move(topLevel);
        }

        co_return std::ranges::min(candidates);
    }

    // Called for every inventory change. Starts refreshChassis() unless it is
    // already running, in which case the new generation makes it wait longer.
    void requestChassisRefresh()
    {
        ++inventoryGeneration;
        if (!chassisRefreshRunning)
        {
            chassisRefreshRunning = true;
            ctx.spawn(refreshChassis());
        }
    }

    // Rediscovers the chassis once inventory has been quiet for
    // CHASSIS_SETTLE_TIME. Tasks on the context only interleave at co_await,
    // so inventoryGeneration can only change while this is suspended.
    sdbusplus::async::task<> refreshChassis()
    {
        while (true)
        {
            const auto generation = inventoryGeneration;

            co_await sdbusplus::async::sleep_for(ctx, CHASSIS_SETTLE_TIME);
            if (generation != inventoryGeneration)
            {
                continue; // still changing
            }

            auto path = co_await discoverChassis();
            if (generation != inventoryGeneration)
            {
                continue; // changed during discovery; the result may be stale
            }

            setChassis(path);
            break;
        }
        chassisRefreshRunning = false;
    }

    void setChassis(const std::string& path)
    {
        if (path == chassis)
        {
            return;
        }
        if (path.empty())
        {
            warning("No inventory chassis found, adhoc sensors have no chassis "
                    "association");
        }
        else
        {
            info("Associating adhoc sensors with chassis {CHASSIS}", "CHASSIS",
                 path);
        }
        chassis = path;
        for (auto& [name, sensor] : sensors)
        {
            sensor.object->associations<true>(chassisAssociations());
        }
    }

    std::vector<std::tuple<std::string, std::string, std::string>>
        chassisAssociations() const
    {
        if (chassis.empty())
        {
            return {};
        }
        return {{"chassis", "all_sensors", chassis}};
    }

    sdbusplus::async::context& ctx;
    std::string selfName;
    struct Sensor
    {
        std::unique_ptr<AdhocSensorObject> object;
        const SensorType* type;
        std::string path;
    };
    // Keyed by file name.
    std::unordered_map<std::string, Sensor> sensors;
    // Files whose sensor path another service owns, until they are deleted.
    std::unordered_set<std::string> blockedFiles;
    struct Metric
    {
        std::unique_ptr<AdhocMetricObject> object;
        MetricUnit unit;
    };
    std::unordered_map<std::string, Metric> metrics;

    std::string chassis;
    // Bumped on every inventory change; see requestChassisRefresh().
    uint64_t inventoryGeneration = 0;
    bool chassisRefreshRunning = false;

    // Inotify members
    int inotifyFd;
    int adhocWd = -1;
    int metricsWd = -1;
};

int main()
{
    sdbusplus::async::context ctx;

    ctx.request_name(SENSOR_BUSNAME);
    sdbusplus::server::manager_t manager{ctx, SENSOR_NAMESPACE};
    sdbusplus::server::manager_t metricManager{ctx, METRIC_NAMESPACE};

    info("Adhoc sensor service started");
    info("Watching directory: {DIR} (file contents = numeric sensor value, "
         "optional unit on line 2, default percent)",
         "DIR", ADHOC_DIR);
    info("Watching directory: {DIR} (file contents = numeric metric value, "
         "optional unit on line 2)",
         "DIR", METRICS_DIR);

    SensorManager sensorManager(ctx);
    ctx.spawn(sensorManager.monitorChassis());
    ctx.spawn(sensorManager.monitorSensorCollisions());

    ctx.run();

    return 0;
}

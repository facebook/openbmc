#include "config.h"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>
#include <sdbusplus/server/manager.hpp>
#include <xyz/openbmc_project/Association/Definitions/aserver.hpp>
#include <xyz/openbmc_project/Sensor/Value/aserver.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <chrono>
#include <sys/inotify.h>
#include <unistd.h>
#include <poll.h>
#include <limits>
#include <cmath>

PHOSPHOR_LOG2_USING;

constexpr const char* SENSOR_BUSNAME = "xyz.openbmc_project.AdhocSensor";
constexpr const char* SENSOR_NAMESPACE = "/xyz/openbmc_project/sensors/utilization";
constexpr const char* ADHOC_DIR = "/run/openbmc/sensors/utilization";

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

class SensorManager
{
  public:
    explicit SensorManager(sdbusplus::async::context& ctx) :
        ctx(ctx),
        inotifyFd(-1)
    {
        // Create watch directory if it doesn't exist
        std::filesystem::create_directories(ADHOC_DIR);

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

    void addSensor(const std::string& sensorName)
    {
        auto it = sensors.find(sensorName);
        if (it == sensors.end())
        {
            std::string path =
                std::string(SENSOR_NAMESPACE) + "/" + sensorName;

            auto object =
                std::make_unique<AdhocSensorObject>(ctx, path.c_str());

            // Set initial properties without signaling
            constexpr bool emitSignal = false;
            object->unit<emitSignal>(
                sdbusplus::common::xyz::openbmc_project::sensor::
                    Value::Unit::Percent);
            object->max_value<emitSignal>(100.0);
            object->min_value<emitSignal>(0.0);
            object->value<emitSignal>(0.0);

            // Set chassis association
            std::vector<std::tuple<std::string, std::string, std::string>>
                associations;
            associations.emplace_back("chassis", "all_sensors",
                                      DEFAULT_CHASSIS);
            object->associations<emitSignal>(associations);

            // Announce the object on D-Bus
            object->emit_added();

            sensors[sensorName] = std::move(object);

            info("Created adhoc sensor: {SENSOR} at {PATH} associated with "
                 "chassis: {CHASSIS}",
                 "SENSOR", sensorName, "PATH", path, "CHASSIS",
                 std::string(DEFAULT_CHASSIS));
        }
    }

    void removeSensor(const std::string& sensorName)
    {
        auto it = sensors.find(sensorName);
        if (it != sensors.end())
        {
            info("Removing sensor: {SENSOR}", "SENSOR", sensorName);
            sensors.erase(it);
        }
    }

    void setSensorValue(const std::string& sensorName, double value)
    {
        auto it = sensors.find(sensorName);
        if (it != sensors.end())
        {
            // Clamp or pass through NaN
            if (!std::isnan(value))
            {
                value = std::max(0.0, std::min(100.0, value));
            }
            it->second->value<true>(value);
        }
        else
        {
            error("Sensor not found: {SENSOR}", "SENSOR", sensorName);
        }
    }

    double readAdhocFromFile(const std::filesystem::path& filePath)
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

        // Log warnings for out-of-range values (will be clamped in
        // setSensorValue)
        if (value < 0.0)
        {
            warning("Negative value {VALUE} in file {FILE}, clamping to 0.0",
                    "VALUE", value, "FILE", filePath.string());
        }
        else if (value > 100.0)
        {
            warning(
                "Value {VALUE} exceeds 100 in file {FILE}, clamping to 100.0",
                "VALUE", value, "FILE", filePath.string());
        }

        return value;
    }

    void scanDirectory()
    {
        std::unordered_set<std::string> currentFiles;

        try
        {
            if (std::filesystem::exists(ADHOC_DIR))
            {
                for (const auto& entry :
                     std::filesystem::directory_iterator(ADHOC_DIR))
                {
                    if (entry.is_regular_file())
                    {
                        std::string filename =
                            entry.path().filename().string();
                        // Append _PCT suffix to conform with Meta standards
                        std::string sensorName = filename + "_PCT";
                        currentFiles.insert(sensorName);

                        // Add sensor if it doesn't exist
                        if (sensors.find(sensorName) == sensors.end())
                        {
                            info("Detected new adhoc file: {FILE}, creating "
                                 "sensor: {SENSOR}",
                                 "FILE", filename, "SENSOR", sensorName);
                            addSensor(sensorName);
                        }

                        // Read value from file
                        double value = readAdhocFromFile(entry.path());
                        setSensorValue(sensorName, value);
                    }
                }
            }

            // Remove sensors for files that no longer exist
            std::vector<std::string> sensorsToRemove;
            for (const auto& [sensorName, sensor] : sensors)
            {
                if (currentFiles.find(sensorName) == currentFiles.end())
                {
                    sensorsToRemove.push_back(sensorName);
                }
            }

            for (const auto& sensorName : sensorsToRemove)
            {
                info("Adhoc file removed for sensor: {SENSOR}, removing "
                     "sensor",
                     "SENSOR", sensorName);
                removeSensor(sensorName);
            }
        }
        catch (const std::exception& e)
        {
            error("Error scanning adhoc directory: {ERROR}", "ERROR",
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

        // Do initial scan
        scanDirectory();

        // Start async monitoring
        ctx.spawn(monitorInotify());
    }

    sdbusplus::async::task<> monitorInotify()
    {
        while (true)
        {
            // Use poll to wait for inotify events
            struct pollfd pfd;
            pfd.fd = inotifyFd;
            pfd.events = POLLIN;

            // Poll with a timeout to allow cooperative multitasking
            int ret = poll(&pfd, 1, 100); // 100ms timeout

            if (ret > 0 && (pfd.revents & POLLIN))
            {
                // Read and process events
                char buffer[4096];
                ssize_t bytesRead = read(inotifyFd, buffer, sizeof(buffer));

                if (bytesRead > 0)
                {
                    size_t offset = 0;
                    while (offset < static_cast<size_t>(bytesRead))
                    {
                        const auto* event =
                            reinterpret_cast<const inotify_event*>(buffer +
                                                                   offset);

                        // Only process regular file events (ignore
                        // directories)
                        if (!(event->mask & IN_ISDIR) && event->len > 0)
                        {
                            std::string filename(event->name);
                            info("Inotify event for adhoc file: {FILE}",
                                 "FILE", filename);
                            scanDirectory();
                        }

                        offset += sizeof(inotify_event) + event->len;
                    }
                }
            }

            // Yield to allow other tasks to run
            co_await sdbusplus::async::sleep_for(
                ctx, std::chrono::milliseconds(10));
        }
    }

  private:
    sdbusplus::async::context& ctx;
    std::unordered_map<std::string, std::unique_ptr<AdhocSensorObject>>
        sensors;

    // Inotify members
    int inotifyFd;
    int adhocWd = -1;
};

int main()
{
    sdbusplus::async::context ctx;

    ctx.request_name(SENSOR_BUSNAME);
    sdbusplus::server::manager_t manager{ctx, SENSOR_NAMESPACE};

    info("Adhoc sensor service started");
    info("Watching directory: {DIR} (file contents = numeric value 0-100)",
         "DIR", ADHOC_DIR);

    SensorManager sensorManager(ctx);

    ctx.run();

    return 0;
}

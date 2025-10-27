#include <redfish_client/core/log_service_handler.hpp>

#include <nlohmann/json.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Logging/Create/client.hpp>
#include <xyz/openbmc_project/Logging/Create/common.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>
#include <xyz/openbmc_project/Logging/event.hpp>

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>

#include <gtest/gtest.h>

namespace redfish_client::core
{

using LoggingLevel =
    sdbusplus::common::xyz::openbmc_project::logging::Entry::Level;

using CreateSyncIntf = sdbusplus::server::object_t<
    sdbusplus::server::xyz::openbmc_project::logging::Create>;

struct Log
{
    std::string message;
    LoggingLevel severity;
    std::map<std::string, std::string> additionalData;
};

class Create : public CreateSyncIntf
{
  public:
    Create(sdbusplus::bus_t& bus, const char* path,
           std::shared_ptr<std::vector<Log>> logs) :
        CreateSyncIntf(bus, path), logs(logs)
    {}

    sdbusplus::message::object_path create(
        std::string message, LoggingLevel severity,
        std::map<std::string, std::string> additionalData) override
    {
        logs->push_back({.message = message,
                         .severity = severity,
                         .additionalData = additionalData});
        // Instance path: "/xyz/openbmc_project/logging"
        std::string pathStr = Create::instance_path;
        pathStr += "/entry/";
        pathStr += std::to_string(nextId++);
        sdbusplus::message::object_path path(pathStr);
        return path;
    }

    void createWithFFDCFiles(
        std::string, LoggingLevel, std::map<std::string, std::string>,
        std::vector<std::tuple<FFDCFormat, uint8_t, uint8_t,
                               sdbusplus::message::unix_fd>>) override
    {
        // This test does not exercise the FFDC code path.
        ASSERT_TRUE(false);
    }

  private:
    int64_t nextId = 1;
    std::shared_ptr<std::vector<Log>> logs;
};

class FakeLogManager
{
  public:
    std::shared_ptr<std::vector<Log>> logs;

    FakeLogManager() : logs(std::make_shared<std::vector<Log>>()) {}

    void start()
    {
        ASSERT_FALSE(serverRunning);
        ASSERT_TRUE(serverThread == nullptr);

        std::mutex lock;
        std::condition_variable condVar;
        bool serverStarted = false;

        serverRunning = true;
        serverThread = std::make_unique<std::thread>([this, &lock, &condVar,
                                                      &serverStarted]() {
            auto bus = sdbusplus::bus::new_default();

            // Instance path: "/xyz/openbmc_project/logging"
            sdbusplus::server::manager_t mgr{bus, Create::instance_path};

            // Default service: "xyz.openbmc_project.Logging"
            bus.request_name(Create::default_service);

            // Instance path: "/xyz/openbmc_project/logging"
            Create impl{bus, Create::instance_path, logs};

            // Notify the client thread.
            {
                {
                    std::lock_guard<std::mutex> guard(lock);
                    serverStarted = true;
                }
                condVar.notify_all();
            }

            // Handle dbus processing while the server is running.
            sdbusplus::SdBusDuration timeout = std::chrono::milliseconds(100);
            while (serverRunning)
            {
                bus.process_discard();
                bus.wait(timeout);
            }
        });

        // Wait for the server to start.
        {
            std::unique_lock<std::mutex> ulock(lock);
            condVar.wait(ulock, [this, &lock, &condVar, &serverStarted] {
                return serverStarted;
            });
        }
    }

    void stop()
    {
        ASSERT_TRUE(serverThread != nullptr);
        ASSERT_TRUE(serverRunning);

        serverRunning = false;
        serverThread->join();
        serverThread = nullptr;
    }

  private:
    bool serverRunning = false;
    std::unique_ptr<std::thread> serverThread;
};

class LogServiceHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        logManager.start();
    }

    void TearDown() override
    {
        logManager.stop();
    }

    FakeLogManager logManager;
};

static constexpr const char* kEventlogEntryCollectionJson = R"(
{
    "@odata.id": "/redfish/v1/Systems/SomeBaseBoard/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Members@odata.count": 2,
    "Members": [
        {
            "@odata.id": "/redfish/v1/Managers/System0/LogServices/EventLog/Entries/101",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "Created": "2025-01-01T12:00:00+00:00",
            "EntryType": "Event",
            "Id": "101",
            "Message": "CPU sensor crossed a warning high threshold going high. Reading=100.123456 Threshold=80.000000.",
            "MessageArgs": [
                "CPU",
                "100.123456",
                "80.000000"
            ],
            "MessageId": "OpenBMC.0.4.SensorThresholdWarningHighGoingHigh",
            "Name": "Manager Event Log Entry",
            "Resolution": "None",
            "Resolved": false,
            "Severity": "Warning"
        },
        {
            "@odata.id": "/redfish/v1/Systems/System0/LogServices/EventLog/Entries/102",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "CPER": {
                "NotificationType": "3d61a466-ab40-409a-a698-f362d464b38f",
                "SectionType": "6d5244f2-2712-11ec-bea7-cb3fdb95c786"
            },
            "Created": "2025-01-01T12:00:00+00:00",
            "DiagnosticDataType": "CPERSection",
            "EntryType": "Event",
            "Id": "102",
            "Links": {
                "OriginOfCondition": {
                    "@odata.id": "/redfish/v1/Systems/System0/Processors/CPU_1"
                }
            },
            "Message": "A platform error occurred.",
            "MessageArgs": [],
            "MessageId": "Platform.1.0.PlatformError",
            "Name": "System Event Log Entry",
            "Resolution": "Check additional diagnostic data if available.",
            "Resolved": false,
            "Severity": "Critical"
        }
    ]
}
)";

TEST_F(LogServiceHandlerTest, BasicTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url");

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kEventlogEntryCollectionJson);

    using namespace std::string_literals;

    logServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());

    // Validate record at index 0 (Unexpected Exception).
    {
        Log log = (*logManager.logs)[0];
        EXPECT_EQ(LoggingLevel::Warning, log.severity);
        EXPECT_EQ("com.meta.RedfishClient.UnexpectedException"s, log.message);

        auto event = nlohmann::json::parse(log.additionalData["REDFISH_EVENT"]);

        EXPECT_EQ("OpenBMC.0.4.SensorThresholdWarningHighGoingHigh"s,
                  event.at("MessageId").get<std::string>());
    }

    // Validate record at index 1 (CPER event).
    {
        Log log = (*logManager.logs)[1];
        EXPECT_EQ(LoggingLevel::Critical, log.severity);
        EXPECT_EQ("xyz.openbmc_project.State.CPER.GenericCPERFault"s,
                  log.message);

        EXPECT_EQ("/redfish/v1/Systems/System0/Processors/CPU_1"s,
                  log.additionalData["SOURCE"]);

        auto cper = nlohmann::json::parse(log.additionalData["CPER"]);

        EXPECT_EQ("3d61a466-ab40-409a-a698-f362d464b38f"s,
                  cper.at("NotificationType").get<std::string>());
    }
}

TEST_F(LogServiceHandlerTest, InMemoryPersistTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url");
    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kEventlogEntryCollectionJson);
    EXPECT_EQ(0, logManager.logs->size());
    logServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());
    logServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());
    // mimic a restart
    auto restartedLogServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url");
    restartedLogServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(4, logManager.logs->size());
    restartedLogServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(4, logManager.logs->size());
}

TEST_F(LogServiceHandlerTest, OnFilePersistTest)
{
    sdbusplus::async::context ctx;
    std::string tmpdir = std::tmpnam(nullptr);
    std::filesystem::create_directory(tmpdir);
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", tmpdir);
    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kEventlogEntryCollectionJson);
    EXPECT_EQ(0, logManager.logs->size());
    logServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());
    logServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());
    // mimic a restart
    auto restartedLogServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", tmpdir);
    restartedLogServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());
    restartedLogServiceHandler->commit(logEntryCollection);
    EXPECT_EQ(2, logManager.logs->size());
    std::filesystem::remove_all(tmpdir);
}

} // namespace redfish_client::core

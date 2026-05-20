#include <redfish_client/core/log_service_handler.hpp>
#include <redfish_client/core/log_entry_mapper_registry.hpp>
#include <redfish_client/core/unhandled_mapper.hpp>
#include <redfish_client/core/cper_mapper.hpp>
#include <redfish_client/core/hgx_ps_run_pwr_fault_mapper.hpp>
#include <redfish_client/core/hgx_thermal_mapper.hpp>

#include <nlohmann/json.hpp>
#include <sdbusplus/server.hpp>
#include <xyz/openbmc_project/Logging/Create/client.hpp>
#include <xyz/openbmc_project/Logging/Create/common.hpp>
#include <xyz/openbmc_project/Logging/Create/server.hpp>
#include <xyz/openbmc_project/Logging/event.hpp>

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <format>
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

void runAsync(sdbusplus::async::context& ctx, sdbusplus::async::task<> task)
{
  ctx.spawn(
      [](sdbusplus::async::task<> task,
        sdbusplus::async::context& ctx) -> sdbusplus::async::task<> {
        co_await std::move(task);
        ctx.request_stop();
      }(std::move(task), ctx));
  ctx.run();
}

class LogServiceHandlerTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        auto& registry = core::LogEntryMapperRegistry::instance();
        registry.registerMapper(std::make_unique<HgxPsRunPwrFaultMapper>(), 110);
        registry.registerMapper(std::make_unique<HgxThermalMapper>(), 105);
        registry.registerMapper(std::make_unique<CperMapper>(), 100);
        registry.registerMapper(std::make_unique<UnhandledMapper>(), 0);
        logManager.start();
        tmpdir = std::tmpnam(nullptr);
        std::filesystem::create_directory(tmpdir);
    }

    void TearDown() override
    {
        logManager.stop();
        std::filesystem::remove_all(tmpdir);
    }

    FakeLogManager logManager;
    std::string tmpdir;
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
        std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kEventlogEntryCollectionJson);

    using namespace std::string_literals;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
      co_await logServiceHandler->commit(logEntryCollection);
    }());
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
        std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);
    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kEventlogEntryCollectionJson);

    size_t numInitial = 0, numAfterFirstCall = 0,
           numAfterDuplication = 0, numAfterRestart = 0,
           numAfterRestartDuplication = 0;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        numInitial = logManager.logs->size();
        co_await logServiceHandler->commit(logEntryCollection);
        numAfterFirstCall = logManager.logs->size();
        co_await logServiceHandler->commit(logEntryCollection);
        numAfterDuplication = logManager.logs->size();
        // mimic a restart
        auto restartedLogServiceHandler =
            std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);
        co_await restartedLogServiceHandler->commit(logEntryCollection);
        numAfterRestart = logManager.logs->size();
        co_await restartedLogServiceHandler->commit(logEntryCollection);
        numAfterRestartDuplication = logManager.logs->size();
    }());

    EXPECT_EQ(0, numInitial);
    EXPECT_EQ(2, numAfterFirstCall);
    EXPECT_EQ(2, numAfterDuplication);
    EXPECT_EQ(4, numAfterRestart);
    EXPECT_EQ(4, numAfterRestartDuplication);
}

TEST_F(LogServiceHandlerTest, OnFilePersistTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", std::nullopt, tmpdir);
    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kEventlogEntryCollectionJson);

    size_t numInitial = 0, numAfterFirstCall = 0,
           numAfterDuplication = 0, numAfterRestart = 0,
           numAfterRestartDuplication = 0;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        numInitial = logManager.logs->size();
        co_await logServiceHandler->commit(logEntryCollection);
        numAfterFirstCall = logManager.logs->size();
        co_await logServiceHandler->commit(logEntryCollection);
        numAfterDuplication = logManager.logs->size();
        // mimic a restart
        auto restartedLogServiceHandler = std::make_shared<LogServiceHandler>(
            ctx, "https://fake.url/redfish/v1", std::nullopt, tmpdir);
        co_await restartedLogServiceHandler->commit(logEntryCollection);
        numAfterRestart = logManager.logs->size();
        co_await restartedLogServiceHandler->commit(logEntryCollection);
        numAfterRestartDuplication = logManager.logs->size();
    }());

    EXPECT_EQ(0, numInitial);
    EXPECT_EQ(2, numAfterFirstCall);
    EXPECT_EQ(2, numAfterDuplication);
    EXPECT_EQ(2, numAfterRestart);
    EXPECT_EQ(2, numAfterRestartDuplication);
}

// Helper to generate a log entry collection JSON from a list of {id, timestamp}
// pairs, simulating incremental Redfish log entry responses.
static std::string makeLogEntryCollectionJson(
    const std::vector<std::pair<std::string, std::string>>& entries)
{
    nlohmann::json members = nlohmann::json::array();
    for (const auto& [id, timestamp] : entries)
    {
        members.push_back({
            {"@odata.id",
             "/redfish/v1/Systems/System0/LogServices/EventLog/Entries/" + id},
            {"@odata.type", "#LogEntry.v1_15_0.LogEntry"},
            {"Created", timestamp},
            {"EntryType", "Event"},
            {"Id", id},
            {"Message", "Test message"},
            {"MessageArgs", nlohmann::json::array()},
            {"MessageId", "OpenBMC.0.4.TestMessage"},
            {"Name", "System Event Log Entry"},
            {"Severity", "Warning"},
        });
    }
    nlohmann::json collection = {
        {"@odata.id",
         "/redfish/v1/Systems/System0/LogServices/EventLog/Entries"},
        {"@odata.type", "#LogEntryCollection.LogEntryCollection"},
        {"Members@odata.count", members.size()},
        {"Members", members},
    };
    return collection.dump();
}

TEST_F(LogServiceHandlerTest, SkipHistoricalEntriesNoPersistFileTest)
{
    // No persist file + threshold configured → old entries should be skipped
    sdbusplus::async::context ctx;
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", 60, tmpdir);

    auto oldTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(120));

    // First poll: two old entries
    auto logEntryCollection1 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp},
                                        {"202", oldTimestamp}}));

    // Second poll: same old entries (dedup test)
    auto logEntryCollection2 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp},
                                        {"202", oldTimestamp}}));

    // Third poll: same old entries plus a new entry with old timestamp
    auto logEntryCollection3 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp},
                                        {"202", oldTimestamp},
                                        {"203", oldTimestamp}}));

    size_t numAfterFirstCommit = 0, numAfterSecondCommit = 0,
           numAfterThirdCommit = 0;
    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        co_await logServiceHandler->commit(logEntryCollection1);
        numAfterFirstCommit = logManager.logs->size();
        // Simulate next polling cycle with the same old entries
        co_await logServiceHandler->commit(logEntryCollection2);
        numAfterSecondCommit = logManager.logs->size();
        // Simulate a new entry arriving in a later polling cycle —
        // historical filtering only applies on the first commit, so new
        // entries should be committed regardless of their timestamp
        co_await logServiceHandler->commit(logEntryCollection3);
        numAfterThirdCommit = logManager.logs->size();
    }());

    // Old entries should be skipped on first commit
    EXPECT_EQ(0, numAfterFirstCommit);
    // Old entries should not reappear on subsequent commits (deduped via
    // persist map)
    EXPECT_EQ(0, numAfterSecondCommit);
    // New entry (203) in later polling cycle should be committed regardless of
    // timestamp; existing entries (201, 202) deduped
    EXPECT_EQ(1, numAfterThirdCommit);
}

TEST_F(LogServiceHandlerTest, AllowRecentEntriesNoPersistFileTest)
{
    // No persist file + threshold configured → recent entries should be
    // committed
    sdbusplus::async::context ctx;
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", 60, tmpdir);

    auto recentTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(10));

    auto logEntryCollection1 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", recentTimestamp}}));

    size_t numAfterCommit = 0;
    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        co_await logServiceHandler->commit(logEntryCollection1);
        numAfterCommit = logManager.logs->size();
    }());

    // Recent entry should be committed
    EXPECT_EQ(1, numAfterCommit);
}

TEST_F(LogServiceHandlerTest, NoFilteringWhenPersistFileExistsTest)
{
    // Persist file exists + threshold configured → no filtering, normal dedup
    sdbusplus::async::context ctx;

    auto oldTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(120));

    // First handler: no persist file, threshold configured → old entries
    // skipped but still recorded in persist map
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", 60, tmpdir);
    auto logEntryCollection1 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp}}));

    // Same entries for the restarted handler
    auto logEntryCollection2 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp}}));

    size_t numAfterFirstHandler = 0, numAfterRestart = 0;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        co_await logServiceHandler->commit(logEntryCollection1);
        numAfterFirstHandler = logManager.logs->size();

        // Restart with threshold configured — persist file now exists
        auto restartedHandler = std::make_shared<LogServiceHandler>(
            ctx, "https://fake.url/redfish/v1", 60, tmpdir);
        co_await restartedHandler->commit(logEntryCollection2);
        numAfterRestart = logManager.logs->size();
    }());

    // First handler: no persist file, entries are old → skipped
    EXPECT_EQ(0, numAfterFirstHandler);
    // Restarted handler: persist file exists → no filtering, entries already in
    // map → deduped
    EXPECT_EQ(0, numAfterRestart);
}

TEST_F(LogServiceHandlerTest, NoFilteringWhenThresholdDisabledTest)
{
    // No persist file + threshold explicitly disabled (nullopt) → all entries
    // should be committed regardless of age
    sdbusplus::async::context ctx;
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", std::nullopt, tmpdir);

    auto oldTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(120));

    auto logEntryCollection1 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp},
                                        {"202", oldTimestamp}}));

    size_t numAfterCommit = 0;
    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        co_await logServiceHandler->commit(logEntryCollection1);
        numAfterCommit = logManager.logs->size();
    }());

    // Threshold disabled, all entries should be committed
    EXPECT_EQ(2, numAfterCommit);
}

TEST_F(LogServiceHandlerTest, CorruptedPersistFileTest)
{
    // Corrupted persist file + threshold configured → treated as no persist
    // file, so historical filtering should apply
    sdbusplus::async::context ctx;

    auto oldTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(120));

    // Write a corrupted persist file
    auto handler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", 60, tmpdir);
    auto persistPath = tmpdir + "/https___fake_url_redfish_v1";
    std::ofstream file(persistPath);
    file << "corrupted";
    file.close();

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp}}));

    size_t numAfterCommit = 0;
    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        co_await handler->commit(logEntryCollection);
        numAfterCommit = logManager.logs->size();
    }());

    // Corrupted file treated as no persist file → old entries skipped
    EXPECT_EQ(0, numAfterCommit);
}

TEST_F(LogServiceHandlerTest, MixedOldAndRecentEntriesTest)
{
    // No persist file + threshold configured + mix of old and recent entries
    // → only recent entries should be committed on the first commit
    sdbusplus::async::context ctx;
    auto logServiceHandler = std::make_shared<LogServiceHandler>(
        ctx, "https://fake.url/redfish/v1", 60, tmpdir);

    auto oldTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(120));
    auto recentTimestamp = std::format("{:%FT%T%Ez}",
        std::chrono::system_clock::now() - std::chrono::seconds(10));

    auto logEntryCollection1 =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            makeLogEntryCollectionJson({{"201", oldTimestamp},
                                        {"202", recentTimestamp},
                                        {"203", oldTimestamp}}));

    size_t numAfterCommit = 0;
    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
        co_await logServiceHandler->commit(logEntryCollection1);
        numAfterCommit = logManager.logs->size();
    }());

    // Only the recent entry (202) should be committed; old entries skipped
    EXPECT_EQ(1, numAfterCommit);
}

static constexpr const char* kPsRunPwrFaultEntryCollectionJson = R"(
{
    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Members@odata.count": 1,
    "Members": [
        {
            "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries/4308",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "AdditionalDataURI": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries/4308/attachment",
            "Created": "2025-12-27T18:17:30+00:00",
            "EntryType": "Event",
            "Id": "4308",
            "Links": {
                "OriginOfCondition": {
                    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0"
                }
            },
            "Message": "The resource property PS_RUN_PWR_FAULT has detected errors of type 'PWR_FAIL_GPU{0x1};PWRSEQ_FAIL_STATE{0xf}'.",
            "MessageArgs": [
                "PS_RUN_PWR_FAULT",
                "PWR_FAIL_GPU{0x1};PWRSEQ_FAIL_STATE{0xf}"
            ],
            "MessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
            "Name": "System Event Log Entry",
            "Oem": {
                "Nvidia": {
                    "@odata.type": "#NvidiaLogEntry.v1_1_0.NvidiaLogEntry",
                    "Device": "Power_0",
                    "ErrorId": "PS_RUN_PWR_FAULT-ERROR"
                }
            },
            "Resolution": "AC cycle the system. If problem persists, collect all dumps per troubleshooting guide.",
            "Resolved": false,
            "Severity": "Critical"
        }
    ]
}
)";

TEST_F(LogServiceHandlerTest, PsRunPwrFaultMapperTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kPsRunPwrFaultEntryCollectionJson);

    using namespace std::string_literals;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
      co_await logServiceHandler->commit(logEntryCollection);
    }());

    // Expect 2 logs: one PowerRailFault for each power state in the message
    std::vector<std::string> expectedPowerRails = {
        "/xyz/openbmc_project/inventory/system/board/NVIDIA_HMC/PWR_FAIL_GPU"s,
        "/xyz/openbmc_project/inventory/system/board/NVIDIA_HMC/PWRSEQ_FAIL_STATE"s};

    ASSERT_EQ(expectedPowerRails.size(), logManager.logs->size());

    for (size_t i = 0; i < expectedPowerRails.size(); ++i)
    {
        SCOPED_TRACE("Validating log at index " + std::to_string(i));

        Log log = (*logManager.logs)[i];
        EXPECT_EQ(LoggingLevel::Error, log.severity);
        EXPECT_EQ("xyz.openbmc_project.State.Power.PowerRailFault"s,
                  log.message);
        EXPECT_EQ(expectedPowerRails[i], log.additionalData["POWER_RAIL"]);

        auto failureData =
            nlohmann::json::parse(log.additionalData["FAILURE_DATA"]);

        EXPECT_EQ("The resource property PS_RUN_PWR_FAULT has detected errors "
                  "of type 'PWR_FAIL_GPU{0x1};PWRSEQ_FAIL_STATE{0xf}'."s,
                  failureData.at("Message").get<std::string>());
        EXPECT_EQ("Critical"s, failureData.at("Severity").get<std::string>());
        EXPECT_EQ("2025-12-27T18:17:30+00:00"s,
                  failureData.at("Created").get<std::string>());
    }
}

static constexpr const char* kPsRunPwrFaultEmptyMessageArgsJson = R"(
{
    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Members@odata.count": 1,
    "Members": [
        {
            "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries/4309",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "Created": "2025-12-27T18:17:30+00:00",
            "EntryType": "Event",
            "Id": "4309",
            "Message": "The resource property PS_RUN_PWR_FAULT has detected errors.",
            "MessageArgs": [],
            "MessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
            "Name": "System Event Log Entry",
            "Severity": "Critical"
        }
    ]
}
)";

TEST_F(LogServiceHandlerTest, PsRunPwrFaultEmptyMessageArgsTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kPsRunPwrFaultEmptyMessageArgsJson);

    using namespace std::string_literals;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
      co_await logServiceHandler->commit(logEntryCollection);
    }());

    // With empty MessageArgs, HgxPsRunPwrFaultMapper should not handle,
    // falling through to UnhandledMapper
    ASSERT_EQ(1, logManager.logs->size());

    Log log = (*logManager.logs)[0];
    EXPECT_EQ(LoggingLevel::Critical, log.severity);
    EXPECT_EQ("com.meta.RedfishClient.UnexpectedException"s, log.message);
}

static constexpr const char* kPsRunPwrFaultInsufficientMessageArgsJson = R"(
{
    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Members@odata.count": 1,
    "Members": [
        {
            "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries/4310",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "Created": "2025-12-27T18:17:30+00:00",
            "EntryType": "Event",
            "Id": "4310",
            "Message": "The resource property PS_RUN_PWR_FAULT has detected errors.",
            "MessageArgs": [
                "PS_RUN_PWR_FAULT"
            ],
            "MessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
            "Name": "System Event Log Entry",
            "Severity": "Critical"
        }
    ]
}
)";

TEST_F(LogServiceHandlerTest, PsRunPwrFaultInsufficientMessageArgsTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kPsRunPwrFaultInsufficientMessageArgsJson);

    using namespace std::string_literals;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
      co_await logServiceHandler->commit(logEntryCollection);
    }());

    // With only 1 MessageArg, HgxPsRunPwrFaultMapper should not handle,
    // falling through to UnhandledMapper
    ASSERT_EQ(1, logManager.logs->size());

    Log log = (*logManager.logs)[0];
    EXPECT_EQ(LoggingLevel::Critical, log.severity);
    EXPECT_EQ("com.meta.RedfishClient.UnexpectedException"s, log.message);
}

static constexpr const char* kThermalEntryCollectionJson = R"(
{
    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries",
    "@odata.type": "#LogEntryCollection.LogEntryCollection",
    "Members@odata.count": 2,
    "Members": [
        {
            "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries/1863",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "Id": "1863",
            "Created": "2025-01-01T12:00:00+00:00",
            "EntryType": "Event",
            "MessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
            "MessageArgs": ["GPU_2 THERM_WARN_INT", "Abnormal State Change"],
            "Severity": "Warning",
            "Links": {
                "OriginOfCondition": {
                    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/Processors/GPU_2"
                }
            }
        },
        {
            "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/LogServices/EventLog/Entries/5023",
            "@odata.type": "#LogEntry.v1_15_0.LogEntry",
            "Id": "5023",
            "Created": "2025-01-01T12:00:00+00:00",
            "EntryType": "Event",
            "MessageId": "ResourceEvent.1.0.ResourceErrorsDetected",
            "MessageArgs": ["GPU_SXM_8 ThrottleReason", "HWThermalSlowdown"],
            "Severity": "Critical",
            "Links": {
                "OriginOfCondition": {
                    "@odata.id": "/redfish/v1/Systems/HGX_Baseboard_0/Processors/GPU_SXM_8"
                }
            }
        }
    ]
}
)";

TEST_F(LogServiceHandlerTest, HgxThermalMapperTest)
{
    sdbusplus::async::context ctx;
    auto logServiceHandler =
        std::make_shared<LogServiceHandler>(ctx, "fake.url", std::nullopt);

    auto logEntryCollection =
        redfish_binding::LogEntryCollection::parseLogEntryCollection(
            kThermalEntryCollectionJson);

    using namespace std::string_literals;

    runAsync(ctx, [&]() -> sdbusplus::async::task<> {
      co_await logServiceHandler->commit(logEntryCollection);
    }());

    ASSERT_EQ(2, logManager.logs->size());

    // Verify the first entry: Warning
    Log log1 = (*logManager.logs)[0];
    EXPECT_EQ(LoggingLevel::Error, log1.severity);
    EXPECT_EQ("xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperature"s,
              log1.message);
    EXPECT_EQ("/redfish/v1/Systems/HGX_Baseboard_0/Processors/GPU_2",
              log1.additionalData["DEVICE"]);

    // Verify the second entry: Critical
    Log log2 = (*logManager.logs)[1];
    EXPECT_EQ(LoggingLevel::Critical, log2.severity);
    EXPECT_EQ("xyz.openbmc_project.State.Thermal.DeviceOverOperatingTemperatureFault"s,
              log2.message);
    EXPECT_EQ("/redfish/v1/Systems/HGX_Baseboard_0/Processors/GPU_SXM_8",
              log2.additionalData["DEVICE"]);
}

} // namespace redfish_client::core

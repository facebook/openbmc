#include <CLI/CLI.hpp>
#include <openbmc/hgx.h>
#include <syslog.h>
#include <iostream>

static void do_version(const std::string& component, bool json_fmt) {
  std::cout << hgx::version(component, json_fmt) << std::endl;
}

static void do_update(const std::string& comp, const std::string& path, bool async, bool json_fmt) {
  if (async) {
    std::string id = hgx::updateNonBlocking(comp, path, json_fmt);
    if (!json_fmt) {
      std::cout << "Task ID: " << id << std::endl;
    } else {
      std::cout << id << std::endl;
    }
  } else {
    hgx::update(comp, path);
  }
}

static void do_dump(
    const std::string& comp,
    const std::string& path,
    bool async,
    bool json_fmt) {
  const std::map<std::string, hgx::DiagnosticDataType> compMap = {
      {"hmc", hgx::DiagnosticDataType::MANAGER},
      {"erot", hgx::DiagnosticDataType::OEM_EROT},
      {"self-test", hgx::DiagnosticDataType::OEM_SELF_TEST},
      {"fpga", hgx::DiagnosticDataType::OEM_FPGA},
      {"retimer", hgx::DiagnosticDataType::OEM_RETIMER}};
  if (async) {
    std::cout << hgx::dumpNonBlocking(compMap.at(comp)) << std::endl;
  } else {
    if (path == "") {
      throw std::runtime_error("Image is required if synchronous");
    }
    hgx::dump(compMap.at(comp), path);
  }
}

static void do_dump_retrieve(const std::string& id, const std::string& path) {
  hgx::retrieveDump(id, path);
}

static void do_task_status(const std::string& id, bool json_fmt) {
  hgx::TaskStatus status = hgx::getTaskStatus(id);
  if (json_fmt) {
    std::cout << status.resp << std::endl;
  } else {
    std::cout << "State: " << status.state;
    std::cout << " Status: " << status.status;
  }
}

static void
do_sensor(const std::string& fru, const std::string& name, bool json_fmt) {
  if (json_fmt) {
    std::string val = hgx::sensorRaw(fru, name);
    std::cout << val << std::endl;
  } else {
    float val = hgx::sensor(fru, name);
    std::cout << "Value: " << val << std::endl;
  }
}

static void do_get(const std::string& subpath) {
  std::string out = hgx::redfishGet(subpath);
  std::cout << out << std::endl;
}

static void do_post(const std::string& subpath, std::string& args) {
  std::string out = hgx::redfishPost(subpath, std::move(args));
  std::cout << out << std::endl;
}

static void do_patch(const std::string& url, std::string& args) {
  std::string out = hgx::redfishPatch(url, std::move(args));
  std::cout << out << std::endl;
}

static void do_get_snr_metric() {
  hgx::getMetricReports();
}

static void do_factory_reset() {
  try {
    hgx::factoryReset();
    syslog(LOG_CRIT, "Perform HGX factory reset...");
  } catch (std::exception& e) {
    syslog(LOG_CRIT, "Perform HGX factory reset failed: %s", e.what());
  }
}

static void do_reset() {
  hgx::reset();
}

static void do_event_log(bool clear, bool json) {
  if (clear) {
    hgx::clearEventLog();
  } else {
    hgx::printEventLog(std::cout, json);
  }
}

static void do_list_attest_components(void) {
 auto comps = hgx::integrityComponents();
 for (const auto& comp : comps) {
   std::cout << comp << std::endl;
 }
}

static void do_attestation(const std::string& comp) {
  auto comps = hgx::integrityComponents();
  if (std::find(comps.begin(), comps.end(), comp) == comps.end()) {
    throw std::runtime_error("Unknown component: " + comp);
  }
  std::cout << hgx::getMeasurement(comp) << std::endl;
}

static void do_set_power_limit(std::string pwrLimit) {
  for (int i = 1; i <= MAX_NUM_GPUs; i++) {
    hgx::setPowerLimit(i, pwrLimit);
    std::cout << "Power limit for GPU" << i << " was set to : "
              << pwrLimit << "W" << std::endl;
  }

  std::cout << "All done. It takes a minute to take effect."
            << std::endl;
}

static void do_get_power_limit() {
  for (int i = 1; i <= MAX_NUM_GPUs; i++) {
    std::cout << "GPU" << i << " power limit : " <<
                 hgx::getPowerLimit(i) << "W" << std::endl;
  }
}

int main(int argc, char* argv[]) {
  CLI::App app("HGX Helper Utility");
  app.failure_message(CLI::FailureMessage::help);

  bool json_fmt = false;
  // Allow flags/options to fallthrough from subcommands.
  app.fallthrough();
  app.add_flag("-j,--json", json_fmt, "JSON output instead of text");

  std::string component{};
  auto version = app.add_subcommand("version", "Get Version of a component");
  version->add_option("component", component, "Component to retrieve")
      ->required();
  version->callback([&]() { do_version(component, json_fmt); });

  bool async = false;
  std::string comp{};
  std::string image{};
  auto update = app.add_subcommand("update", "Update the component");
  update->add_option("comp", comp, "Component")->required();
  update->add_option("image", image, "Path to the image")->required();
  update->add_flag(
      "--async", async, "Do not block, return immediately printing the task ID");
  update->callback([&]() { do_update(comp, image, async, json_fmt); });

  std::set<std::string> allowedComps{"hmc", "erot", "self-test", "fpga", "retimer"};
  auto dump = app.add_subcommand("dump", "perform a dump");
  dump->add_option("comp", comp, "What to dump")
      ->required()
      ->check(CLI::IsMember(allowedComps));
  dump->add_option("path", image, "Path to store the dump");
  dump->add_flag(
      "--async",
      async,
      "Do not block, return immediately printing the task ID");
  dump->callback([&]() { do_dump(comp, image, async, json_fmt); });

  auto dump_ret =
      app.add_subcommand("retrieve-dump", "Retrieve a dump given a task ID");
  dump_ret->add_option("taskID", comp, "Task ID got from dump --async")
      ->required();
  dump_ret->add_option("path", image, "Path to store the dump")->required();
  dump_ret->callback([&]() { do_dump_retrieve(comp, image); });

  std::string fru{};
  std::string sensorName{};
  auto sensor = app.add_subcommand("sensor", "Get sensor value");
  sensor->add_option("FRU", fru, "The FRU on which this sensor is")->required();
  sensor->add_option("Sensor", sensorName, "The sensor name")->required();
  sensor->callback([&]() { do_sensor(fru, sensorName, json_fmt); });

  std::string subpath{};
  auto get = app.add_subcommand("get", "Perform a GET on the redfish subpath");
  get->add_option("SUBPATH", subpath, "Subpath after /redfish/v1")->required();
  get->callback([&]() { do_get(subpath); });

  std::string taskID{};
  auto taskid = app.add_subcommand("get-task", "Get Task Status");
  taskid->add_option("id", taskID, "Task ID")->required();
  taskid->callback([&]() { do_task_status(taskID, json_fmt); });

  auto snr_metrics = app.add_subcommand("get-snr-metrics", "Get sensor metrics from Telemetry service");
  snr_metrics->callback([&]() { do_get_snr_metric(); });

  std::string args{};
  auto post =
      app.add_subcommand("post", "Perform a POST on the redfish subpath");
  post->add_option("SUBPATH", subpath, "Subpath after /redfish/v1")->required();
  post->add_option("args", args, "JSON arguments for post")->required();
  post->callback([&]() { do_post(subpath, args); });

  auto patch = app.add_subcommand("patch", "Perform a Patch on the redfish subpath");
  patch->add_option("SUBPATH", subpath, "Subpath after /redfish/v1")->required();
  patch->add_option("args", args, "JSON arguments for patch")->required();
  patch->callback([&]() { do_patch(subpath, args); });

  auto freset = app.add_subcommand("factory-reset", "Perform factory reset to default");
  freset->callback([&]() { do_factory_reset(); });

  auto reset = app.add_subcommand("reset", "Graceful reboot the HMC");
  reset->callback([&]() { do_reset(); });

  bool eventClear = false;
  auto eventlog = app.add_subcommand("event-log", "Retrieve event logs from HGX");
  eventlog->add_flag("--clear", eventClear, "Clears the event Log");
  eventlog->callback([&]() { do_event_log(eventClear, json_fmt); });

  auto timeSync = app.add_subcommand("time-sync", "Sync current BMC time with HGX");
  timeSync->callback([&]() { hgx::syncTime(); });

  auto measurement = app.add_subcommand("attest", "Get SPDM measurements");
  auto listM = measurement->add_subcommand("list", "List components supporting measurements");
  listM->callback([&]() { do_list_attest_components(); });
  auto measure = measurement->add_subcommand("get", "Get measurements");
  measure->add_option("component", args, "Component for which we want the measurements")->required();
  measure->callback([&]() { do_attestation(args); });
  measurement->require_subcommand(1, 1);

  std::string pwrLimit{};
  auto setPwrLimit = app.add_subcommand("set-pwr-limit", "Set power limit from GPU");
  setPwrLimit->add_option("PwrLimit", pwrLimit, "the value of the power limit.")->required();
  setPwrLimit->callback([&]() { do_set_power_limit(pwrLimit); });

  auto getPwrLimit = app.add_subcommand("get-pwr-limit", "Get power limit from GPU");
  getPwrLimit->callback([&]() { do_get_power_limit(); });

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  return 0;
}

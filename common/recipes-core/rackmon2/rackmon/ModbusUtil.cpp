// Copyright 2021-present Facebook. All Rights Reserved.
#include <CLI/CLI.hpp>
#include <sys/file.h>
#include <systemd/sd-bus.h>
#include <unistd.h>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <regex>
#include <thread>
#include "InterfaceScanner.h"
#include "Log.h"
#include "Modbus.h"
#include "ModbusDevice.h"
#include "Msg.h"
#include "UnixSock.h"

using nlohmann::json;

struct ServiceExclusionBase {
  sd_bus* bus = nullptr;
  std::string serviceName;

  explicit ServiceExclusionBase(const std::string& svcName)
      : serviceName(svcName) {
    if (sd_bus_default_system(&bus) < 0) {
      throw std::runtime_error("Failed to open system bus");
    }
  }
  virtual ~ServiceExclusionBase() {
    sd_bus_unref(bus);
  }

  void init() {
    if (!isServiceRunning()) {
      return;
    }
    if (serviceAction(false)) {
      stopped_ = true;
    }
  }
  void deinit() {
    if (stopped_) {
      serviceAction(true);
    }
    stopped_ = false;
  }

 private:
  bool stopped_ = false;

  bool isServiceRunning() {
    auto unitName = serviceName + ".service";
    auto stubbedUnitName =
        std::regex_replace(unitName, std::regex("\\."), "_2e");
    std::string objPath = "/org/freedesktop/systemd1/unit/" + stubbedUnitName;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    char* cstate = nullptr;
    int r = sd_bus_get_property_string(
        bus,
        "org.freedesktop.systemd1",
        objPath.c_str(),
        "org.freedesktop.systemd1.Unit",
        "UnitFileState",
        &error,
        &cstate);
    if (r < 0) {
      sd_bus_error_free(&error);
      return false;
    }
    std::string fileState(cstate);
    free(cstate);
    sd_bus_error_free(&error);
    if (fileState != "enabled") {
      return false;
    }

    error = SD_BUS_ERROR_NULL;
    cstate = nullptr;
    r = sd_bus_get_property_string(
        bus,
        "org.freedesktop.systemd1",
        objPath.c_str(),
        "org.freedesktop.systemd1.Unit",
        "ActiveState",
        &error,
        &cstate);
    if (r < 0) {
      sd_bus_error_free(&error);
      return false;
    }
    std::string activeState(cstate);
    free(cstate);
    sd_bus_error_free(&error);

    if (activeState != "active") {
      return false;
    }
    return true;
  }

 protected:
  virtual bool serviceAction(bool start) = 0;
};

struct RackmonExclusion : public ServiceExclusionBase {
  RackmonExclusion() : ServiceExclusionBase("rackmond") {}
  bool serviceAction(bool start) override {
    json req;
    req["type"] = start ? "resume" : "pause";
    rackmonsvc::RackmonClient cli;
    std::string resp = cli.request(req.dump());
    json resp_j = json::parse(resp);
    std::string status;
    resp_j.at("status").get_to(status);
    if (status != "SUCCESS") {
      std::cerr << "ACTION: " << req["type"] << " failed" << std::endl;
      return false;
    }
    return true;
  }
};

struct PhosphorModbusExclusion : public ServiceExclusionBase {
  static constexpr auto kService = "xyz.openbmc_project.ModbusRTU";
  static constexpr auto kPortNamespace = "/xyz/openbmc_project/control/port";
  static constexpr auto kPortInterface = "xyz.openbmc_project.Control.Port";
  static constexpr auto kMonitoringEnabled = "MonitoringEnabled";
  std::string ttyName;
  std::vector<std::string> changedPaths;

  explicit PhosphorModbusExclusion(const std::string& tty)
      : ServiceExclusionBase(kService),
        ttyName(std::filesystem::path(tty).filename()) {
    // DBus Object paths cannot have -.
    std::replace(ttyName.begin(), ttyName.end(), '-', '_');
  }

  bool changeProperty(const std::string& path, bool start) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    int r = sd_bus_set_property(
        bus,
        kService,
        path.c_str(),
        kPortInterface,
        kMonitoringEnabled,
        &error,
        "b",
        static_cast<int>(start));
    if (r < 0) {
      std::cerr << "Failed to set " << kMonitoringEnabled << " on " << path
                << ": " << (error.message ? error.message : "unknown error")
                << std::endl;
      sd_bus_error_free(&error);
      return false;
    }
    return true;
  }

  bool stopMonitoring() {
    std::vector<std::string> portPaths;
    if (!getPortPaths(portPaths)) {
      return false;
    }
    for (const auto& path : portPaths) {
      if (std::filesystem::path(path).filename() != ttyName) {
        continue;
      }
      if (!changeProperty(path, false)) {
        startMonitoring();
        return false;
      }
      changedPaths.push_back(path);
    }
    return true;
  }

  bool startMonitoring() {
    for (const auto& path : changedPaths) {
      changeProperty(path, true);
    }
    changedPaths.clear();
    return true;
  }

  bool serviceAction(bool start) override {
    return start ? startMonitoring() : stopMonitoring();
  }

 private:
  // Enumerate the serial port objects exported under the port-control
  // namespace via the service's ObjectManager.
  bool getPortPaths(std::vector<std::string>& portPaths) {
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message* reply = nullptr;
    int r = sd_bus_call_method(
        bus,
        kService,
        kPortNamespace,
        "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects",
        &error,
        &reply,
        "");
    if (r < 0) {
      std::cerr << "Failed to enumerate ports under " << kPortNamespace << ": "
                << (error.message ? error.message : "unknown error")
                << std::endl;
      sd_bus_error_free(&error);
      return false;
    }

    r = sd_bus_message_enter_container(
        reply, SD_BUS_TYPE_ARRAY, "{oa{sa{sv}}}");
    if (r < 0) {
      sd_bus_error_free(&error);
      sd_bus_message_unref(reply);
      return false;
    }
    while (sd_bus_message_enter_container(
               reply, SD_BUS_TYPE_DICT_ENTRY, "oa{sa{sv}}") > 0) {
      const char* objPath = nullptr;
      if (sd_bus_message_read(reply, "o", &objPath) < 0) {
        break;
      }
      portPaths.emplace_back(objPath);
      sd_bus_message_skip(reply, "a{sa{sv}}");
      sd_bus_message_exit_container(reply);
    }
    sd_bus_message_exit_container(reply);

    sd_bus_error_free(&error);
    sd_bus_message_unref(reply);
    return true;
  }
};

struct ServiceExclusion {
  std::unique_ptr<RackmonExclusion> rackmonLock;
  std::unique_ptr<PhosphorModbusExclusion> modbusLock;
  ServiceExclusion(const std::string& tty)
      : rackmonLock(std::make_unique<RackmonExclusion>()),
        modbusLock(std::make_unique<PhosphorModbusExclusion>(tty)) {
    rackmonLock->init();
    modbusLock->init();

    // Wait for the service to settle down after we've paused it.
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  ~ServiceExclusion() {
    modbusLock->deinit();
    rackmonLock->deinit();
  }
};

static void devInitialize(rackmon::Modbus& dev, const json& intf) {
  dev.initialize(intf);
  if (!dev.isPresent()) {
    throw std::runtime_error("Device creation failed!");
  }
}

void rawCommand(
    const json& intf,
    const std::string& cmd,
    size_t respLen,
    rackmon::Parity parity,
    int timeout) {
  rackmon::Modbus dev;
  devInitialize(dev, intf);
  rackmon::Msg req;
  rackmon::Msg resp;
  resp.len = respLen;
  for (const char* c = cmd.c_str(); c[0] != '\0' && c[1] != '\0'; c += 2) {
    char v[3] = {c[0], c[1], '\0'};
    req << uint8_t(std::stoi(v, 0, 16));
  }
  try {
    dev.command(req, resp, 0, rackmon::ModbusTime(timeout), parity);
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return;
  }
  // command would strip out CRC, include it to our output so
  // we have everything to debug in case we are debugging CRC
  // issues.
  resp.len += 2;
  std::cout << resp << std::endl;
}

void readCommand(
    const json& intf,
    uint8_t deviceAddr,
    uint16_t registerOffset,
    uint16_t registerCount,
    rackmon::Parity parity,
    int timeout) {
  rackmon::Modbus dev;
  devInitialize(dev, intf);
  std::vector<uint16_t> regs(registerCount);
  rackmon::ReadHoldingRegistersReq req(
      deviceAddr, registerOffset, registerCount);
  rackmon::ReadHoldingRegistersResp resp(deviceAddr, regs);
  try {
    dev.command(req, resp, 0, rackmon::ModbusTime(timeout), parity);
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return;
  }
  for (size_t i = 0; i < regs.size(); i++) {
    std::cout << "0x" << std::hex << std::setfill('0') << std::setw(4)
              << regs[i] << " ";
  }
  std::cout << std::endl;
}

void writeCommand(
    const json& intf,
    uint8_t deviceAddr,
    uint16_t registerOffset,
    std::vector<uint16_t>& values,
    rackmon::Parity parity,
    int timeout) {
  rackmon::Modbus dev;
  devInitialize(dev, intf);
  try {
    if (values.size() == 1) {
      rackmon::WriteSingleRegisterReq req(
          deviceAddr, registerOffset, values[0]);
      rackmon::WriteSingleRegisterResp resp(deviceAddr, registerOffset);
      dev.command(req, resp, 0, rackmon::ModbusTime(timeout), parity);
    } else {
      rackmon::WriteMultipleRegistersReq req(deviceAddr, registerOffset);
      for (uint16_t val : values) {
        req << val;
      }
      rackmon::WriteMultipleRegistersResp resp(
          deviceAddr, registerOffset, values.size());
      dev.command(req, resp, 0, rackmon::ModbusTime(timeout), parity);
    }
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return;
  }
}

json getJSON(const std::string& fileName) {
  std::ifstream ifs(fileName);
  json contents;
  try {
    ifs >> contents;
  } catch (const nlohmann::json::parse_error& ex) {
    logError << "Error loading: " << fileName << " byte: " << ex.byte
             << std::endl;
    throw;
  }
  ifs.close();
  return contents;
};

rackmon::RegisterMapDatabase loadDatabase(const std::string& regMapPath) {
  rackmon::RegisterMapDatabase db;
  if (std::filesystem::is_directory(regMapPath)) {
    for (auto const& dir_entry :
         std::filesystem::directory_iterator{regMapPath}) {
      db.load(getJSON(dir_entry.path().string()));
    }
  } else {
    db.load(getJSON(regMapPath));
  }
  return db;
}

void dataCommand(
    const json& intf,
    const std::string& regMapPath,
    bool readData) {
  auto dev = std::make_shared<rackmon::Modbus>();
  devInitialize(*dev, intf);
  rackmon::RegisterMapDatabase db = loadDatabase(regMapPath);

  rackmon::ModbusDeviceInventory devices;
  rackmon::InterfaceScanner scanner(
      dev, devices, db, rackmon::PollThreadTime(0), true);
  scanner.fullScan();
  json jdata;
  auto allDevices = devices.getAllModbusDevices();
  if (readData) {
    scanner.updateDeviceRegisters();
    std::transform(
        allDevices.begin(),
        allDevices.end(),
        std::back_inserter(jdata),
        [](const auto& kv) { return kv->getValueData(); });
  } else {
    std::transform(
        allDevices.begin(),
        allDevices.end(),
        std::back_inserter(jdata),
        [](const auto& kv) { return kv->getInfo(); });
  }
  std::string sdata = jdata.dump(4);
  std::cout << sdata << std::endl;
}

int main(int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  std::map<std::string, rackmon::Parity> parityMap = {
      {"ODD", rackmon::Parity::ODD},
      {"EVEN", rackmon::Parity::EVEN},
      {"NONE", rackmon::Parity::NONE},
  };

  CLI::App app("Modbus Raw Interface");

  std::string tty{};
  app.add_option("--tty", tty, "TTY Interface to use")->required();
  int minDelay = 2;
  app.add_option(
         "--min-delay", minDelay, "Minimum delay (ms) between transactions")
      ->capture_default_str();
  CLI::App* raw =
      app.add_subcommand("raw", "Execute a Raw command like 0d0300000001");
  std::string cmd{};
  raw->add_option("command", cmd, "Command to send (hex)")->required();
  size_t respLen = 0;
  raw->add_option(
         "-x,--expected-bytes",
         respLen,
         "Expected response length (including 2b CRC)")
      ->required();
  std::string parityStr = "EVEN";
  raw->add_option("--parity", parityStr, "Parity")
      ->check(CLI::IsMember({"ODD", "EVEN", "NONE"}))
      ->capture_default_str();
  int baudrate = 19200;
  raw->add_option("--baudrate", baudrate, "Baudrate")->capture_default_str();
  int timeout = 0;
  raw->add_option("-t,--timeout", timeout, "Transaction Timeout (ms)")
      ->capture_default_str();

  CLI::App* data = app.add_subcommand("data", "Get Data from device");
  std::string regMapPath{};
  data->add_option("-r,--regmap", regMapPath, "Register Map JSON")->required();

  CLI::App* discover =
      app.add_subcommand("discover", "Discover devices without reading data");
  std::string discoverRegMapPath{};
  discover->add_option("-r,--regmap", discoverRegMapPath, "Register Map JSON")
      ->required();

  auto addCommonOpts = [&](CLI::App* sub,
                           int& addr,
                           int& reg,
                           std::string& par,
                           int& baud,
                           int& tout) {
    sub->add_option("addr", addr, "Device address")->required();
    sub->add_option("register", reg, "Register offset")->required();
    sub->add_option("--parity", par, "Parity")
        ->check(CLI::IsMember({"ODD", "EVEN", "NONE"}))
        ->capture_default_str();
    sub->add_option("--baudrate", baud, "Baudrate")->capture_default_str();
    sub->add_option("-t,--timeout", tout, "Transaction Timeout (ms)")
        ->capture_default_str();
  };

  int rAddr = 0, rReg = 0, rBaud = 19200, rTimeout = 0, rCount = 1;
  std::string rParity = "EVEN";
  CLI::App* read = app.add_subcommand("read", "Read holding register(s)");
  addCommonOpts(read, rAddr, rReg, rParity, rBaud, rTimeout);
  read->add_option("-c,--count", rCount, "Number of registers to read")
      ->capture_default_str();

  int wAddr = 0, wReg = 0, wBaud = 19200, wTimeout = 0;
  std::vector<uint16_t> wValues;
  std::string wParity = "EVEN";
  CLI::App* write = app.add_subcommand("write", "Write holding register(s)");
  addCommonOpts(write, wAddr, wReg, wParity, wBaud, wTimeout);
  write->add_option("values", wValues, "Value(s) to write")->required();

  CLI11_PARSE(app, argc, argv);
  ServiceExclusion serviceExclusion(tty);

  json intf;
  intf["device_path"] = tty;
  if (*raw) {
    intf["baudrate"] = baudrate;
    intf["min_delay"] = minDelay;
    rackmon::Parity parity = parityMap.at(parityStr);
    rawCommand(intf, cmd, respLen, parity, timeout);
    return 0;
  }
  if (*data) {
    intf["baudrate"] = 19200;
    intf["min_delay"] = 3;
    dataCommand(intf, regMapPath, true);
    return 0;
  }
  if (*discover) {
    intf["baudrate"] = 19200;
    intf["min_delay"] = 3;
    dataCommand(intf, discoverRegMapPath, false);
    return 0;
  }
  if (*read) {
    intf["baudrate"] = rBaud;
    intf["min_delay"] = minDelay;
    readCommand(intf, rAddr, rReg, rCount, parityMap.at(rParity), rTimeout);
    return 0;
  }
  if (*write) {
    intf["baudrate"] = wBaud;
    intf["min_delay"] = minDelay;
    writeCommand(intf, wAddr, wReg, wValues, parityMap.at(wParity), wTimeout);
    return 0;
  }

  return 0;
}

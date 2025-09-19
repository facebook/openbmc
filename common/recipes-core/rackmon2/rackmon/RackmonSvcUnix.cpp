// Copyright 2021-present Facebook. All Rights Reserved.
#include <nlohmann/json.hpp>
#include <sys/file.h>
#include <unistd.h>
#include "Log.h"
#include "Rackmon.h"
#include "UnixSock.h"

using nlohmann::json;
using namespace rackmon;

namespace rackmonsvc {

class RackmonUNIXSocketService : public UnixService {
  // The configuration file paths.
  const std::string kRackmonConfigurationPath;
  const std::string kRackmonRegmapDirPath;
  Rackmon rackmond_{};

  // Handle commands with the JSON format.
  void executeJSONCommand(const json& req, json& resp);
  void handleJSONCommand(
      std::unique_ptr<json> reqPtr,
      std::unique_ptr<UnixSock> cli);

  void handleRequest(
      const std::vector<char>& buf,
      std::unique_ptr<UnixSock> sock) override;

 public:
  RackmonUNIXSocketService() = delete;

  explicit RackmonUNIXSocketService(
      const std::string& confPath,
      const std::string& regmapDirPath)
      : UnixService("/var/run/rackmond.sock"),
        kRackmonConfigurationPath(confPath),
        kRackmonRegmapDirPath(regmapDirPath) {}
  ~RackmonUNIXSocketService() {}
  // initialize based on service args.
  void initialize() override;
  // Clean up everything before exit.
  void deinitialize() override;
};

struct ModbusDataFilter {
  ModbusDeviceFilter devFilter{};
  ModbusRegisterFilter regFilter{};
  bool latestValueOnly = false;
};

void from_json(const json& filter, ModbusDataFilter& out) {
  if (filter.contains("deviceFilter")) {
    const json& jdevFilter = filter["deviceFilter"];
    if (jdevFilter.contains("addressFilter")) {
      out.devFilter.locationFilter =
          DeviceLocationFilter((std::set<uint16_t>)jdevFilter["addressFilter"]);
    } else if (jdevFilter.contains("typeFilter")) {
      out.devFilter.typeFilter = jdevFilter["typeFilter"];
    } else {
      throw std::logic_error("Device Filter needs at least one set");
    }
  }
  if (filter.contains("registerFilter")) {
    const json& jregFilter = filter["registerFilter"];
    if (jregFilter.contains("addressFilter")) {
      out.regFilter.addrFilter = jregFilter["addressFilter"];
    } else if (jregFilter.contains("nameFilter")) {
      out.regFilter.nameFilter = jregFilter["nameFilter"];
    } else if (jregFilter.contains("unitsOnly") && jregFilter["unitsOnly"]) {
      out.regFilter.unitsOnly = jregFilter["unitsOnly"];
    } else {
      throw std::logic_error("Register Filter needs at least one set");
    }
  }
  out.latestValueOnly = filter.value("latestValueOnly", false);
}

static UniqueDeviceAddress getVerifiedAddress(
    uint16_t devAddress,
    std::optional<uint16_t> uniqueDevAddress) {
  auto [port1, addr1] = DeviceLocationFilter::decompose(devAddress);
  if (uniqueDevAddress.has_value()) {
    auto [port2, addr2] =
        DeviceLocationFilter::decompose(uniqueDevAddress.value());
    if (addr1 != addr2) {
      throw std::runtime_error(
          "Mismatch between device address and unique device address");
    }
    if (port1.has_value() && port1 != port2) {
      throw std::runtime_error(
          "Mismatch between port from device address and port from unique device address");
    }
    return std::make_pair(port2, addr2);
  }
  return std::make_pair(port1, addr1);
}

void RackmonUNIXSocketService::executeJSONCommand(const json& req, json& resp) {
  std::string cmd;
  req.at("type").get_to(cmd);

  std::optional<uint16_t> uniqueDevAddress = std::nullopt;
  if (req.contains("uniqueDevAddress")) {
    uniqueDevAddress = req["uniqueDevAddress"].get<uint16_t>();
  }

  if (cmd == "raw") {
    Request req_m;
    Response resp_m;
    for (auto& b : req["cmd"])
      req_m << uint8_t(b);
    resp_m.len = req["response_length"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    rackmond_.rawCmd(req_m, uniqueDevAddress, resp_m, timeout);
    resp["data"] = {};
    for (size_t i = 0; i < resp_m.len; i++) {
      resp["data"].push_back(int(resp_m.raw[i]));
    }
  } else if (cmd == "listModbusDevices") {
    resp["data"] = rackmond_.listDevices();

  } else if (cmd == "readHoldingRegisters") {
    auto [port, devAddress] =
        getVerifiedAddress(req["devAddress"], uniqueDevAddress);
    uint16_t regAddress = req["regAddress"];
    size_t numRegisters = req["numRegisters"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    std::vector<uint16_t> value(numRegisters);
    rackmond_.readHoldingRegisters(
        devAddress, port, regAddress, value, timeout);
    resp["regValues"] = value;
  } else if (cmd == "writeSingleRegister") {
    auto [port, devAddress] =
        getVerifiedAddress(req["devAddress"], uniqueDevAddress);
    uint16_t regAddress = req["regAddress"];
    uint16_t regValue = req["regValue"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    rackmond_.writeSingleRegister(
        devAddress, port, regAddress, regValue, timeout);
  } else if (cmd == "presetMultipleRegisters") {
    auto [port, devAddress] =
        getVerifiedAddress(req["devAddress"], uniqueDevAddress);
    uint16_t regAddress = req["regAddress"];
    std::vector<uint16_t> values = req["regValue"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    rackmond_.writeMultipleRegisters(
        devAddress, port, regAddress, values, timeout);
  } else if (cmd == "readFileRecord") {
    auto [port, devAddress] =
        getVerifiedAddress(req["devAddress"], uniqueDevAddress);
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    std::vector<FileRecord> records = req["records"];
    rackmond_.readFileRecord(devAddress, port, records, timeout);
    resp["data"] = records;
  } else if (cmd == "getMonitorDataRaw") {
    std::vector<ModbusDeviceRawData> ret;
    rackmond_.getRawData(ret);
    resp["data"] = ret;
  } else if (cmd == "pause") {
    rackmond_.stop();
  } else if (cmd == "resume") {
    rackmond_.start();
  } else if (cmd == "rescan") {
    rackmond_.forceScan();
  } else if (cmd == "getMonitorData") {
    ModbusDataFilter filter{};
    if (req.contains("filter")) {
      filter = req["filter"];
    }
    std::vector<ModbusDeviceValueData> ret;
    rackmond_.getValueData(
        ret, filter.devFilter, filter.regFilter, filter.latestValueOnly);
    resp["data"] = ret;
  } else if (cmd == "reloadRegisters") {
    ModbusDataFilter filter{};
    if (req.contains("filter")) {
      filter = req["filter"];
    }
    bool sync = req.value("synchronous", true);
    if (sync) {
      rackmond_.reload(filter.devFilter, filter.regFilter);
    } else {
      auto tid = std::thread([this, filter]() {
        try {
          rackmond_.reload(filter.devFilter, filter.regFilter);
        } catch (std::exception& e) {
          logError << "Async Reload failed: " << e.what() << std::endl;
        }
      });
      tid.detach();
    }
  } else {
    throw std::logic_error("UNKNOWN_CMD: " + cmd);
  }
  resp["status"] = "SUCCESS";
}

void RackmonUNIXSocketService::handleJSONCommand(
    std::unique_ptr<json> reqPtr,
    std::unique_ptr<UnixSock> cli) {
  const json& req = *reqPtr;
  auto print_msg = [&req](std::exception& e) {
    logError << "ERROR Executing: " << req["type"] << " " << e.what()
             << std::endl;
  };
  json resp;

  // Handle the JSON command and this is where all the
  // exceptions we have been ignoring all the way from
  // Device to Rackmon is going to come to roost. Convert
  // each exception to an error code.
  try {
    executeJSONCommand(req, resp);
  } catch (CRCError& e) {
    resp["status"] = "ERR_BAD_CRC";
    print_msg(e);
  } catch (TimeoutException& e) {
    resp["status"] = "ERR_TIMEOUT";
    print_msg(e);
  } catch (std::underflow_error& e) {
    resp["status"] = "ERR_UNDERFLOW";
    print_msg(e);
  } catch (std::overflow_error& e) {
    resp["status"] = "ERR_OVERFLOW";
    print_msg(e);
  } catch (std::logic_error& e) {
    resp["status"] = "ERR_INVALID_ARGS";
    print_msg(e);
  } catch (ModbusError& e) {
    resp["status"] = e.toString(e.errorCode);
  } catch (std::runtime_error& e) {
    resp["status"] = "ERR_IO_FAILURE";
    print_msg(e);
  } catch (std::exception& e) {
    resp["status"] = "ERR_IO_FAILURE";
    print_msg(e);
  }

  try {
    std::string resp_s =
        resp.dump(-1, ' ', false, json::error_handler_t::replace);
    cli->send(resp_s.c_str(), resp_s.length());
  } catch (std::exception& e) {
    logError << "Unable to send response: " << e.what() << std::endl;
  }
}

void RackmonUNIXSocketService::initialize() {
  logInfo << "Loading configuration" << std::endl;
  rackmond_.load(kRackmonConfigurationPath, kRackmonRegmapDirPath);
  logInfo << "Starting rackmon threads" << std::endl;
  rackmond_.start();
  UnixService::initialize();
}

void RackmonUNIXSocketService::deinitialize() {
  logInfo << "Deinitializing... stopping rackmond" << std::endl;
  rackmond_.stop();
  UnixService::deinitialize();
}

void RackmonUNIXSocketService::handleRequest(
    const std::vector<char>& buf,
    std::unique_ptr<UnixSock> sock) {
  std::unique_ptr<json> req = std::make_unique<json>();
  try {
    *req = json::parse(buf);
    if ((*req)["type"] == "raw") {
      handleJSONCommand(std::move(req), std::move(sock));
    } else {
      auto tid = std::thread(
          &RackmonUNIXSocketService::handleJSONCommand,
          this,
          std::move(req),
          std::move(sock));
      tid.detach();
    }
  } catch (std::exception& e) {
    logError << "Handling request failed: " << e.what() << std::endl;
  }
}

} // namespace rackmonsvc

using namespace rackmonsvc;

int main(int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  int fd = open("/var/run/rackmond.lock", O_CREAT | O_RDWR, 0666);
  if (fd < 0) {
    logError << "Cannot create/open /var/run/rackmond.lock" << std::endl;
    return fd;
  }
  if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
    close(fd);
    logError << "Another instance of rackmond is running" << std::endl;
    return -1;
  }
  std::string confPath;
  std::string regmapDirPath;
  if (argc == 1) {
    confPath = "/etc/rackmon.conf";
    regmapDirPath = "/etc/rackmon.d";
  } else if (argc == 3) {
    confPath = argv[1];
    regmapDirPath = argv[2];
  } else {
    logError << "Unexpected command line arguments" << std::endl;
    return -1;
  }
  rackmonsvc::RackmonUNIXSocketService svc(confPath, regmapDirPath);
  svc.initialize();
  svc.doLoop();
  svc.deinitialize();
  return 0;
}

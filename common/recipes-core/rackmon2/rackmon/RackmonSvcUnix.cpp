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
  const std::string kRackmonConfigurationPath = "/etc/rackmon.conf";
  const std::string kRackmonRegmapDirPath = "/etc/rackmon.d";
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
  RackmonUNIXSocketService() : UnixService("/var/run/rackmond.sock") {}
  ~RackmonUNIXSocketService() {}
  // initialize based on service args.
  void initialize(int argc, char** argv) override;
  // Clean up everything before exit.
  void deinitialize() override;
};

void RackmonUNIXSocketService::executeJSONCommand(const json& req, json& resp) {
  std::string cmd;
  req.at("type").get_to(cmd);
  if (cmd == "raw") {
    Request req_m;
    Response resp_m;
    for (auto& b : req["cmd"])
      req_m << uint8_t(b);
    resp_m.len = req["response_length"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    rackmond_.rawCmd(req_m, resp_m, timeout);
    resp["data"] = {};
    for (size_t i = 0; i < resp_m.len; i++) {
      resp["data"].push_back(int(resp_m.raw[i]));
    }
  } else if (cmd == "listModbusDevices") {
    resp["data"] = rackmond_.listDevices();

  } else if (cmd == "readHoldingRegisters") {
    uint8_t devAddress = req["devAddress"];
    uint16_t regAddress = req["regAddress"];
    size_t numRegisters = req["numRegisters"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    std::vector<uint16_t> value(numRegisters);
    rackmond_.readHoldingRegisters(devAddress, regAddress, value, timeout);
    resp["regValues"] = value;
  } else if (cmd == "writeSingleRegister") {
    uint8_t devAddress = req["devAddress"];
    uint16_t regAddress = req["regAddress"];
    uint16_t regValue = req["regValue"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    rackmond_.writeSingleRegister(devAddress, regAddress, regValue, timeout);
  } else if (cmd == "presetMultipleRegisters") {
    uint8_t devAddress = req["devAddress"];
    uint16_t regAddress = req["regAddress"];
    std::vector<uint16_t> values = req["regValue"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    rackmond_.writeMultipleRegisters(devAddress, regAddress, values, timeout);
  } else if (cmd == "readFileRecord") {
    uint8_t devAddress = req["devAddress"];
    ModbusTime timeout = ModbusTime(req.value("timeout", 0));
    std::vector<FileRecord> records = req["records"];
    rackmond_.readFileRecord(devAddress, records, timeout);
    resp["data"] = records;
  } else if (cmd == "getMonitorDataRaw") {
    std::vector<ModbusDeviceRawData> ret;
    rackmond_.getRawData(ret);
    resp["data"] = ret;
  } else if (cmd == "pause") {
    rackmond_.stop();
  } else if (cmd == "resume") {
    rackmond_.start();
  } else if (cmd == "getMonitorData") {
    ModbusDeviceFilter devFilter{};
    ModbusRegisterFilter regFilter{};
    bool latestValueOnly = false;
    if (req.contains("filter")) {
      const json& filter = req["filter"];
      if (filter.contains("deviceFilter")) {
        const json& jdevFilter = filter["deviceFilter"];
        if (jdevFilter.contains("addressFilter")) {
          devFilter.addrFilter = jdevFilter["addressFilter"];
        } else if (jdevFilter.contains("typeFilter")) {
          devFilter.typeFilter = jdevFilter["typeFilter"];
        } else {
          throw std::logic_error("Device Filter needs at least one set");
        }
      }
      if (filter.contains("registerFilter")) {
        const json& jregFilter = filter["registerFilter"];
        if (jregFilter.contains("addressFilter")) {
          regFilter.addrFilter = jregFilter["addressFilter"];
        } else if (jregFilter.contains("nameFilter")) {
          regFilter.nameFilter = jregFilter["nameFilter"];
        } else {
          throw std::logic_error("Register Filter needs at least one set");
        }
      }
      latestValueOnly = filter.value("latestValueOnly", false);
    }
    std::vector<ModbusDeviceValueData> ret;
    rackmond_.getValueData(ret, devFilter, regFilter, latestValueOnly);
    resp["data"] = ret;
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
    logError << "ERROR Executing: " << req["type"] << e.what() << std::endl;
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

  std::string resp_s = resp.dump();
  try {
    cli->send(resp_s.c_str(), resp_s.length());
  } catch (std::exception& e) {
    logError << "Unable to send response: " << e.what() << std::endl;
  }
}

void RackmonUNIXSocketService::initialize(int argc, char** argv) {
  logInfo << "Loading configuration" << std::endl;
  rackmond_.load(kRackmonConfigurationPath, kRackmonRegmapDirPath);
  logInfo << "Starting rackmon threads" << std::endl;
  rackmond_.start();
  UnixService::initialize(argc, argv);
}

void RackmonUNIXSocketService::deinitialize() {
  logInfo << "Deinitializing... stopping rackmond" << std::endl;
  rackmond_.stop();
  UnixService::deinitialize();
}

void RackmonUNIXSocketService::handleRequest(
    const std::vector<char>& buf,
    std::unique_ptr<UnixSock> sock) {
  bool is_json = true;
  std::unique_ptr<json> req = std::make_unique<json>();
  try {
    *req = json::parse(buf);
  } catch (...) {
    is_json = false;
  }

  if (is_json) {
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
  } else {
    logError << "Service got invalid JSON" << std::endl;
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
  rackmonsvc::RackmonUNIXSocketService svc;
  svc.initialize(argc, argv);
  svc.doLoop();
  svc.deinitialize();
  return 0;
}

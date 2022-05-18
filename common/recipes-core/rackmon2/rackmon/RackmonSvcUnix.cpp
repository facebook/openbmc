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
  void handleJSONCommand(const json& req, json& resp);
  void handleJSONCommand(const json& req, UnixSock& cli);
  // Handle commands with the legacy byte format.
  void handleLegacyCommand(
      const std::vector<char>& req_buf,
      std::vector<char>& resp_buf);
  void handleLegacyCommand(const std::vector<char>& req_buf, UnixSock& cli);

  void handleRequest(const std::vector<char>& buf, UnixSock& sock) override;

 public:
  RackmonUNIXSocketService() : UnixService("/var/run/rackmond.sock") {}
  ~RackmonUNIXSocketService() {}
  // initialize based on service args.
  void initialize(int argc, char** argv) override;
  // Clean up everything before exit.
  void deinitialize() override;
};

void RackmonUNIXSocketService::handleJSONCommand(const json& req, json& resp) {
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
  } else if (cmd == "list") {
    resp["data"] = rackmond_.listDevices();
  } else if (cmd == "raw_data") {
    std::vector<ModbusDeviceRawData> ret;
    rackmond_.getRawData(ret);
    resp["data"] = ret;
  } else if (cmd == "pause") {
    rackmond_.stop();
  } else if (cmd == "resume") {
    rackmond_.start();
  } else if (cmd == "value_data") {
    std::vector<ModbusDeviceValueData> ret;
    rackmond_.getValueData(ret);
    resp["data"] = ret;
  } else {
    throw std::logic_error("UNKNOWN_CMD: " + cmd);
  }
  resp["status"] = "SUCCESS";
}

void RackmonUNIXSocketService::handleJSONCommand(
    const json& req,
    UnixSock& cli) {
  auto print_msg = [&req](std::exception& e) {
    logError << "ERROR Executing: " << req["type"] << e.what() << std::endl;
  };
  json resp;

  // Handle the JSON command and this is where all the
  // exceptions we have been ignoring all the way from
  // Device to Rackmon is going to come to roost. Convert
  // each exception to an error code.
  // TODO: Work with rest-api to correctly define these.
  try {
    handleJSONCommand(req, resp);
  } catch (CRCError& e) {
    resp["status"] = "CRC_ERROR";
    print_msg(e);
  } catch (TimeoutException& e) {
    resp["status"] = "TIMEOUT_ERROR";
    print_msg(e);
  } catch (std::underflow_error& e) {
    resp["status"] = "UNDERFLOW_ERROR";
    print_msg(e);
  } catch (std::overflow_error& e) {
    resp["status"] = "OVERFLOW_ERROR";
    print_msg(e);
  } catch (std::logic_error& e) {
    resp["status"] = "USER_ERROR";
    print_msg(e);
  } catch (std::runtime_error& e) {
    resp["status"] = "RUNTIME_ERROR";
    print_msg(e);
  } catch (std::exception& e) {
    resp["status"] = "UNKNOWN_ERROR";
    print_msg(e);
  }

  std::string resp_s = resp.dump();
  try {
    cli.send(resp_s.c_str(), resp_s.length());
  } catch (std::exception& e) {
    logError << "Unable to send response: " << e.what() << std::endl;
  }
}

void RackmonUNIXSocketService::handleLegacyCommand(
    const std::vector<char>& req_buf,
    std::vector<char>& resp_buf) {
  struct req_hdr {
    uint16_t type;
    uint16_t length;
    uint16_t expected_resp_length;
    uint32_t custom_timeout;
  } const* req_hdr = (const struct req_hdr*)req_buf.data();

  if (req_buf.size() < sizeof(req_hdr))
    throw std::underflow_error("header");
  if (req_hdr->type != 1)
    throw std::logic_error(
        "Unsupported command: " + std::to_string(req_hdr->type));
  if (req_hdr->length < 1)
    throw std::logic_error("request needs at least 1 byte");
  if (req_hdr->length + sizeof(req_hdr) != req_buf.size())
    throw std::overflow_error("body");
  Request req_msg;
  std::copy(
      req_buf.begin() + sizeof(struct req_hdr),
      req_buf.end(),
      req_msg.raw.begin());
  req_msg.len = req_hdr->length;

  Response resp_msg;
  resp_msg.len = req_hdr->expected_resp_length;
  ModbusTime timeout(req_hdr->custom_timeout);
  rackmond_.rawCmd(req_msg, resp_msg, timeout);

  size_t resp_sz = req_hdr->expected_resp_length;
  resp_buf.resize(2 + resp_sz);
  resp_buf[0] = 0xff & resp_sz;
  resp_buf[1] = 0xff & (resp_sz >> 8);
  std::copy(
      resp_msg.raw.begin(),
      resp_msg.raw.begin() + resp_sz,
      resp_buf.begin() + 2);
}

void RackmonUNIXSocketService::handleLegacyCommand(
    const std::vector<char>& req_buf,
    UnixSock& cli) {
  std::vector<char> resp_buf;
  try {
    handleLegacyCommand(req_buf, resp_buf);
  } catch (std::exception& e) {
    resp_buf = {0, 0, 1, 0};
    logError << "Unable to handle legacy command: " << e.what() << std::endl;
  }
  try {
    cli.send(resp_buf.data(), resp_buf.size());
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
    UnixSock& sock) {
  bool is_json = true;
  json req;
  try {
    req = json::parse(buf);
  } catch (...) {
    is_json = false;
  }

  if (is_json)
    handleJSONCommand(req, sock);
  else
    handleLegacyCommand(buf, sock);
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

// Copyright 2021-present Facebook. All Rights Reserved.
#include <nlohmann/json.hpp>
#include <poll.h>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include "Log.hpp"
#include "Rackmon.hpp"
#include "RackmonSvcUnix.hpp"

using nlohmann::json;
using namespace rackmonsvc;
using namespace rackmon;

class RackmonUNIXSocketService {
  // The configuration file paths.
  const std::string kRackmonConfigurationPath = "/etc/rackmon.conf";
  const std::string kRackmonRegmapDirPath = "/etc/rackmon.d";
  Rackmon rackmond_{};
  // The pipe used for the signal handler to request
  // for the loops to exit.
  int backChannelFDs_[2] = {-1, -1};
  // End of the pipe used by the service loop.
  int& backChannelHandler_ = backChannelFDs_[0];
  // End of the pipe to be used by the signal handler.
  int& backChannelRequestor_ = backChannelFDs_[1];
  // The socket we want to receive connections from.
  std::unique_ptr<RackmonService> sock_ = nullptr;

  void registerExitHandler();

  // Handle commands with the JSON format.
  void handleJSONCommand(const json& req, json& resp);
  void handleJSONCommand(const json& req, RackmonSock& cli);
  // Handle commands with the legacy byte format.
  void handleLegacyCommand(
      std::vector<char>& req_buf,
      std::vector<char>& resp_buf);
  void handleLegacyCommand(std::vector<char>& req_buf, RackmonSock& cli);
  // Handle a connection from a client. Currently
  // we can deal with one client at a time, but we
  // could easily extend this to multiple with threading.
  void handleConnection(std::unique_ptr<RackmonSock> sock);

 public:
  RackmonUNIXSocketService() {}
  ~RackmonUNIXSocketService() {
    deinitialize();
  }
  // initialize based on service args.
  void initialize(int argc, char* argv[]);
  // Clean up everything before exit.
  void deinitialize();
  // Request the doLoop to exit.
  void requestExit();
  // The main service loop. This will block
  // till someone requests it to exit.
  void doLoop();
  static void triggerExit(int /* unused */) {
    svc.requestExit();
  }
  static RackmonUNIXSocketService svc;
};

RackmonUNIXSocketService RackmonUNIXSocketService::svc;

void RackmonUNIXSocketService::handleJSONCommand(const json& req, json& resp) {
  std::string cmd;
  req.at("type").get_to(cmd);
  if (cmd == "raw") {
    Msg req_m, resp_m;
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
  } else if (cmd == "print_data") {
    std::vector<ModbusDeviceFmtData> ret;
    rackmond_.getFmtData(ret);
    resp["data"] = ret;
  } else if (cmd == "value_data") {
    std::vector<ModbusDeviceValueData> ret;
    rackmond_.getValueData(ret);
    resp["data"] = ret;
  } else if (cmd == "profile") {
    resp["data"] = rackmond_.getProfileData();
  } else {
    throw std::logic_error("UNKNOWN_CMD: " + cmd);
  }
  resp["status"] = "SUCCESS";
}

void RackmonUNIXSocketService::handleJSONCommand(
    const json& req,
    RackmonSock& cli) {
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
    std::vector<char>& req_buf,
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
  Msg req_msg;
  std::copy(
      req_buf.begin() + sizeof(struct req_hdr),
      req_buf.end(),
      req_msg.raw.begin());
  req_msg.len = req_hdr->length;

  Msg resp_msg;
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
    std::vector<char>& req_buf,
    RackmonSock& cli) {
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

void RackmonUNIXSocketService::requestExit() {
  char c = 'c';
  if (write(backChannelRequestor_, &c, 1) != 1) {
    logError << "Could not request rackmon svc to exit!" << std::endl;
  }
}

void RackmonUNIXSocketService::registerExitHandler() {
  struct sigaction ign_action, exit_action;

  if (pipe(backChannelFDs_) != 0) {
    std::system_error(
        std::error_code(errno, std::generic_category()),
        "Backchannel Pipe creation");
  }

  ign_action.sa_flags = 0;
  sigemptyset(&ign_action.sa_mask);
  ign_action.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &ign_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGPIPE handler");
  }

  exit_action.sa_flags = 0;
  sigemptyset(&exit_action.sa_mask);
  exit_action.sa_handler = RackmonUNIXSocketService::triggerExit;
  if (sigaction(SIGTERM, &exit_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGTERM handler");
  }
  if (sigaction(SIGINT, &exit_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGINT handler");
  }
}

void RackmonUNIXSocketService::initialize(
    int /*unused */,
    char** /* unused */) {
  logInfo << "Loading configuration" << std::endl;
  rackmond_.load(kRackmonConfigurationPath, kRackmonRegmapDirPath);
  logInfo << "Starting rackmon threads" << std::endl;
  rackmond_.start();
  registerExitHandler();
  logInfo << "Creating Rackmon UNIX service" << std::endl;
  sock_ = std::make_unique<RackmonService>();
}

void RackmonUNIXSocketService::deinitialize() {
  logInfo << "Deinitializing... stopping rackmond" << std::endl;
  rackmond_.stop();
  sock_ = nullptr;
  if (backChannelRequestor_ != -1) {
    close(backChannelRequestor_);
    backChannelRequestor_ = -1;
  }
  if (backChannelHandler_ != -1) {
    close(backChannelHandler_);
    backChannelHandler_ = -1;
  }
}

void RackmonUNIXSocketService::handleConnection(
    std::unique_ptr<RackmonSock> sock) {
  std::vector<char> buf;

  try {
    sock->recv(buf);
  } catch (...) {
    logError << "Failed to receive message" << std::endl;
    return;
  }
  bool is_json = true;
  json req;
  try {
    req = json::parse(buf);
  } catch (...) {
    is_json = false;
  }

  if (is_json)
    handleJSONCommand(req, *sock);
  else
    handleLegacyCommand(buf, *sock);
}

void RackmonUNIXSocketService::doLoop() {
  struct sockaddr_un client;
  struct pollfd pfd[2] = {
      {sock_->getSock(), POLLIN, 0},
      {backChannelHandler_, POLLIN, 0},
  };

  while (1) {
    int ret;
    socklen_t clisocklen = sizeof(struct sockaddr_un);

    ret = poll(pfd, 2, -1);
    if (ret <= 0) {
      // This should be the common case. The entire thing
      // with the pipe is to handle the race condition when
      // we get a signal when we are actively handling a
      // previous request.
      logInfo << "Handling termination signal" << std::endl;
      break;
    }
    if (pfd[1].revents & POLLIN) {
      char c;
      if (read(pfd[1].fd, &c, 1) != 1) {
        logError << "Got something but no data!" << std::endl;
      } else if (c == 'c') {
        logInfo << "Handling termination request" << std::endl;
        break;
      } else {
        logError << "Got unknown command: " << c << std::endl;
      }
    }
    if (pfd[0].revents & POLLIN) {
      int clifd =
          accept(sock_->getSock(), (struct sockaddr*)&client, &clisocklen);
      if (clifd < 0) {
        logError << "Failed to accept new connection" << std::endl;
        continue;
      }
      auto clisock = std::make_unique<RackmonSock>(clifd);
      auto tid = std::thread(
          &RackmonUNIXSocketService::handleConnection,
          this,
          std::move(clisock));
      tid.detach();
    }
  }
}

int main(int argc, char* argv[]) {
  RackmonUNIXSocketService::svc.initialize(argc, argv);
  RackmonUNIXSocketService::svc.doLoop();
  RackmonUNIXSocketService::svc.deinitialize();
  return 0;
}

#include "rackmon_svc_unix.hpp"
#include <nlohmann/json.hpp>
#include <poll.h>
#include <unistd.h>
#include <csignal>
#include <iostream>
#include "rackmon.hpp"

using nlohmann::json;

class RackmonUNIXSocketService {
  // The configuration file paths.
  const std::string rackmon_configuration_path = "/etc/rackmon.conf";
  const std::string rackmon_regmap_dir_path = "/etc/rackmon.d";
  Rackmon rackmond{};
  // The pipe used for the signal handler to request
  // for the loops to exit.
  int backchannel_fds[2] = {-1, -1};
  // End of the pipe used by the service loop.
  int& backchannel_handler = backchannel_fds[0];
  // End of the pipe to be used by the signal handler.
  int& backchannel_req = backchannel_fds[1];
  // The socket we want to receive connections from.
  std::unique_ptr<RackmonService> sock = nullptr;

  void register_exit_handler();

  // Handle commands with the JSON format.
  void handle_json_command(const json& req, json& resp);
  void handle_json_command(const json& req, RackmonSock& cli);
  // Handle commands with the legacy byte format.
  void handle_legacy_command(
      std::vector<char>& req_buf,
      std::vector<char>& resp_buf);
  void handle_legacy_command(std::vector<char>& req_buf, RackmonSock& cli);
  // Handle a connection from a client. Currently
  // we can deal with one client at a time, but we
  // could easily extend this to multiple with threading.
  void handle_connection(RackmonSock& sock);

 public:
  RackmonUNIXSocketService() {}
  ~RackmonUNIXSocketService() {
    deinitialize();
  }
  // initialize based on service args.
  void initialize(int argc, char* argv[]);
  // Clean up everything before exit.
  void deinitialize();
  // Request the do_loop to exit.
  void request_exit();
  // The main service loop. This will block
  // till someone requests it to exit.
  void do_loop();
  static void trigger_exit(int signum) {
    svc.request_exit();
  }
  static RackmonUNIXSocketService svc;
};

RackmonUNIXSocketService RackmonUNIXSocketService::svc;

void RackmonUNIXSocketService::handle_json_command(
    const json& req,
    json& resp) {
  std::string cmd;
  req.at("type").get_to(cmd);
  if (cmd == "raw") {
    Msg req_m, resp_m;
    for (auto &b : req["cmd"])
      req_m << uint8_t(b);
    resp_m.len = req["response_length"];
    modbus_time timeout = modbus_time(req.value("timeout", 0));
    rackmond.rawCmd(req_m, resp_m, timeout);
    resp["data"] = {};
    for (size_t i = 0; i < resp_m.len; i++) {
      resp["data"].push_back(int(resp_m.raw[i]));
    }
  } else if (cmd == "status") {
    RackmonStatus ret;
    rackmond.get_monitor_status(ret);
    resp["monitor_status"] = ret;
  } else if (cmd == "data") {
    std::vector<ModbusDeviceMonitorData> ret;
    rackmond.get_monitor_data(ret);
    resp["monitor_data"] = ret;
  } else if (cmd == "pause") {
    rackmond.stop();
  } else if (cmd == "resume") {
    rackmond.start();
  } else if (cmd == "formatted_data") {
    std::vector<ModbusDeviceFormattedData> ret;
    rackmond.get_monitor_data_formatted(ret);
    resp["monitor_data"] = ret;
  } else {
    throw std::logic_error("UNKNOWN_CMD: " + cmd);
  }
  resp["status"] = "SUCCESS";
}

void RackmonUNIXSocketService::handle_json_command(
    const json& req,
    RackmonSock& cli) {
  auto print_msg = [&req](std::exception& e) {
    std::cerr << "ERROR Executing: " << req["type"] << e.what() << std::endl;
  };
  json resp;

  // Handle the JSON command and this is where all the
  // exceptions we have been ignoring all the way from
  // Device to Rackmon is going to come to roost. Convert
  // each exception to an error code.
  // TODO: Work with rest-api to correctly define these.
  try {
    handle_json_command(req, resp);
  } catch (crc_exception& e) {
    resp["status"] = "CRC_ERROR";
    print_msg(e);
  } catch (timeout_exception& e) {
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
    std::cerr << "Unable to send response: " << e.what() << std::endl;
  }
}

void RackmonUNIXSocketService::handle_legacy_command(
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
  modbus_time timeout(req_hdr->custom_timeout);
  rackmond.rawCmd(req_msg, resp_msg, timeout);

  size_t resp_sz = req_hdr->expected_resp_length;
  resp_buf.resize(2 + resp_sz);
  resp_buf[0] = 0xff & resp_sz;
  resp_buf[1] = 0xff & (resp_sz >> 8);
  std::copy(
      resp_msg.raw.begin(),
      resp_msg.raw.begin() + resp_sz,
      resp_buf.begin() + 2);
}

void RackmonUNIXSocketService::handle_legacy_command(
    std::vector<char>& req_buf,
    RackmonSock& cli) {
  std::vector<char> resp_buf;
  try {
    handle_legacy_command(req_buf, resp_buf);
  } catch (std::exception& e) {
    resp_buf = {0, 0, 1, 0};
    std::cerr << "Unable to handle legacy command: " << e.what() << std::endl;
  }
  try {
    cli.send(resp_buf.data(), resp_buf.size());
  } catch (std::exception& e) {
    std::cerr << "Unable to send response: " << e.what() << std::endl;
  }
}

void RackmonUNIXSocketService::request_exit() {
  char c = 'c';
  if (write(backchannel_req, &c, 1) != 1) {
    std::cerr << "Could not request rackmon svc to exit!" << std::endl;
  }
}

void RackmonUNIXSocketService::register_exit_handler() {
  struct sigaction ign_action, exit_action;

  if (pipe(backchannel_fds) != 0) {
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
  exit_action.sa_handler = RackmonUNIXSocketService::trigger_exit;
  if (sigaction(SIGTERM, &exit_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGTERM handler");
  }
  if (sigaction(SIGINT, &exit_action, NULL) != 0) {
    throw std::system_error(
        std::error_code(errno, std::generic_category()), "SIGINT handler");
  }
}

void RackmonUNIXSocketService::initialize(int argc, char* argv[]) {
  std::cout << "Loading configuration" << std::endl;
  rackmond.load(rackmon_configuration_path, rackmon_regmap_dir_path);
  std::cout << "Starting rackmon threads" << std::endl;
  rackmond.start();
  // rackmond.start();
  register_exit_handler();
  std::cout << "Creating Rackmon UNIX service" << std::endl;
  sock = std::make_unique<RackmonService>();
}

void RackmonUNIXSocketService::deinitialize() {
  std::cout << "Deinitializing... stopping rackmond" << std::endl;
  rackmond.stop();
  sock = nullptr;
  if (backchannel_req != -1) {
    close(backchannel_req);
    backchannel_req = -1;
  }
  if (backchannel_handler != -1) {
    close(backchannel_handler);
    backchannel_handler = -1;
  }
}

void RackmonUNIXSocketService::handle_connection(RackmonSock& sock) {
  std::vector<char> buf;

  try {
    sock.recv(buf);
  } catch (...) {
    std::cerr << "Failed to receive message" << std::endl;
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
    handle_json_command(req, sock);
  else
    handle_legacy_command(buf, sock);
}

void RackmonUNIXSocketService::do_loop() {
  struct sockaddr_un client;
  struct pollfd pfd[2] = {
      {
          .fd = sock->get_sock(),
          .events = POLLIN,
      },
      {
          .fd = backchannel_handler,
          .events = POLLIN,
      },
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
      std::cout << "Handling termination signal" << std::endl;
      break;
    }
    if (pfd[1].revents & POLLIN) {
      char c;
      if (read(pfd[1].fd, &c, 1) != 1) {
        std::cerr << "Got something but no data!" << std::endl;
      } else if (c == 'c') {
        std::cout << "Handling termination request" << std::endl;
        break;
      } else {
        std::cerr << "Got unknown command: " << c << std::endl;
      }
    }
    if (pfd[0].revents & POLLIN) {
      int clifd =
          accept(sock->get_sock(), (struct sockaddr*)&client, &clisocklen);
      if (clifd < 0) {
        std::cerr << "Failed to accept new connection" << std::endl;
        continue;
      }
      RackmonSock clisock(clifd);
      handle_connection(clisock);
    }
  }
}

int main(int argc, char* argv[]) {
  RackmonUNIXSocketService::svc.initialize(argc, argv);
  RackmonUNIXSocketService::svc.do_loop();
  RackmonUNIXSocketService::svc.deinitialize();
  return 0;
}

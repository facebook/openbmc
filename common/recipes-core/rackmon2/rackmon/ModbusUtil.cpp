#include <CLI/CLI.hpp>
#include <sys/file.h>
#include <unistd.h>
#include <iostream>
#include "Log.h"
#include "Modbus.h"
#include "Msg.h"

using nlohmann::json;

struct RackmondLock {
  int fd = -1;
  RackmondLock() {
    fd = open("/var/run/rackmond.lock", O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
      logError << "Cannot create/open /var/run/rackmond.lock" << std::endl;
      throw std::runtime_error("Cannot create!");
    }
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
      close(fd);
      fd = -1;
      throw std::runtime_error("You need to stop rackmond to use this utility");
    }
  }
  ~RackmondLock() {
    if (fd < 0)
      return;
    flock(fd, LOCK_UN);
    close(fd);
  }
};

int main(int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  RackmondLock lock;
  std::map<std::string, rackmon::Parity> parityMap = {
      {"ODD", rackmon::Parity::ODD},
      {"EVEN", rackmon::Parity::EVEN},
      {"NONE", rackmon::Parity::NONE},
  };

  CLI::App app("Modbus Raw Interface");

  std::string tty{};
  app.add_option("--tty", tty, "TTY Interface to use")->required();
  std::string parityStr = "EVEN";
  app.add_option("--parity", parityStr, "Parity")
      ->check(CLI::IsMember({"ODD", "EVEN", "NONE"}))
      ->capture_default_str();
  int baudrate = 19200;
  app.add_option("--baudrate", baudrate, "Baudrate")->capture_default_str();
  int minDelay = 2;
  size_t respLen = 0;
  app.add_option(
         "-x,--expected-bytes",
         respLen,
         "Expected response length (including 2b CRC)")
      ->required();
  app.add_option(
         "--min-delay", minDelay, "Minimum delay (ms) between transactions")
      ->capture_default_str();
  std::string cmd{};
  app.add_option("command", cmd, "Command to send (hex)")->required();

  CLI11_PARSE(app, argc, argv);

  json intf;
  intf["device_path"] = tty;
  intf["baudrate"] = baudrate;
  intf["min_delay"] = minDelay;
  rackmon::Parity parity = parityMap.at(parityStr);
  rackmon::Modbus dev;
  dev.initialize(intf);
  if (!dev.isPresent()) {
    std::cerr << "Device creation failed!" << std::endl;
    return -1;
  }
  std::stringstream ss;
  rackmon::Msg req;
  rackmon::Msg resp;
  resp.len = respLen;
  for (const char* c = cmd.c_str(); c[0] != '\0' && c[1] != '\0'; c += 2) {
    char v[3] = {c[0], c[1], '\0'};
    req << uint8_t(std::stoi(v, 0, 16));
  }
  try {
    dev.command(req, resp, 0, rackmon::ModbusTime::zero(), parity);
  } catch (std::exception& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    return -1;
  }
  // command would strip out CRC, include it to our output so
  // we have everything to debug in case we are debugging CRC
  // issues.
  resp.len += 2;
  std::cout << resp << std::endl;
  return 0;
}

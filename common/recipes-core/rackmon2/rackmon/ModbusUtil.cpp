// Copyright 2021-present Facebook. All Rights Reserved.
#include <CLI/CLI.hpp>
#include <sys/file.h>
#include <unistd.h>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include "InterfaceScanner.h"
#include "Log.h"
#include "Modbus.h"
#include "ModbusDevice.h"
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

void dataCommand(const json& intf, const std::string& regMapPath) {
  auto dev = std::make_shared<rackmon::Modbus>();
  devInitialize(*dev, intf);
  rackmon::RegisterMapDatabase db = loadDatabase(regMapPath);

  rackmon::ModbusDeviceInventory devices;
  rackmon::InterfaceScanner scanner(
      dev, devices, db, rackmon::PollThreadTime(0), true);
  scanner.fullScan();
  scanner.updateDeviceRegisters();
  std::vector<rackmon::ModbusDeviceValueData> data;
  for (const auto& device : devices.getAllModbusDevices()) {
    auto valueData = device->getValueData();
    data.push_back(valueData);
  }
  json jdata = data;
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
  RackmondLock lock;

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
    dataCommand(intf, regMapPath);
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

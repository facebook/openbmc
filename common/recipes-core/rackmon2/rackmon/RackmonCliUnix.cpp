// Copyright 2021-present Facebook. All Rights Reserved.
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include "UnixSock.h"
#if (defined(__llvm__) && (__clang_major__ < 9)) || \
    (!defined(__llvm__) && (__GNUC__ < 8))
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

using nlohmann::json;
using namespace std::literals::string_literals;
using namespace rackmonsvc;

class RackmonClient : public UnixClient {
 public:
  RackmonClient() : UnixClient("/var/run/rackmond.sock") {}
};

static void print_json(json& j) {
  std::string status;
  json data = j["data"];
  j.at("status").get_to(status);
  if (status == "SUCCESS") {
    std::cout << data.dump() << std::endl;
  } else {
    std::cout << j.dump(4) << std::endl;
    exit(1);
  }
}

static std::string to_string(const json& v) {
  if (v.is_number())
    return std::to_string(int(v));
  else if (v.is_boolean())
    return std::to_string(bool(v));
  else if (v.is_string())
    return v;
  return v.dump();
}

static void print_value_data(const json& j) {
  for (const auto& device : j) {
    std::cout << "Device Address: 0x" << std::hex << std::setw(2)
              << std::setfill('0') << int(device["devInfo"]["devAddress"])
              << '\n';
    std::cout << "Device Type: " << std::string(device["devInfo"]["deviceType"])
              << '\n';
    std::cout << "CRC Errors: " << std::dec << device["devInfo"]["crcErrors"]
              << '\n';
    std::cout << "timeouts: " << std::dec << device["devInfo"]["timeouts"]
              << '\n';
    std::cout << "Misc Errors: " << std::dec << device["devInfo"]["miscErrors"]
              << '\n';
    std::cout << "Baudrate: " << std::dec << device["devInfo"]["baudrate"]
              << '\n';
    std::cout << "Mode: " << std::string(device["devInfo"]["mode"]) << '\n';
    for (const auto& reg : device["regList"]) {
      std::cout << "  " << std::string(reg["name"]) << "<0x" << std::hex
                << std::setw(4) << std::setfill('0') << int(reg["regAddress"])
                << "> :";
      for (const auto& value : reg["history"]) {
        if (value["type"] == "FLAGS") {
          for (const auto& flag : value["value"]["flagsValue"]) {
            std::cout << "\n    [" << int(flag["value"]) << "] "
                      << std::string(flag["name"]) << " <" << std::dec
                      << int(flag["bitOffset"]) << ">";
          }
          std::cout << '\n';
        } else {
          for (auto x : value["value"].items()) {
            std::cout << ' ' << x.value();
            break;
          }
        }
      }
      std::cout << '\n';
    }
  }
}

static void print_table(const json& j) {
  const int col_width = 12;
  const std::string col_sep = " | ";
  const char hdr_under = '-';
  std::vector<std::string> keys;
  for (const auto& row : j) {
    for (const auto& col : row.items()) {
      if (std::find(keys.begin(), keys.end(), col.key()) == keys.end())
        keys.push_back(col.key());
    }
  }
  for (const auto& hdr : keys) {
    std::cout << std::left << std::setfill(' ') << std::setw(col_width) << hdr
              << col_sep;
  }
  std::cout << '\n';
  std::cout << std::setfill(hdr_under)
            << std::setw((col_width + col_sep.length()) * keys.size() - 1) << ""
            << std::endl;
  for (const auto& row : j) {
    for (const auto& key : keys) {
      std::string val = row.contains(key) ? to_string(row[key]) : "null";
      std::cout << std::setfill(' ') << std::setw(col_width) << val << col_sep;
    }
    std::cout << "\n";
  }
  std::cout << std::endl;
}

static void print_nested(json& j, int indent = 0) {
  if (j.is_array()) {
    for (auto& v : j) {
      print_nested(v, indent + 2);
    }
    std::cout << '\n';
  } else if (j.is_object()) {
    for (auto& item : j.items()) {
      std::cout << std::string(indent, ' ') << item.key() << " : ";
      if (item.value().is_primitive()) {
        print_nested(
            item.value(), 0); // We have already indented. Dont do it again.
      } else {
        std::cout << '\n';
        print_nested(item.value(), indent + 2);
      }
    }
    std::cout << '\n';
  } else {
    if (j.is_string()) {
      std::string s = j;
      size_t end = 0, start = 0;
      while ((end = s.find("\n", start)) != s.npos) {
        std::cout << std::string(indent, ' ') << s.substr(start, end - start)
                  << '\n';
        start = end + 1;
      }
      std::cout << std::string(indent, ' ')
                << s.substr(start, s.length() - start) << '\n';
    } else {
      std::cout << std::string(indent, ' ') << j << '\n';
    }
  }
}

static void print_hexstring(const json& j) {
  std::cout << "Response: ";
  for (const uint8_t& byte : j.get<std::vector<uint8_t>>()) {
    std::cout << std::hex << std::setw(2) << std::setfill('0') << int(byte)
              << " ";
  }
  std::cout << std::endl;
}

static void print_text(const std::string& req_s, json& j) {
  std::string status;
  j.at("status").get_to(status);
  if (status == "SUCCESS") {
    if (req_s == "getMonitorData")
      print_value_data(j["data"]);
    else if (req_s == "getMonitorDataRaw")
      print_nested(j["data"]);
    else if (req_s == "listModbusDevices")
      print_table(j["data"]);
    else if (req_s == "raw")
      print_hexstring(j["data"]);
  } else {
    std::cerr << "FAILURE: " << status << std::endl;
    exit(1);
  }
}

static void
do_raw_cmd(const std::string& req_s, int timeout, int resp_len, bool json_fmt) {
  json req;
  req["type"] = "raw";
  std::vector<uint8_t> bytes;
  for (size_t i = 0; i < req_s.size(); i += 2) {
    std::string bs = req_s.substr(i, 2);
    bytes.push_back(strtol(bs.c_str(), NULL, 16));
  }
  req["cmd"] = bytes;
  req["response_length"] = resp_len;
  if (timeout != 0)
    req["timeout"] = timeout;
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  if (json_fmt)
    print_json(resp_j);
  else
    print_text("raw", resp_j);
}

static void do_read_cmd(
    int devAddr,
    int regAddr,
    int regCount,
    int timeout,
    bool json_fmt) {
  json req;
  req["type"] = "readHoldingRegisters";
  req["devAddress"] = devAddr;
  req["regAddress"] = regAddr;
  req["numRegisters"] = regCount;
  if (timeout != 0) {
    req["timeout"] = timeout;
  }
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  if (json_fmt) {
    print_json(resp_j);
    return;
  }
  std::string status;
  resp_j.at("status").get_to(status);
  if (status == "SUCCESS") {
    std::vector<unsigned int> values;
    resp_j.at("regValues").get_to(values);
    for (auto value : values) {
      std::cout << "0x" << std::hex << std::setw(4) << std::setfill('0')
                << value << ' ';
    }
    std::cout << std::endl;
  } else {
    std::cout << status << std::endl;
  }
}

static void do_read_file_cmd(
    int devAddr,
    int fileNum,
    int recordNum,
    int dataSize,
    int timeout,
    bool json_fmt) {
  json req;
  req["type"] = "readFileRecord";
  req["devAddress"] = devAddr;
  if (timeout != 0) {
    req["timeout"] = timeout;
  }
  req["records"] = json::array();
  req["records"][0]["fileNum"] = fileNum;
  req["records"][0]["recordNum"] = recordNum;
  req["records"][0]["dataSize"] = dataSize;
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  if (json_fmt) {
    print_json(resp_j);
    return;
  }
  std::string status;
  resp_j.at("status").get_to(status);
  if (status == "SUCCESS") {
    for (auto& rec : resp_j["data"]) {
      std::cout << "FILE:0x" << std::hex << rec["fileNum"];
      std::cout << " RECORD:0x" << std::hex << rec["recordNum"] << std::endl;
      for (auto& data : rec["data"]) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << int(data)
                  << ' ';
      }
      std::cout << std::endl;
    }
  } else {
    std::cout << status << std::endl;
  }
}

static void do_write_cmd(
    int devAddr,
    int regAddr,
    std::vector<int>& values,
    int timeout,
    bool json_fmt) {
  json req;
  if (values.size() == 1) {
    req["type"] = "writeSingleRegister";
    req["regValue"] = values[0];
  } else {
    req["type"] = "presetMultipleRegisters";
    req["regValue"] = values;
  }
  req["devAddress"] = devAddr;
  req["regAddress"] = regAddr;
  if (timeout != 0) {
    req["timeout"] = timeout;
  }
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  if (json_fmt) {
    print_json(resp_j);
    return;
  }
  std::string status;
  resp_j.at("status").get_to(status);
  std::cout << status << std::endl;
}

static void do_cmd(const std::string& type, bool json_fmt) {
  json req;
  req["type"] = type;
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  if (json_fmt)
    print_json(resp_j);
  else
    print_text(type, resp_j);
}

static void do_data_cmd(
    const std::string& type,
    bool json_fmt,
    std::vector<int>& deviceFilter,
    std::vector<std::string>& deviceTypeFilter,
    std::vector<int>& regFilter,
    std::vector<std::string>& regNameFilter,
    bool latestOnly) {
  json req;
  req["type"] = type;
  if (deviceFilter.size()) {
    req["filter"]["deviceFilter"]["addressFilter"] = deviceFilter;
  } else if (deviceTypeFilter.size()) {
    req["filter"]["deviceFilter"]["typeFilter"] = deviceTypeFilter;
  }
  if (regFilter.size()) {
    req["filter"]["registerFilter"]["addressFilter"] = regFilter;
  } else if (regNameFilter.size()) {
    req["filter"]["registerFilter"]["nameFilter"] = regNameFilter;
  }
  req["filter"]["latestValueOnly"] = latestOnly;
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  if (json_fmt)
    print_json(resp_j);
  else
    print_text(type, resp_j);
}

static void do_rackmonstatus() {
  json req;
  req["type"] = "listModbusDevices";
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  for (const auto& ent : resp_j["data"]) {
    std::cout << "PSU addr " << std::hex << std::setw(2) << std::setfill('0')
              << int(ent["devAddress"]);
    std::cout << " - crc errors: " << std::dec << ent["crcErrors"];
    std::cout << ", timeouts: " << std::dec << ent["timeouts"];
    std::cout << ", baud rate: " << std::dec << ent["baudrate"] << std::endl;
  }
}

int main(int argc, const char** argv) {
  CLI::App app("Rackmon CLI interface");
  app.failure_message(CLI::FailureMessage::help);
  if (std::filesystem::path(argv[0]).filename() == "modbuscmd") {
    const char** _argv = argv;
    argv = new const char*[argc + 1];
    argv[0] = _argv[0];
    argv[1] = "raw";
    for (int i = 1; i < argc; i++) {
      argv[i + 1] = _argv[i];
    }
    argc++;
  }

  bool json_fmt = false;
  // Allow flags/options to fallthrough from subcommands.
  app.fallthrough();
  app.add_flag("-j,--json", json_fmt, "JSON output instead of text");

  // Raw command
  int raw_cmd_timeout = 0;
  int expected_len = 0;
  std::string req = "";
  auto raw_cmd = app.add_subcommand("raw", "Execute a RAW request");
  raw_cmd->add_option("-t,--timeout", raw_cmd_timeout, "Timeout (ms)");
  raw_cmd
      ->add_option(
          "-x,--expected-bytes",
          expected_len,
          "Expected response length (minus CRC)")
      ->required();
  raw_cmd->add_option("cmd", req, "Request command bytes, ex: a40300000008")
      ->required();
  raw_cmd->callback(
      [&]() { do_raw_cmd(req, raw_cmd_timeout, expected_len, json_fmt); });

  int devAddress = 0;
  int regAddress = 0;
  int regCount = 1;
  auto read_cmd =
      app.add_subcommand("read", "Read Register(s) of a given device");
  read_cmd->add_option("-t,--timeout", raw_cmd_timeout, "Timeout (ms)");
  read_cmd
      ->add_option(
          "DeviceAddress", devAddress, "The device from which we want to read")
      ->required();
  read_cmd
      ->add_option(
          "RegisterAddress",
          regAddress,
          "The Register from which we want to read")
      ->required();
  read_cmd->add_option("--count", regCount, "The number of registers to read")
      ->capture_default_str();
  read_cmd->callback([&]() {
    do_read_cmd(devAddress, regAddress, regCount, raw_cmd_timeout, json_fmt);
  });

  std::vector<int> values{};
  auto write_cmd = app.add_subcommand(
      "write", "Write Register(s) of a given device with values");
  write_cmd->add_option("-t,--timeout", raw_cmd_timeout, "Timeout (ms)");
  write_cmd
      ->add_option(
          "DeviceAddress", devAddress, "The device to which we want to write")
      ->required();
  write_cmd
      ->add_option(
          "RegisterAddress",
          regAddress,
          "The Register to which we want to write")
      ->required();
  write_cmd
      ->add_option(
          "Value",
          values,
          "The values we want to write (Each value is 16bit register)")
      ->required();
  write_cmd->callback([&]() {
    do_write_cmd(devAddress, regAddress, values, raw_cmd_timeout, json_fmt);
  });

  int fileNum, recNum, dataSize;
  auto read_file =
      app.add_subcommand("read_file", "Read file record from the device");
  read_file
      ->add_option(
          "DeviceAddress",
          devAddress,
          "The device from which we want to read the record")
      ->required();
  read_file->add_option("FileNum", fileNum, "The File we want to read")
      ->required();
  read_file
      ->add_option(
          "RecordNum", recNum, "The Record in the file we want to read")
      ->required();
  read_file
      ->add_option("DataSize", dataSize, "The size in words we want to read")
      ->required();
  read_file->add_option("-t,--timeout", raw_cmd_timeout, "Timeout (ms)");
  read_file->callback([&]() {
    do_read_file_cmd(
        devAddress, fileNum, recNum, dataSize, raw_cmd_timeout, json_fmt);
  });

  // List command
  app.add_subcommand("list", "Return list of Modbus devices")->callback([&]() {
    do_cmd("listModbusDevices", json_fmt);
  });

  // Status command
  app.add_subcommand(
         "legacy_list",
         "Return list of Modbus devices (Legacy backwards compatibility)")
      ->callback(do_rackmonstatus);

  // Data command (Get monitored data)
  std::string format = "value";
  std::vector<int> regFilter{};
  std::vector<int> deviceFilter{};
  std::vector<std::string> deviceTypeFilter{};
  std::vector<std::string> regNameFilter{};
  bool latestOnly = false;
  auto data = app.add_subcommand("data", "Return detailed monitoring data");
  data->callback([&]() {
    do_data_cmd(
        format == "raw" ? "getMonitorDataRaw" : "getMonitorData",
        json_fmt,
        deviceFilter,
        deviceTypeFilter,
        regFilter,
        regNameFilter,
        latestOnly);
  });
  data->add_option("-f,--format", format, "Format the data")
      ->check(CLI::IsMember({"raw", "value"}));
  data->add_option(
      "--reg-addr", regFilter, "Return values of provided registers only");
  data->add_option(
      "--dev-addr",
      deviceFilter,
      "Return values of provided device addresses only");
  data->add_option(
      "--dev-type",
      deviceTypeFilter,
      "Return values of provided devices of the given type only");
  data->add_option(
      "--reg-name",
      regNameFilter,
      "Return values of provided register names only");
  data->add_flag(
      "--latest",
      latestOnly,
      "Returns only the latest stored value for a given register");

  // Pause command
  app.add_subcommand("pause", "Pause monitoring")->callback([&]() {
    do_cmd("pause", json_fmt);
  });

  // Resume command
  app.add_subcommand("resume", "Resume monitoring")->callback([&]() {
    do_cmd("resume", json_fmt);
  });

  // Rescan
  app.add_subcommand("rescan", "Force rescan all busses")->callback([&]() {
    do_cmd("rescan", json_fmt);
  });

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  return 0;
}

// Copyright 2021-present Facebook. All Rights Reserved.
#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include "UnixSock.h"

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
              << std::setfill('0') << int(device["deviceAddress"]) << '\n';
    std::cout << "Device Type: " << std::string(device["deviceType"]) << '\n';
    std::cout << "CRC Errors: " << std::dec << device["crcErrors"] << '\n';
    std::cout << "timeouts: " << std::dec << device["timeouts"] << '\n';
    std::cout << "Misc Errors: " << std::dec << device["miscErrors"] << '\n';
    std::cout << "Baudrate: " << std::dec << device["baudrate"] << '\n';
    std::cout << "Mode: " << std::string(device["mode"]) << '\n';
    for (const auto& reg : device["registers"]) {
      std::cout << "  " << std::string(reg["name"]) << "<0x" << std::hex
                << std::setw(4) << std::setfill('0') << int(reg["regAddress"])
                << "> :";
      for (const auto& value : reg["readings"]) {
        if (value["type"] == "flags") {
          for (const auto& flag : value["value"]) {
            std::cout << "\n    [" << int(flag[0]) << "] "
                      << std::string(flag[1]) << " <" << int(flag[2]) << ">";
          }
          std::cout << '\n';
        } else {
          std::cout << ' ' << value["value"];
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
  for (const uint8_t& byte : j) {
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << int(byte) << " ";
  }
  std::cout << std::endl;
}

static void print_text(const std::string& req_s, json& j) {
  std::string status;
  j.at("status").get_to(status);
  if (status == "SUCCESS") {
    if (req_s == "value_data")
      print_value_data(j["data"]);
    else if (req_s == "raw_data" || req_s == "profile")
      print_nested(j["data"]);
    else if (req_s == "list")
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

static void do_rackmonstatus() {
  json req;
  req["type"] = "list";
  RackmonClient cli;
  std::string resp = cli.request(req.dump());
  json resp_j = json::parse(resp);
  for (const auto& ent : resp_j["data"]) {
    std::cout << "PSU addr " << std::hex << std::setw(2) << std::setfill('0') << int(ent["addr"]);
    std::cout << " - crc errors: " << std::dec << ent["crc_fails"];
    std::cout << ", timeouts: " << std::dec << ent["timeouts"];
    std::cout << ", baud rate: " << std::dec << ent["baudrate"] << std::endl;
  }
}

int main(int argc, char* argv[]) {
  CLI::App app("Rackmon CLI interface");
  app.failure_message(CLI::FailureMessage::help);

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

  // List command
  app.add_subcommand("list", "Return list of Modbus devices")->callback([&]() {
    do_cmd("list", json_fmt);
  });

  // Status command
  app.add_subcommand("legacy_list", "Return list of Modbus devices (Legacy backwards compatibility)")->callback(do_rackmonstatus);

  // Data command (Get monitored data)
  std::string format = "raw";
  auto data = app.add_subcommand("data", "Return detailed monitoring data");
  data->callback([&]() { do_cmd(format + "_data", json_fmt); });
  data->add_set("-f,--format", format, {"raw", "value"}, "Format the data");

  // Profile
  app.add_subcommand("profile", "Print profiling data collected from last read")
      ->callback([&]() { do_cmd("profile", json_fmt); });

  // Pause command
  app.add_subcommand("pause", "Pause monitoring")->callback([&]() {
    do_cmd("pause", json_fmt);
  });

  // Resume command
  app.add_subcommand("resume", "Resume monitoring")->callback([&]() {
    do_cmd("resume", json_fmt);
  });

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  return 0;
}

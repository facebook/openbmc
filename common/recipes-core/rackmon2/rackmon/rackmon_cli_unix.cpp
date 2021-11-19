#include <CLI/CLI.hpp>
#include <nlohmann/json.hpp>
#include "rackmon_svc_unix.hpp"

using nlohmann::json;

static int send_recv(const char* str, size_t len, std::vector<char>& resp) {
  RackmonClient cli;
  cli.send(str, len);
  cli.recv(resp);
  return 0;
}

static void print_json(json& j) {
  std::cout << j.dump(4) << std::endl;
}

static void do_raw_cmd(const std::string& req_s, int timeout, int resp_len) {
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
  std::vector<char> resp;
  std::string req_js = req.dump();
  send_recv(req_js.c_str(), req_js.length(), resp);
  json resp_j = json::parse(resp);
  print_json(resp_j);
}

static void do_cmd(const std::string& type) {
  json req;
  req["type"] = type;
  std::string req_s = req.dump();
  std::vector<char> resp;
  send_recv(req_s.c_str(), req_s.length(), resp);
  json resp_j = json::parse(resp);
  print_json(resp_j);
}

int main(int argc, char* argv[]) {
  CLI::App app("Rackmon CLI interface");
  app.failure_message(CLI::FailureMessage::help);

  // Raw command
  auto raw_cmd = app.add_subcommand("raw", "Execute a RAW request");
  int raw_cmd_timeout = 0;
  raw_cmd->add_option("-t,--timeout", raw_cmd_timeout, "Timeout (ms)");
  int expected_len = 0;
  raw_cmd
      ->add_option(
          "-x,--expected-bytes",
          expected_len,
          "Expected response length (minus CRC)")
      ->required();
  std::string req = "";
  raw_cmd->add_option("cmd", req, "Request command bytes, ex: a40300000008");
  auto status_cmd = app.add_subcommand("status", "Return status of rackmon");
  auto data_cmd = app.add_subcommand("data", "Return detailed monitoring data");
  auto pause_cmd = app.add_subcommand("pause", "Pause monitoring");
  auto resume_cmd = app.add_subcommand("resume", "Resume monitoring");

  app.require_subcommand(/* min */ 1, /* max */ 1);

  CLI11_PARSE(app, argc, argv);

  if (*raw_cmd)
    do_raw_cmd(req, raw_cmd_timeout, expected_len);
  else if (*status_cmd)
    do_cmd("status");
  else if (*data_cmd)
    do_cmd("data");
  else if (*pause_cmd)
    do_cmd("pause");
  else if (*resume_cmd)
    do_cmd("resume");
}

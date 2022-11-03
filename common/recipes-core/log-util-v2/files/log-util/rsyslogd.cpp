#include "rsyslogd.hpp"
#include <errno.h>
#include <array>
#include <cstdio>
#include <exception>
#include <memory>
#include <regex>
#include <string>
#include <cstring>

void rsyslogd::runcmd(const std::string& cmd, std::string& output) {
  // 128char lines.
  std::array<char, 128> out;
  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp)
    throw std::runtime_error("Cannot run: " + cmd);
  while (fgets(out.data(), out.size(), fp) != NULL) {
    output.append(out.data());
  }
  int status = pclose(fp);
  if (status == -1) {
    throw std::runtime_error(std::strerror(errno));
  } else if (status > 0) {
    throw std::runtime_error("Subprocess exited with non-zero code");
  }
}

void rsyslogd::runcmd(const std::string& cmd) {
  if (system(cmd.c_str()) != 0)
    throw std::runtime_error("Sending HUP to rsyslogd failed");
}

int rsyslogd::getpid(void) {
  std::string pid;
  runcmd("pidof rsyslogd", pid);
  static const std::regex num_regex(R"(^(\d+)[\r\n\s]*$)");
  if (std::smatch match; std::regex_match(pid, match, num_regex)) {
    return std::stoi(match[1]);
  }
  throw std::runtime_error(
      "Non numeric PID received from `pidof rsyslogd`: \"" + pid + "\"");
}

void rsyslogd::reload() {
  int pid = getpid();
  std::string cmd = "kill -HUP " + std::to_string(pid);
  runcmd(cmd);
}

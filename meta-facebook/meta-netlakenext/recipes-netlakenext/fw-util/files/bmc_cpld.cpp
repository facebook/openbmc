#include <cstdio>
#include <iostream>
#include <array>
#include <optional>
#include <sys/wait.h>  // for WIFEXITED, WEXITSTATUS, etc.
#include <syslog.h>
#include "bmc_cpld.h"

int BmcCpldComponent::run_command(const std::string& cmd, std::string* output = nullptr) const
{
  std::string fullCmd = cmd + " 2>&1";

  std::array<char, 1024> buffer{};
  FILE* pipe = popen(fullCmd.c_str(), "r");
  if (!pipe) {
    std::cerr << "Failed to execute command: " << cmd << std::endl;
    return -1;
  }

  while (fgets(buffer.data(), buffer.size(), pipe)) {
    std::cout << buffer.data();
    if (output) {
      (*output) += buffer.data();
    }
  }

  int rc = pclose(pipe);
  if (rc == -1) {
    std::cerr << "pclose() failed\n";
    return -1;
  }

  if (WIFEXITED(rc)) {
    int exitCode = WEXITSTATUS(rc);
    if (exitCode != 0) {
      std::cerr << "Command exited with code: " << exitCode << std::endl;
      return -1;
    }
  } else if (WIFSIGNALED(rc)) {
    int sig = WTERMSIG(rc);
    std::cerr << "Command terminated by signal: " << strsignal(sig) << " (" << sig << ")\n";
    return -1;
  }

  return 0;
}

int BmcCpldComponent::print_version()
{
  std::string cmd = "cpld-handler version -b " + this->bus + " -a " + this->address + " -i i2c -c " + this->cpld_type;
  int rc = run_command(cmd);

  return rc;
}

std::optional<std::string> BmcCpldComponent::get_ver_from_string(const std::string& result) const
{
  size_t pos = result.find(": ");
  if (pos == std::string::npos) {
    return std::nullopt;
  }

  std::string hexValue = result.substr(pos + 2);
  hexValue.erase(hexValue.find_last_not_of(" \n\r\t") + 1);

  if (hexValue.empty()) {
    return std::nullopt;
  }

  return hexValue;
}

int BmcCpldComponent::get_version(json& ver_json)
{
  std::string cmd = "cpld-handler version -b " + this->bus + " -a " + this->address + " -i i2c -c " + this->cpld_type;
  std::string result;

  std::cout.setstate(std::ios_base::failbit);
  int rc = run_command(cmd, &result);
  std::cout.clear();

  if (rc != 0) {
    std::cerr << "Failed to get CPLD version (command error)\n";
    ver_json["VERSION"] = "error_returned";
    return FW_STATUS_SUCCESS;
  }

  auto version = get_ver_from_string(result);
  if (!version.has_value()) {
    ver_json["VERSION"] = "not_present";
  } else {
    ver_json["VERSION"] = *version;
  }

  return FW_STATUS_SUCCESS;
}

int BmcCpldComponent::update(const std::string& image)
{
  syslog(LOG_CRIT, "Updating CPLD on baseboard. File: %s", image.c_str());

  std::string cmd = "cpld-handler update -b " + this->bus + " -a " + this->address + " -i i2c -c " + this->cpld_type + " -p " + image;
  int rc = run_command(cmd);

  if (rc != 0) {
    syslog(LOG_CRIT, "Updated CPLD on baseboard. File: %s. Result: Fail", image.c_str());
  } else {
    syslog(LOG_CRIT, "Updated CPLD on baseboard. File: %s. Result: Success", image.c_str());
  }

  return rc;
}

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "fw-util.h"

#define PAGE_SIZE                     0x1000
#define VERIFIED_BOOT_STRUCT_BASE     0x1E720000
#define VERIFIED_BOOT_HARDWARE_ENFORCE(base) \
  *((uint8_t *)(base + 0x215))

using namespace std;

#ifdef __TEST__

// TODO Mocks are yet to be written

int System::runcmd(const string &cmd)
{
  cout << "Running: " << cmd << endl;
  return FW_STATUS_SUCCESS;
}

int System::vboot_support_status(void)
{
  const char *env = std::getenv("FWUTIL_HWENFORCE");
  if (env) {
    return VBOOT_HW_ENFORCE;
  }
  env = std::getenv("FWUTIL_SWENFORCE");
  if (env) {
    return VBOOT_SW_ENFORCE;
  }
  env = std::getenv("FWUTIL_NOENFORCE");
  if (env) {
    return VBOOT_NO_ENFORCE;
  }
  return VBOOT_NO_SUPPORT;
}

bool System::vboot_timestamp(uint32_t& fallback, uint32_t& current) {
  const char* env = std::getenv("FWUTIL_SWENFORCE");
  if (!env) {
    return false;
  }
  fallback = current = 0;
  env = std::getenv("FWUTIL_VBOOT_TSP_CURRENT");
  if (env) {
    current = std::stoi(env);
  }
  env = std::getenv("FWUTIL_VBOOT_TSP_FALLBACK");
  if (env) {
    fallback = std::stoi(env);
  }
  return true;
}

bool System::get_mtd_name(string name, string &dev, size_t& size, size_t& erasesize)
{
  map<string, string> mtd_map = {
    {"flash0", "/dev/mtd5"},
  };
  if (std::getenv("FWUTIL_VBOOT") ||
      std::getenv("FWUTIL_DUALFLASH")) {
    mtd_map["flash1"] = "/dev/mtd12";
  }
  if (std::getenv("FWUTIL_VBOOT")) {
    mtd_map["romx"] = "/dev/mtd0";
  }
  if (mtd_map.find(name) == mtd_map.end())
    return false;
  dev = mtd_map[name];
  size = 512 * 1024;
  erasesize = 64 * 1024;
  return true;
}

string System::version()
{
  static string ret = "fbtp-v10.0";
  const char *env = std::getenv("FWUTIL_VERSION");
  if (env) {
    ret = env;
  }
  return ret;
}

string& System::partition_conf()
{
  static string parts = "./image_parts.json";
  return parts;
}

uint8_t System::get_fru_id(string &name [[maybe_unused]])
{
  const char *env = std::getenv("FWUTIL_FRUID");
  if (!env) {
    return 1;
  }
  return atoi(env);
}

void System::set_update_ongoing(uint8_t fru_id [[maybe_unused]],
                                int timeo [[maybe_unused]])
{
}

bool System::is_update_ongoing(uint8_t fru_id [[maybe_unused]])
{
  return false;
}

bool System::is_sled_cycle_initiated()
{
  return false;
}

bool System::is_healthd_running()
{
  return false;
}

bool System::is_reboot_ongoing()
{
  return false;
}

bool System::is_shutdown_non_executable()
{
  return false;
}

int System::wait_shutdown_non_executable(uint8_t timeout_sec [[maybe_unused]])
{
  return 0;
}

#endif

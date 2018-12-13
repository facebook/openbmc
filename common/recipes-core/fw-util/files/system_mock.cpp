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
  return VBOOT_NO_ENFORCE;
}

bool System::get_mtd_name(string name, string &dev)
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

uint8_t System::get_fru_id(string &name)
{
  const char *env = std::getenv("FWUTIL_FRUID");
  if (!env) {
    return 1;
  }
  return atoi(env);
}

string System::lock_file(string &name)
{
  return "./fw-util-" + name + ".lock";
}

void System::set_update_ongoing(uint8_t fru_id, int timeo)
{
}

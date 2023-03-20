#if (__GNUC__ < 8)
#include <experimental/filesystem>
namespace std
{
    namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif
#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <openbmc/pal.h>
#include <openbmc/vbs.h>
#include <openbmc/kv.hpp>
#include "fw-util.h"

#define PAGE_SIZE                     0x1000
#define VERIFIED_BOOT_STRUCT_BASE     0x1E720000
#define VERIFIED_BOOT_HARDWARE_ENFORCE(base) \
  *((uint8_t *)(base + 0x215))

using namespace std;

#ifndef __TEST__

int System::runcmd(const string &cmd)
{
#ifdef DEBUG
  cout << "Executing: " << cmd << endl;
#endif
  int status = system(cmd.c_str());
  if (status == -1) {
    return 127;
  }
  if (WIFEXITED(status) && (WEXITSTATUS(status) == 0))
    return FW_STATUS_SUCCESS;
  return FW_STATUS_FAILURE;
}

int System::vboot_support_status(void)
{
  struct vbs *v = vboot_status();
  if (!v)
    return VBOOT_NO_SUPPORT;
  if (v->hardware_enforce)
    return VBOOT_HW_ENFORCE;
  if (v->software_enforce)
    return VBOOT_SW_ENFORCE;
  return VBOOT_NO_ENFORCE;
}

bool System::vboot_timestamp(uint32_t& fallback, uint32_t& current) {
  struct vbs *v = vboot_status();
  if (!v) {
    return false;
  }
  fallback = v->uboot_last;
  current = v->uboot_current;
  return true;
}

bool System::get_mtd_name(string name, string &dev, size_t& size, size_t& erasesize)
{
  FILE* partitions = fopen("/proc/mtd", "r");
  char line[256], mnt_name[32];
  unsigned int mtdno;
  bool found = false;

  name.insert(0, 1, '"');
  name += '"';
  dev.clear();

  if (!partitions) {
    return false;
  }
  while (fgets(line, sizeof(line), partitions)) {
    size_t sz, esz;
    if(sscanf(line, "mtd%u: %zx %zx %s",
                &mtdno, &sz, &esz, mnt_name) == 4) {
      if(!strcmp(name.c_str(), mnt_name)) {
        dev = "/dev/mtd";
        dev.append(to_string(mtdno));
        size = sz;
        erasesize = esz;
        found = true;
        break;
      }
    }
  }
  fclose(partitions);
  return found;
}

string System::version()
{
  static string ret = "";
  if (ret == "") {
    char vers[128] = "NA";
    FILE *fp = fopen("/etc/issue", "r");
    if (fp) {
      if (fscanf(fp, "OpenBMC Release %127s\n", vers) == 1) {
        ret = vers;
      }
      fclose(fp);
    }
  }
  return ret;
}

string& System::partition_conf()
{
  static string parts = "/etc/image_parts.json";
  return parts;
}

uint8_t System::get_fru_id(string &name)
{
  uint8_t fru_id;
  if (pal_get_fru_id((char *)name.c_str(), &fru_id))
  {

    // Set to some default FRU which should be present
    // in the system.
    fru_id = 1;
  }
  return fru_id;
}

void System::set_update_ongoing(uint8_t fru_id, int timeo)
{
  pal_set_fw_update_ongoing(fru_id, timeo);
}

bool System::is_update_ongoing(uint8_t fru_id)
{
  return pal_is_fw_update_ongoing(fru_id);
}

bool System::is_sled_cycle_initiated()
{
  return pal_sled_cycle_prepare();
}

bool System::is_healthd_running()
{
  static const char* cmd = "ps -w | grep [/]usr/local/bin/healthd > /dev/null";
  return (system(cmd) == 0) ? true : false;
}

bool System::is_reboot_ongoing()
{
  const std::string key = "reboot_ongoing";
  std::string val;
  try {
    val = kv::get(key, kv::region::temp);
    if (val.find_first_of("1") == std::string::npos) {
      return false;
    }
  } catch(std::exception& e) {
    return false;
  }
  return true;
}

bool System::is_shutdown_non_executable()
{
  const std::vector<std::string> reboot_cmds = {
    "/sbin/shutdown.sysvinit",
    "/sbin/halt.sysvinit",
    "/sbin/init"};

  const std::filesystem::perms non_exec = std::filesystem::perms::owner_exec |
                                          std::filesystem::perms::group_exec |
                                          std::filesystem::perms::others_exec;

  for (size_t i=0; i<reboot_cmds.size(); i++) {
    std::filesystem::perms f_perms;
    f_perms = std::filesystem::status(reboot_cmds[i]).permissions();
    if ((f_perms & non_exec) != std::filesystem::perms::none) {
      return false;
    }
  }

  return true;
}

int System::wait_shutdown_non_executable(uint8_t timeout_sec)
{
  if (!this->is_healthd_running()) {
    // for those platforms does't have healthd service
    // just assume permission changed successfully
    return 0;
  }

  while (timeout_sec > 0) {
    if (this->is_shutdown_non_executable()) {
      return 0;
    }
    sleep(1);
    timeout_sec -= 1;
  };

  if (this->is_shutdown_non_executable()) {
      return 0;
  }
  return -1;
}

#endif

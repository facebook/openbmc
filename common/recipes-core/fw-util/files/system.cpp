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
#include "fw-util.h"

#define PAGE_SIZE                     0x1000
#define VERIFIED_BOOT_STRUCT_BASE     0x1E720000
#define VERIFIED_BOOT_HARDWARE_ENFORCE(base) \
  *((uint8_t *)(base + 0x215))

using namespace std;

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

bool System::get_mtd_name(string name, string &dev)
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
    if(sscanf(line, "mtd%d: %*x %*x %s",
                &mtdno, mnt_name) == 2) {
      if(!strcmp(name.c_str(), mnt_name)) {
        dev = "/dev/mtd";
        dev.append(to_string(mtdno));
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
      if (fscanf(fp, "OpenBMC Release %s\n", vers) == 1) {
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

string System::lock_file(string &name)
{
  return "/var/run/fw-util-" + name + ".lock";
}

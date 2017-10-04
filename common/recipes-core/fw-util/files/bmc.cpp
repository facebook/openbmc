#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include "fw-util.h"

#ifndef VERIFIED_BOOT_LOCKDOWN
#define VERIFIED_BOOT_LOCKDOWN 0
#endif

using namespace std;

static int system_w(const string &cmd)
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

static bool get_mtd_name(const char* name, char* dev)
{
  FILE* partitions = fopen("/proc/mtd", "r");
  char line[256], mnt_name[32];
  unsigned int mtdno;
  bool found = false;

  if (!partitions) {
    return false;
  }
  if (dev) {
    dev[0] = '\0';
  }
  while (fgets(line, sizeof(line), partitions)) {
    if(sscanf(line, "mtd%d: %*x %*x %s",
                 &mtdno, mnt_name) == 2) {
      if(!strcmp(name, mnt_name)) {
        if (dev) {
          sprintf(dev, "/dev/mtd%d", mtdno);
        }
        found = true;
        break;
      }
    }
  }
  fclose(partitions);
  return found;
}

/* Flashes full image to provided MTD. Gets version from 
 * /etc/issue */
class BmcComponent : public Component {
  protected:
    string _mtd_name;
    size_t _writable_offset;
  public:
    BmcComponent(string fru, string comp, string mtd, size_t w_offset = 0)
      : Component(fru, comp), _mtd_name(mtd), _writable_offset(w_offset) {}
    int update(string image_path)
    {
      char dev[12];
      int ret;
      string flash_image = image_path;
      stringstream cmd_str;

      if (_mtd_name == "") {
        // Upgrade not supported
        return FW_STATUS_NOT_SUPPORTED;
      }

      if (!get_mtd_name(_mtd_name.c_str(), dev)) {
        cerr << "Failed to get device for " << _mtd_name << endl;
        return FW_STATUS_FAILURE;
      }
      cout << "Flashing to device: " << string(dev) << endl;
      if (_writable_offset > 0) {
        flash_image = image_path + "-tmp";
        // Create a temp image with the first 'writable_offset' bytes removed
        cmd_str << "tail -c+" << _writable_offset << " "
          << image_path << " > " << flash_image;
        ret = system_w(cmd_str.str());
        if (ret != 0) {
          cerr << "Creating temp image: " << flash_image << " failed" << endl;
          return FW_STATUS_FAILURE;
        }
        cmd_str.clear();
        cmd_str.str("");
      }
      cmd_str << "flashcp -v " << flash_image << " " << string(dev);
      ret = system_w(cmd_str.str());
      if (_writable_offset > 0) {
        remove(flash_image.c_str());
      }
      return ret;
    }
    int print_version()
    {
      char vers[128] = "NA";
      FILE *fp = fopen("/etc/issue", "r");
      if (fp) {
        fscanf(fp, "OpenBMC Release %s\n", vers);
        fclose(fp);
      }
      cout << _component << " Version: " << string(vers) << endl;
      return FW_STATUS_SUCCESS;
    }
};

/* Inherits the flashing bits from BmcComponent, overrides
 * the getting version to get it from the u-boot mtd */
class BmcUbootVersionComponent : public BmcComponent {
  private:
    string _vers_mtd;
  public:
    BmcUbootVersionComponent(string fru, string comp, string mtd, string vers_mtd)
      : BmcComponent(fru, comp, mtd), _vers_mtd(vers_mtd) {}
  int print_version()
  {
    char vers[128] = "NA";
    char mtd[32];

    if (get_mtd_name(_vers_mtd.c_str(), mtd)) {
      char cmd[128];
      FILE *fp;
      sprintf(cmd, "strings %s | grep 'U-Boot 2016.07'", mtd);
      fp = popen(cmd, "r");
      if (fp) {
        char line[256];
        char min[32];
        while (fgets(line, sizeof(line), fp)) {
          int ret;
          ret = sscanf(line, "U-Boot 2016.07%*[ ]%[^ \n]%*[ ](%*[^)])\n", min);
          if (ret == 1) {
            sprintf(vers, "%s", min);
            break;
          }
        }
        pclose(fp);
      }
    }
    cout << "ROM Version: " << string(vers) << endl;
    return FW_STATUS_SUCCESS;
  }
};

class SystemConfig {
  public:
    bool dual_flash;
  SystemConfig() {
    // If flash1 exists, then we have two flashes
    if (get_mtd_name("\"flash1\"", NULL)) {
      dual_flash = true;
    } else {
      dual_flash = false;
    }
    if (dual_flash) {
#if VERIFIED_BOOT == 1
      // Verified boot supported and in dual-flash mode.
#if VERIFIED_BOOT_LOCKDOWN == 1
      // Locked down, Update from a 64k offset.
      static BmcComponent bmc("bmc", "bmc", "\"flash1rw\"", 64 * 1024);
      // Locked down, Allow getting version of ROM, but do not allow
      // upgrading it.
      static BmcUbootVersionComponent rom("bmc", "rom", "", "\"rom\"");
#else
      // Unlocked Flash, Upgrade the full BMC flash1.
      static BmcComponent bmc("bmc", "bmc", "\"flash1\"");
      // Unlocked flash, Allow upgrading the ROM.
      static BmcUbootVersionComponent rom("bmc", "rom", "\"flash0\"", "\"rom\"");
#endif
#else
      // non-verified-boot. BMC boots off of flash0.
      static BmcComponent bmc("bmc", "bmc", "\"flash0\"");
      // Dual flash supported, but not in verified boot format.
      // Allow flashing the second flash. Read version from u-boot partition.
      static BmcUbootVersionComponent bmcalt("bmc", "flash1", "\"flash1\"", "\"u-boot\"");
#endif
    } else {
      // We just have the one flash. Allow upgrading flash0.
      static BmcComponent bmc("bmc", "bmc", "\"flash0\"");
    }
  }
};

SystemConfig sysconf;

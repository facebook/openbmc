#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <sys/mman.h>
#include <syslog.h>
#include <openbmc/pal.h>
#include "bmc.h"

using namespace std;

#define BMC_RW_OFFSET               (64 * 1024)
#define ROMX_SIZE                   (84 * 1024)

int BmcComponent::update(string image_path)
{
  string dev;
  int ret, pfr_active;
  string flash_image = image_path;
  stringstream cmd_str;

  if (_mtd_name == "") {
    // Upgrade not supported
    return FW_STATUS_NOT_SUPPORTED;
  }

  pfr_active = pal_is_pfr_active();
  if (is_valid(image_path, (pfr_active == PFR_ACTIVE)) == false) {
    system.error << image_path << " is not a valid BMC image for " << system.name() << endl;
    return FW_STATUS_FAILURE;
  }

  if (pfr_active == PFR_ACTIVE) {
    if (!sys.get_mtd_name(string("stg-bmc"), dev)) {
      return FW_STATUS_FAILURE;
    }

    system.output << "Flashing to device: " << dev << endl;
    cmd_str << "flashcp -v " << flash_image << " " << dev;
    ret = system.runcmd(cmd_str.str());
    return pal_fw_update_finished(0, _component.c_str(), ret);
  }

  if (!system.get_mtd_name(_mtd_name, dev)) {
    system.error << "Failed to get device for " << _mtd_name << endl;
    return FW_STATUS_FAILURE;
  }

  syslog(LOG_CRIT, "BMC fw upgrade initiated");

  system.output << "Flashing to device: " << dev << endl;
  if (_skip_offset > 0 || _writable_offset > 0) {
    flash_image = image_path + "-tmp";
    // Ensure that we are not overwriting an existing file.
    while (access( flash_image.c_str(), F_OK  ) != -1) {
      flash_image.append("_");
    }
    // Open input image and seek skipping to the writable offset.
    int fd_r = open(image_path.c_str(), O_RDONLY);
    if (fd_r < 0) {
      system.error << "Cannot open " << image_path << " for reading" << endl;
      return FW_STATUS_FAILURE;
    }
    if (lseek(fd_r, _skip_offset, SEEK_SET) != (off_t)_skip_offset) {
      close(fd_r);
      system.error << "Cannot seek " << image_path << endl;
      return FW_STATUS_FAILURE;
    }
    // create tmp file for writing.
    int fd_w = open(flash_image.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd_w < 0) {
      system.error << "Cannot write to " << flash_image << endl;
      close(fd_r);
      return FW_STATUS_FAILURE;
    }
    uint8_t buf[1024];
    size_t r_b;

    if (_skip_offset > _writable_offset) {
      size_t copy = _skip_offset - _writable_offset;
      int fd_d = open(dev.c_str(), O_RDONLY);
      if (fd_d < 0) {
        close(fd_r);
        close(fd_w);
        return FW_STATUS_FAILURE;
      }
      while (copy > 0) {
        size_t to_copy = copy > sizeof(buf) ? sizeof(buf) : copy;
        r_b = read(fd_d, buf, to_copy);
        if (r_b != to_copy) {
          close(fd_r);
          close(fd_w);
          close(fd_d);
          return FW_STATUS_FAILURE;
        }
        if (write(fd_w, buf, to_copy) != (int)to_copy) {
          close(fd_r);
          close(fd_w);
          close(fd_d);
          return FW_STATUS_FAILURE;
        }
        copy -= to_copy;
      }
      close(fd_d);
    }

    // Copy from r to w.
    while ((r_b = read(fd_r, buf, 1024)) > 0) {
      if (write(fd_w, buf, r_b) != (int)r_b) {
        close(fd_r);
        close(fd_w);
        return -1;
      }
    }
    close(fd_r);
    close(fd_w);
  }
  cmd_str << "flashcp -v " << flash_image << " " << dev;
  ret = system.runcmd(cmd_str.str());
  if (_writable_offset > 0) {
    // this is a temp. file, remove it.
    remove(flash_image.c_str());
  }

  // If flashcp cmd was successful, keep historical info that BMC fw was upgraded
  if (ret == 0) {
    syslog(LOG_CRIT, "BMC fw upgrade completed. Version: %s", get_bmc_version(dev).c_str());
  }

  return ret;
}

std::string BmcComponent::get_bmc_version()
{
  std::string bmc_ver = "NA";
  std::string mtd;
  if (!system.get_mtd_name(_vers_mtd, mtd)) {
    return bmc_ver;
  }
  return get_bmc_version(mtd);
}

std::string BmcComponent::get_bmc_version(const std::string &mtd)
{
  // parsering the image to get the version string
  std::string bmc_ver = "NA";
  char cmd[128];
  FILE *fp;

  snprintf(cmd, sizeof(cmd),
      "strings %s | grep -E 'U-Boot 20[[:digit:]]{2}\\.[[:digit:]]{2}'", mtd.c_str());
  fp = popen(cmd, "r");
  if (fp) {
    char line[256];
    char *ver = 0;
    while (fgets(line, sizeof(line), fp)) {
      int ret;
      ret = sscanf(line, "U-Boot 20%*2d.%*2d%*[ ]%m[^ \n]%*[ ](%*[^)])\n", &ver);
      if (1 == ret) {
        bmc_ver = ver;
        break;
      }
    }
    if (ver) {
       free(ver);
    }
    pclose(fp);
  }

  return bmc_ver;
}

int BmcComponent::print_version()
{
  string mtd;

  string comp = _component;
  std::transform(comp.begin(), comp.end(),comp.begin(), ::toupper);

  if (_vers_mtd == "") {
    system.output << comp << " Version: " << system.version() << endl;
    return FW_STATUS_SUCCESS;
  }

  system.output << comp << " Version: " << get_bmc_version() << endl;
  return FW_STATUS_SUCCESS;
}

void BmcComponent::get_ver_in_json(json& j) {
  if (_vers_mtd == "") {
    j["VERSION"] = system.version();
  } else {
    j["VERSION"] = get_bmc_version();
  }
}

class SystemConfig {
  public:
    bool dual_flash;
    System system;
  SystemConfig() : system() {
    // If flash1 exists, then we have two flashes
    if (system.get_mtd_name("flash1")) {
      dual_flash = true;
    } else {
      dual_flash = false;
    }
    if (dual_flash) {
      int vboot = system.vboot_support_status();
      if (vboot != VBOOT_NO_SUPPORT)  {
        // If verified boot is enabled, we should
        // have the romx partition
        if (vboot == VBOOT_HW_ENFORCE) {
          // Locked down, Update from a 64k offset.
          static BmcComponent bmc("bmc", "bmc", system, "flash1rw", "u-boot", BMC_RW_OFFSET, ROMX_SIZE);
          // Locked down, Allow getting version of ROM, but do not allow
          // upgrading it.
          static BmcComponent rom("bmc", "rom", system, "", "u-bootro");
        } else {
          // Unlocked Flash, Upgrade the full BMC flash1.
          static BmcComponent bmc("bmc", "bmc", system, "flash1", "u-boot");
          // Unlocked flash, Allow upgrading the ROM.
          static BmcComponent rom("bmc", "rom", system, "flash0", "u-bootro");
        }
      } else {
        // non-verified-boot. BMC boots off of flash0.
        static BmcComponent bmc("bmc", "bmc", system, "flash0", "u-boot");
        // Dual flash supported, but not in verified boot format.
        // Allow flashing the second flash. Read version from u-boot partition.
        static BmcComponent bmcalt("bmc", "altbmc", system, "flash1", "u-bootx");
      }
      // Verified boot supported and in dual-flash mode.
    } else {
      // We just have the one flash. Allow upgrading flash0.
      static BmcComponent bmc("bmc", "bmc", system, "flash0");
    }
  }
};

SystemConfig bmc_sysconf;

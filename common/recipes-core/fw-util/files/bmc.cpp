#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "fw-util.h"

using namespace std;

#define BMC_RW_OFFSET               (64 * 1024)
#define ROMX_SIZE                   (84 * 1024)

#define PAGE_SIZE                     0x1000
#define VERIFIED_BOOT_STRUCT_BASE     0x1E720000
#define VERIFIED_BOOT_HARDWARE_ENFORCE(base) \
  *((uint8_t *)(base + 0x215))

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

static bool vboot_hardware_enforce(void)
{
  int mem_fd;
  uint8_t *vboot_base;
  uint8_t hw_enforce_flag;

  mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (mem_fd < 0) {
    cerr << "Error opening /dev/mem" << endl;
    return false;
  }

  vboot_base = (uint8_t *)mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, VERIFIED_BOOT_STRUCT_BASE);
  if (!vboot_base) {
    cerr << "Error mapping VERIFIED_BOOT_STRUCT_BASE" << endl;
    close(mem_fd);
    return false;
  }

  hw_enforce_flag = VERIFIED_BOOT_HARDWARE_ENFORCE(vboot_base);

  munmap(vboot_base, PAGE_SIZE);
  close(mem_fd);

  return hw_enforce_flag == 1 ? true : false;
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

string get_machine()
{
  string ret = "";
  char machine[128] = "NA";
  FILE *fp = fopen("/etc/issue", "r");
  if (fp) {
    if (fscanf(fp, "OpenBMC Release %[^ \t\n-]", machine) == 1) {
      ret = machine;
    }
    fclose(fp);
  }
  return ret;
}

bool is_image_valid(string &image)
{
  string curr_machine = get_machine();
  string cmd = "grep \"U-Boot 2016.07 " + curr_machine +
    "\" " + image + " 2> /dev/null | wc -l";
  FILE *fp = popen(cmd.c_str(), "r");
  if (!fp) {
    return false;
  }
  bool ret = false;
  int num = 0;
  if (fscanf(fp, "%d", &num) == 1 && num > 0) {
    ret = true;
  }
  pclose(fp);
  return ret;
}

/* Flashes full image to provided MTD. Gets version from 
 * /etc/issue */
class BmcComponent : public Component {
  protected:
    string _mtd_name;
    size_t _writable_offset;
    size_t _skip_offset;
  public:
    BmcComponent(string fru, string comp, string mtd, size_t w_offset = 0, size_t skip_offset = 0)
      : Component(fru, comp), _mtd_name(mtd), _writable_offset(w_offset), _skip_offset(skip_offset) {}
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

      if (is_image_valid(image_path) == false) {
        cerr << image_path << " is not a valid BMC image for " << get_machine() << endl;
        return FW_STATUS_FAILURE;
      }

      if (!get_mtd_name(_mtd_name.c_str(), dev)) {
        cerr << "Failed to get device for " << _mtd_name << endl;
        return FW_STATUS_FAILURE;
      }
      cout << "Flashing to device: " << string(dev) << endl;
      if (_skip_offset > 0 || _writable_offset > 0) {
        flash_image = image_path + "-tmp";
        // Ensure that we are not overwriting an existing file.
        while (access( flash_image.c_str(), F_OK  ) != -1) {
          flash_image.append("_");
        }
        // Open input image and seek skipping to the writable offset.
        int fd_r = open(image_path.c_str(), O_RDONLY);
        if (fd_r < 0) {
          cerr << "Cannot open " << image_path << " for reading" << endl;
          return FW_STATUS_FAILURE;
        }
        if (lseek(fd_r, _skip_offset, SEEK_SET) != (off_t)_skip_offset) {
          close(fd_r);
          cerr << "Cannot seek " << image_path << endl;
          return FW_STATUS_FAILURE;
        }
        // create tmp file for writing.
        int fd_w = open(flash_image.c_str(), O_WRONLY | O_CREAT);
        if (fd_w < 0) {
          cerr << "Cannot write to " << flash_image << endl;
          close(fd_r);
          return FW_STATUS_FAILURE;
        }
        uint8_t buf[1024];
        size_t r_b;

        if (_skip_offset > _writable_offset) {
          size_t copy = _skip_offset - _writable_offset;
          int fd_d = open(dev, O_RDONLY);
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
              close(fd_d);
              close(fd_d);
              return FW_STATUS_FAILURE;
            }
            write(fd_w, buf, to_copy);
            copy -= to_copy;
          }
          close(fd_d);
        }

        // Copy from r to w.
        while ((r_b = read(fd_r, buf, 1024)) > 0) {
          write(fd_w, buf, r_b);
        }
        close(fd_r);
        close(fd_w);
      }
      cmd_str << "flashcp -v " << flash_image << " " << string(dev);
      ret = system_w(cmd_str.str());
      if (_writable_offset > 0) {
        // this is a temp. file, remove it.
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
      cout << "BMC Version: " << string(vers) << endl;
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
      if (get_mtd_name("\"romx\"", NULL)) {
        // If verified boot is enabled, we should
        // have the romx partition
        if (vboot_hardware_enforce()) {
          // Locked down, Update from a 64k offset.
          static BmcComponent bmc("bmc", "bmc", "\"flash1rw\"", BMC_RW_OFFSET, ROMX_SIZE);
          // Locked down, Allow getting version of ROM, but do not allow
          // upgrading it.
          static BmcUbootVersionComponent rom("bmc", "rom", "", "\"rom\"");
        } else {
          // Unlocked Flash, Upgrade the full BMC flash1.
          static BmcComponent bmc("bmc", "bmc", "\"flash1\"");
          // Unlocked flash, Allow upgrading the ROM.
          static BmcUbootVersionComponent rom("bmc", "rom", "\"flash0\"", "\"rom\"");
        }
      } else {
        // non-verified-boot. BMC boots off of flash0.
        static BmcComponent bmc("bmc", "bmc", "\"flash0\"");
        // Dual flash supported, but not in verified boot format.
        // Allow flashing the second flash. Read version from u-boot partition.
        static BmcUbootVersionComponent bmcalt("bmc", "flash1", "\"flash1\"", "\"u-boot\"");
      }
      // Verified boot supported and in dual-flash mode.
    } else {
      // We just have the one flash. Allow upgrading flash0.
      static BmcComponent bmc("bmc", "bmc", "\"flash0\"");
    }
  }
};

SystemConfig bmc_sysconf;

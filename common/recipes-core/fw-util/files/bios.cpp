#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <regex>
#include <thread>
#include <chrono>
#include <sys/stat.h>
#include <openbmc/pal.h>
#include <syslog.h>
#include "bios.h"

#define BIOS_VER_REGION_SIZE (4*1024*1024)
#define BIOS_ERASE_PKT_SIZE  (64*1024)
#define BIOS_VER_MAX_SIZE (32)

using namespace std;

int BiosComponent::extract_signature(const char *path, std::string &sig)
{
  int fd = 0, ret = -1;
  int offs, rcnt, end;
  uint8_t *buf;
  uint8_t ver_sig[] = { 0x46, 0x49, 0x44, 0x04, 0x78, 0x00 };
  struct stat st;

  sig.clear();

  if (stat(path, &st)) {
    return -1;
  }

  if (st.st_size < BIOS_VER_REGION_SIZE) {
    return -1;
  }

  buf = (uint8_t *)malloc(BIOS_VER_REGION_SIZE);
  if (!buf) {
    return -1;
  }

  do {
    fd = open(path, O_RDONLY);
    if (fd < 0) {
      break;
    }

    offs = st.st_size - BIOS_VER_REGION_SIZE;
    if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
      printf("%s: lseek to %d failed\n", __func__, offs);
      break;
    }

    offs = 0;
    while (offs < BIOS_VER_REGION_SIZE) {
      rcnt = ((offs + BIOS_ERASE_PKT_SIZE) <= BIOS_VER_REGION_SIZE) ? BIOS_ERASE_PKT_SIZE : (BIOS_VER_REGION_SIZE - offs);
      rcnt = read(fd, (buf + offs), rcnt);
      if (rcnt <= 0) {
        if (errno == EINTR) {
          continue;
        }
        printf("%s: unexpected rcnt: %d", __func__, rcnt);
        break;
      }
      offs += rcnt;
    }
    if (offs < BIOS_VER_REGION_SIZE) {
      break;
    }

    end = BIOS_VER_REGION_SIZE - sizeof(ver_sig);
    // Scan through the region looking for the version signature.
    for (offs = 0; offs < end; offs++) {
      if (!memcmp(buf+offs, ver_sig, sizeof(ver_sig))) {
        offs += sizeof(ver_sig);
        for (int nc = 0; offs < end && nc < BIOS_VER_MAX_SIZE; offs++, nc++) {
          if (buf[offs] == '\0' || !isprint(buf[offs])) {
            break;
          }
          char c = (char)buf[offs];
          sig.append(1, c);
        }
        ret = 0;
        break;
      }
    }
  } while (0);

  free(buf);
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

int BiosComponent::check_image(const char *path)
{
  string vers;
  smatch sm;
  regex re(_ver_prefix);

  if (extract_signature(path, vers)) {
    return -1;
  }
  if (!regex_match(vers, sm, re)) {
    return -1;
  }
  return 0;
}

int BiosComponent::update(std::string image, bool force) {
  int retry = 0, poff_retry = 3;
  int ret = 0;
  uint8_t fruid = 1;
  uint8_t status;
  const int max_retry_power_ctl = 3;
  const int max_retry_me_recovery = 15;
  bool setLow = true;

  if (pal_get_fru_id((char *)_fru.c_str(), &fruid)) {
    return -1;
  }

  if (!force) {
    ret = check_image(image.c_str());
    if (ret) {
      sys().error << "Invalid image. Stopping the update!" << endl;
      return -1;
    }
  }

  while (poff_retry > 0) {
    retry = max_retry_power_ctl;
    pal_set_server_power(fruid, SERVER_POWER_OFF);
    while (retry > 0) {
      if (!pal_get_server_power(fruid, &status) && (status == SERVER_POWER_OFF)) {
        break;
      }
      if ((--retry) > 0) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
      }
    }
    if (retry <= 0) {
      sys().error << "Failed to Power Off Server. Stopping the update!\n";
      return -1;
    }

    ret = setMeRecovery(max_retry_me_recovery);
    // If we are doing a forced update, ignore this error.
    if (!force && ret) {
      return -1;
    }

    if (setDeepSleepWell(setLow))
      syslog(LOG_DEBUG, "Set PWRGD_DSW_PWROK low failed");

    // wait for ME changing mode
    std::this_thread::sleep_for(std::chrono::seconds(5));

    if (!pal_get_server_power(fruid, &status) && (status == SERVER_POWER_OFF)) {
      break;
    }
    if ((--poff_retry) > 0) {
      syslog(LOG_WARNING, "System might wake from S5, try Power Off again");
    }
  }
  if (poff_retry <= 0) {
    sys().error << "ERROR: failed to Power Off Server. Stopping the update!\n";
    return -1;
  }

  ret = GPIOSwitchedSPIMTDComponent::update(image);

  if (ret == 0) {
    reboot(fruid);
  }
  return ret;
}

int BiosComponent::update(string image) {
  return update(image, 0);
}

int BiosComponent::fupdate(string image) {
  return update(image, 1);
}

int BiosComponent::setDeepSleepWell(bool setting) {
  return 0;
}

int BiosComponent::reboot(uint8_t fruid) {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  pal_power_button_override(fruid);
  std::this_thread::sleep_for(std::chrono::seconds(10));
  return pal_set_server_power(fruid, SERVER_POWER_ON);
}

int BiosComponent::setMeRecovery(uint8_t retry) {

  while (retry > 0) {
    if (sys().runcmd("/usr/local/bin/me-util 0xB8 0xDF 0x57 0x01 0x00 0x01 > /dev/null") == 0) {
      break;
    }
    if ((--retry) > 0) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
  if (retry == 0) {
    sys().error << "ERROR: unable to put ME in recovery mode!" << endl;
    syslog(LOG_ERR, "Unable to put ME in recovery mode!\n");
    return -1;
  }
  return 0;
}

int BiosComponent::get_version(json& j) {
  uint8_t ver[32] = {0};
  uint8_t fruid = 1;
  int end;

  j["PRETTY_COMPONENT"] = "BIOS";
  pal_get_fru_id((char *)_fru.c_str(), &fruid);
  if (!pal_get_sysfw_ver(fruid, ver)) {
    // BIOS version response contains the length at offset 2 followed by ascii string
    if ((end = 3+ver[2]) > (int)sizeof(ver)) {
      end = sizeof(ver);
    }
    string sver((char *)ver + 3);
    j["VERSION"] = sver.substr(0, end - 3);
  } else {
    j["VERISON"] = "NA";
  }

  return 0;
}

#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <stdlib.h>
#include <errno.h>
#include <openbmc/vbs.h>
#include <openbmc/kv.h>

#define DEVICE_TREE_HEADER_SIZE (4*10)
#define MTD_REGION_SIZE 0x60000
#define MTD_PKT_SIZE 0x10000
#define ROM_MTD "/dev/mtd7" // BMC flash0
#define ROMX_MTD "/dev/mtd0" // BMC flash1

int check_lock_signed(const char *path, uint8_t * img_sig, uint8_t *img_lock)
{
  int fd = 0, ret = -1;
  int offs, rcnt, end, size_pos, size, dt_offs;
  uint8_t *buf;
  uint8_t magic_number[] = { 0xd0, 0x0d, 0xfe, 0xed}; // magic number
  uint8_t signed_string[] = { 0x73, 0x69, 0x67, 0x6e, 0x61, 0x74, 0x75, 0x72, 0x65}; // signature
  uint8_t locked_string[] = { 0x68, 0x77, 0x6c, 0x6f, 0x63, 0x6b}; // hwlock

  *img_lock = 0x00;
  *img_sig = 0x00;

  buf = (uint8_t *)malloc(MTD_REGION_SIZE);
  if (!buf) {
    syslog(LOG_WARNING,"%s: malloc failed", __func__);
    return -1;
  }

  do {
    fd = open(path, O_RDONLY);
    if (fd < 0) {
      syslog(LOG_WARNING,"%s: %s open failed", __func__, path);
      break;
    }

    offs = 0;
    if (lseek(fd, offs, SEEK_SET) != (off_t)offs) {
      syslog(LOG_WARNING,"%s: lseek to %d failed", __func__, offs);
      break;
    }

    offs = 0;
    while (offs < MTD_REGION_SIZE) {
      rcnt = ((offs + MTD_PKT_SIZE) <= MTD_REGION_SIZE) ? MTD_PKT_SIZE : (MTD_REGION_SIZE - offs);
      rcnt = read(fd, buf, MTD_REGION_SIZE);
      if (rcnt <= 0) {
        if (errno == EINTR) {
          continue;
        }
        syslog(LOG_WARNING,"%s: unexpected rcnt: %d", __func__, rcnt);
        break;
      }
      offs += rcnt;
    }
    if (offs < MTD_REGION_SIZE) {
      break;
    }

    end = MTD_REGION_SIZE - sizeof(magic_number);
    // Scan through the region looking for the version signature.
    for (offs = 0; offs < end; offs++) {
      if (!memcmp(buf+offs, magic_number, sizeof(magic_number))) {
        size_pos = offs + sizeof(magic_number);
        size = buf[size_pos]* 0x1000000 + buf[size_pos+1] * 0x10000 + buf[size_pos+2]* 0x100 + buf[size_pos+3];
        ret = 0;
        for (dt_offs = offs + DEVICE_TREE_HEADER_SIZE ; dt_offs < offs + size; dt_offs++) { // DEVICE_TREE_HEADER_SIZE
          if (!memcmp(buf+dt_offs, locked_string, sizeof(locked_string))) {
            *img_lock = 0x01;
          }
          if (!memcmp(buf+dt_offs, signed_string, sizeof(signed_string))) {
            *img_sig = 0x01;
          }
          if (*img_lock && *img_sig) //find lock snd sig
            break;
        }
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

bool get_mtd_name(char* mtd_name)
{
    FILE* partitions = fopen("/proc/mtd", "r");
    char line[256], mnt_name[32];
    char uboot_name[] = "\"u-bootro\"";
    unsigned int mtd_no;
    bool found = false;

    if (!partitions) {
      return false;
    }
    while (fgets(line, sizeof(line), partitions)) {
      if (sscanf(line, "mtd%d: %*x %*x %s",
            &mtd_no, mnt_name) == 2)  {
        if (!strcmp(uboot_name, mnt_name)) {
          sprintf(mtd_name, "/dev/mtd%d", mtd_no);
          found = true;
          break;
        }
      }
    }
    fclose(partitions);
    return found;
}

bool get_rom_ver_from_mtd(char *ver_rom, char *ver_uboot)
{
  char mtd_name[32];
  if (!get_mtd_name(mtd_name)) {
    return false;
  }

  char cmd[128];
  FILE *fp;
  snprintf(cmd, sizeof(cmd),
      "strings %s | grep -E 'U-Boot 20[[:digit:]]{2}\\.[[:digit:]]{2}'", mtd_name);
  fp = popen(cmd, "r");
  if (!fp) {
    return false;
  }

  char line[256];
  bool found = false;
  while (fgets(line, sizeof(line), fp)) {
    int ret;
    ret = sscanf(line, "U-Boot %[^ \n]%*[ ]%[^ \n]%*[ ](%*[^)])\n",
                 ver_uboot, ver_rom);
    if (ret == 2) {
      found = true;
      break;
    }
  }

  pclose(fp);
  return found;
}

bool get_rom_ver(char *ver_rom, char *ver_uboot)
{
  if((0 == kv_get("rom_version", ver_rom, NULL, 0)) &&
     (0 == kv_get("rom_uboot_version", ver_uboot, NULL, 0))) {
    return true;
  }

  if (get_rom_ver_from_mtd(ver_rom, ver_uboot)) {
    kv_set("rom_version", ver_rom, 0, 0);
    kv_set("rom_uboot_version", ver_uboot, 0, 0);
    return true;
  }

  return false;
}

int main(int argc, char *argv[])
{
  char buf[128];
  char ver_rom[MAX_VALUE_LEN] = {0};
  char ver_uboot[MAX_VALUE_LEN] = {0};
  struct vbs *v;
  uint8_t img_sig = 0, img_lock = 0;

  if (!vboot_supported()) {
    printf("Verified boot is not supported on this platform!\n");
    return 0;
  }

  v = vboot_status();
  if (!v) {
    printf("ERROR: Could not read the verified boot status\n");
    return -1;
  }

  printf("ROM executed from:       0x%08x\n", v->rom_exec_address);
  printf("ROM KEK certificates:    0x%08x\n", v->rom_keys);
  printf("ROM handoff marker:      0x%08x\n", v->rom_handoff);
  printf("U-Boot executed from:    0x%08x\n", v->uboot_exec_address);
  printf("U-Boot certificates:     0x%08x\n", v->subordinate_keys);
  printf("Certificates fallback:   %s\n",
      vboot_time(buf, sizeof(buf), v->subordinate_last));
  printf("Certificates time:       %s\n",
      vboot_time(buf, sizeof(buf), v->subordinate_current));
  printf("U-Boot fallback:         %s\n",
      vboot_time(buf, sizeof(buf), v->uboot_last));
  printf("U-Boot time:             %s\n",
      vboot_time(buf, sizeof(buf), v->uboot_current));
  printf("Kernel fallback:         %s\n",
      vboot_time(buf, sizeof(buf), v->kernel_last));
  printf("Kernel time:             %s\n",
      vboot_time(buf, sizeof(buf), v->kernel_current));
  printf("Flags force_recovery:    0x%02x\n", v->force_recovery);
  printf("Flags hardware_enforce:  0x%02x\n", v->hardware_enforce);
  printf("Flags software_enforce:  0x%02x\n", v->software_enforce);
  printf("Flags recovery_boot:     0x%02x\n", v->recovery_boot);
  printf("Flags recovery_retried:  0x%02x\n", v->recovery_retries);
  printf("\n");
  if (get_rom_ver(ver_rom, ver_uboot)) {
    check_lock_signed(ROM_MTD,&img_sig,&img_lock);
    printf("ROM version:             %s\n", ver_rom);
    printf("ROM U-Boot version:      %s\n", ver_uboot);
    printf("FLASH0 Signed:           0x%02x\n", img_sig);
    printf("FLASH0 Locked:           0x%02x\n", img_lock);
    check_lock_signed(ROMX_MTD,&img_sig,&img_lock);
    printf("FLASH1 Signed:           0x%02x\n", img_sig);
    printf("FLASH1 Locked:           0x%02x\n", img_lock);
    printf("\n");
  }
  printf("Status CRC: 0x%04x\n", v->crc);
  printf("TPM status  (%d)\n", v->error_tpm);
  printf("Status type (%d) code (%d)\n", v->error_type, v->error_code);
  printf("%s\n", vboot_error(v->error_code));

  return 0;
}

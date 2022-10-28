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
#include <libfdt.h>
#define FIT_SIG_NODENAME	"signature"
#define FIT_LOCK_NODENAME	"hwlock"

#define DEVICE_TREE_HEADER_SIZE (4*10)
#define MTD_PKT_SIZE 0x10000

static uint32_t fdt_getprop_u32(const void *fdt, int node, const char *prop)
{
  const uint32_t *cell;
  int len;

  cell = fdt_getprop(fdt, node, prop, &len);
  if (len != sizeof(*cell)) {
    return 0;
  }
  return fdt32_to_cpu(*cell);
}

int check_lock_signed(const char *path, uint8_t * img_sig, uint8_t *img_lock, uint32_t mtd_region_size)
{
  int fd = 0, ret = -1;
  size_t offs, rcnt, end;
  uint8_t *buf, *fdt_aligned_buf;
  uint8_t magic_number[] = {0xd0, 0x0d, 0xfe, 0xed}; // magic number

  *img_lock = 0x00;
  *img_sig = 0x00;

  buf = (uint8_t *)malloc(mtd_region_size);
  fdt_aligned_buf = (uint8_t *)aligned_alloc(8, mtd_region_size);
  if (!buf || !fdt_aligned_buf) {
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
    while (offs < mtd_region_size) {
      rcnt = ((offs + MTD_PKT_SIZE) <= mtd_region_size) ? MTD_PKT_SIZE : (mtd_region_size - offs);
      rcnt = read(fd, buf, mtd_region_size);
      if (rcnt <= 0) {
        if (errno == EINTR) {
          continue;
        }
        syslog(LOG_WARNING,"%s: unexpected rcnt: %d", __func__, rcnt);
        break;
      }
      offs += rcnt;
    }
    if (offs < mtd_region_size) {
      break;
    }

    end = mtd_region_size - sizeof(magic_number);
    // Scan through the region looking for the version signature.
    for (offs = 0; offs < end; offs++) {
      const void *fdt = (const void *) (buf + offs);
      if (!memcmp(fdt, magic_number, sizeof(magic_number)) &&
          (fdt_totalsize(fdt) <= (mtd_region_size - offs)))
      {
        memset(fdt_aligned_buf, 0, mtd_region_size);
        memcpy(fdt_aligned_buf, fdt, fdt_totalsize(fdt));

        // if fdt header is not valid, keep finding the next magic number
        ret = fdt_check_header(fdt_aligned_buf);
        if (ret) {
          continue;
        }

        if (fdt_subnode_offset(fdt_aligned_buf, 0, FIT_SIG_NODENAME) >= 0) {
          *img_sig = 0x01;
        }

        if (fdt_getprop_u32(fdt_aligned_buf, 0, FIT_LOCK_NODENAME) == 1) {
          *img_lock = 0x01;
        }

        // if doesn't find lock or sign, keep finding the next magic number
        if (*img_lock || *img_sig)
          break;
      }
    }
  } while (0);

  free(buf);
  free(fdt_aligned_buf);
  if (fd > 0) {
    close(fd);
  }

  return ret;
}

bool get_mtd_name(char* mtd_name, char* partition_name, uint32_t* size)
{
    FILE* partitions = fopen("/proc/mtd", "r");
    char line[256], mnt_name[32];
    unsigned int mtd_no;
    bool found = false;

    if (!partitions) {
      return false;
    }
    while (fgets(line, sizeof(line), partitions)) {
      if (sscanf(line, "mtd%u: %x %*x %s",
            &mtd_no,size, mnt_name) == 3)  {
        if (!strcmp(partition_name, mnt_name)) {
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
  char mtd_name[32] = {0};
  uint32_t size;
  if (!get_mtd_name(mtd_name,"\"u-bootro\"",&size) && !get_mtd_name(mtd_name,"\"recovery\"",&size)) {
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

int main()
{
  char buf[128] = {0};
  char ver_rom[MAX_VALUE_LEN] = {0};
  char ver_uboot[MAX_VALUE_LEN] = {0};
  char rom_name[32] = {0};
  char romx_name[32] = {0};
  uint32_t rom_size = 0, romx_size = 0;
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
    printf("ROM version:             %s\n", ver_rom);
    printf("ROM U-Boot version:      %s\n", ver_uboot);
    if (get_mtd_name(rom_name,"\"rom\"", &rom_size)) {
      check_lock_signed(rom_name,&img_sig,&img_lock,rom_size);
      printf("FLASH0 Signed:           0x%02x\n", img_sig);
      printf("FLASH0 Locked:           0x%02x\n", img_lock);
    }
    if (get_mtd_name(romx_name,"\"romx\"", &romx_size)) {
      check_lock_signed(romx_name,&img_sig,&img_lock,romx_size);
      printf("FLASH1 Signed:           0x%02x\n", img_sig);
      printf("FLASH1 Locked:           0x%02x\n", img_lock);
    }
    printf("\n");
  }
  printf("Status CRC: 0x%04x\n", v->crc);
#ifdef CONFIG_TPM_V1
  printf("TPM status  (%d)\n", v->error_tpm);
#endif
#ifdef CONFIG_TPM_V2
  printf("TPM2 status  (%d)\n", v->error_tpm2);
#endif
  printf("Status type (%d) code (%d)\n", v->error_type, v->error_code);
  printf("%s\n", vboot_error(v->error_code));

  return 0;
}

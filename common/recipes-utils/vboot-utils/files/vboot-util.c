#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <openbmc/vbs.h>

int main(int argc, char *argv[])
{
  char buf[128];
  struct vbs *v;

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
  printf("Status CRC: 0x%04x\n", v->crc);
  printf("TPM status  (%d)\n", v->error_tpm);
  printf("Status type (%d) code (%d)\n", v->error_type, v->error_code);
  printf("%s\n", vboot_error(v->error_code));
  return 0;
}
